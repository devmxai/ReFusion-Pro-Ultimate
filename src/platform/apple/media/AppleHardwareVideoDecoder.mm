#include "refusion/platform/PlatformMediaCapability.hpp"
#include "refusion/platform/apple/AppleMediaSurface.hpp"

#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <Metal/Metal.h>
#import <VideoToolbox/VideoToolbox.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace refusion::platform {
namespace {

using runtime::media::DecodedSurfaceInfo;
using runtime::media::DecodeState;
using runtime::media::HardwareDecodeRequest;
using runtime::media::HardwareDecodeResult;
using runtime::media::MediaPathCounters;
using runtime::media::NativeVideoSurfaceLease;

constexpr std::uintmax_t kMaximumFixtureBytes = 8U * 1024U * 1024U;

struct CounterState final {
  [[nodiscard]] MediaPathCounters snapshot() const {
    std::scoped_lock lock(mutex);
    return counters;
  }

  template <typename Member>
  void increment(Member member, const std::uint64_t amount = 1) {
    std::scoped_lock lock(mutex);
    counters.*member += amount;
  }

  mutable std::mutex mutex;
  MediaPathCounters counters;
};

struct NalUnit final {
  std::span<const std::uint8_t> bytes;
  std::uint8_t type{0};
};

struct AnnexBStream final {
  std::vector<std::uint8_t> parameter_set_sequence;
  std::vector<std::uint8_t> parameter_set_picture;
  std::vector<std::vector<NalUnit>> access_units;
};

struct StartCode final {
  std::size_t offset{0};
  std::size_t length{0};
};

[[nodiscard]] std::optional<StartCode> find_start_code(const std::span<const std::uint8_t> bytes,
                                                       const std::size_t begin) noexcept {
  for (std::size_t index = begin; index + 3 <= bytes.size(); ++index) {
    if (index + 4 <= bytes.size() && bytes[index] == 0 && bytes[index + 1] == 0 &&
        bytes[index + 2] == 0 && bytes[index + 3] == 1) {
      return StartCode{.offset = index, .length = 4};
    }
    if (bytes[index] == 0 && bytes[index + 1] == 0 && bytes[index + 2] == 1) {
      return StartCode{.offset = index, .length = 3};
    }
  }
  return std::nullopt;
}

[[nodiscard]] AnnexBStream parse_annex_b(const std::span<const std::uint8_t> bytes) {
  AnnexBStream parsed;
  std::vector<NalUnit> units;
  auto start = find_start_code(bytes, 0);
  while (start) {
    const auto payload_start = start->offset + start->length;
    const auto next = find_start_code(bytes, payload_start);
    const auto payload_end = next ? next->offset : bytes.size();
    if (payload_start < payload_end) {
      const auto payload = bytes.subspan(payload_start, payload_end - payload_start);
      units.push_back(NalUnit{
          .bytes = payload,
          .type = static_cast<std::uint8_t>(payload.front() & 0x1FU),
      });
    }
    start = next;
  }

  std::vector<NalUnit> current_access_unit;
  for (const auto &unit : units) {
    if (unit.type == 7 && parsed.parameter_set_sequence.empty()) {
      parsed.parameter_set_sequence.assign(unit.bytes.begin(), unit.bytes.end());
      continue;
    }
    if (unit.type == 8 && parsed.parameter_set_picture.empty()) {
      parsed.parameter_set_picture.assign(unit.bytes.begin(), unit.bytes.end());
      continue;
    }
    if (unit.type == 9) {
      if (!current_access_unit.empty()) {
        parsed.access_units.push_back(std::move(current_access_unit));
        current_access_unit.clear();
      }
      continue;
    }
    if (unit.type != 7 && unit.type != 8) {
      current_access_unit.push_back(unit);
    }
  }
  if (!current_access_unit.empty()) {
    parsed.access_units.push_back(std::move(current_access_unit));
  }
  return parsed;
}

[[nodiscard]] std::vector<std::uint8_t> make_avcc_sample(const std::vector<NalUnit> &access_unit) {
  std::vector<std::uint8_t> sample;
  for (const auto &unit : access_unit) {
    if (unit.bytes.empty() || unit.bytes.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw std::runtime_error("H.264 access unit contains an invalid NAL size");
    }
    const auto size = static_cast<std::uint32_t>(unit.bytes.size());
    sample.push_back(static_cast<std::uint8_t>((size >> 24U) & 0xFFU));
    sample.push_back(static_cast<std::uint8_t>((size >> 16U) & 0xFFU));
    sample.push_back(static_cast<std::uint8_t>((size >> 8U) & 0xFFU));
    sample.push_back(static_cast<std::uint8_t>(size & 0xFFU));
    sample.insert(sample.end(), unit.bytes.begin(), unit.bytes.end());
  }
  return sample;
}

[[nodiscard]] bool image_buffer_is_rec709(const CVImageBufferRef image_buffer) noexcept {
  CFTypeRef primaries =
      CVBufferCopyAttachment(image_buffer, kCVImageBufferColorPrimariesKey, nullptr);
  CFTypeRef transfer =
      CVBufferCopyAttachment(image_buffer, kCVImageBufferTransferFunctionKey, nullptr);
  CFTypeRef matrix = CVBufferCopyAttachment(image_buffer, kCVImageBufferYCbCrMatrixKey, nullptr);
  const bool matches = primaries != nullptr && transfer != nullptr && matrix != nullptr &&
                       CFEqual(primaries, kCVImageBufferColorPrimaries_ITU_R_709_2) &&
                       CFEqual(transfer, kCVImageBufferTransferFunction_ITU_R_709_2) &&
                       CFEqual(matrix, kCVImageBufferYCbCrMatrix_ITU_R_709_2);
  if (primaries != nullptr) {
    CFRelease(primaries);
  }
  if (transfer != nullptr) {
    CFRelease(transfer);
  }
  if (matrix != nullptr) {
    CFRelease(matrix);
  }
  return matches;
}

class AppleDecodeLifetime final {
 public:
  explicit AppleDecodeLifetime(runtime::gpu::DeviceLease device_lease)
      : device_lease_(std::move(device_lease)) {
    const auto handles = device_lease_.native_handles();
    id<MTLDevice> device = (__bridge id<MTLDevice>)(reinterpret_cast<void *>(handles.device));
    if (device == nil ||
        static_cast<std::uint64_t>(device.registryID) != device_lease_.identity().adapter_id) {
      throw std::runtime_error("Decoded surfaces require the admitted engine Metal device");
    }
    const auto status =
        CVMetalTextureCacheCreate(kCFAllocatorDefault, nullptr, device, nullptr, &texture_cache_);
    if (status != kCVReturnSuccess || texture_cache_ == nullptr) {
      throw std::runtime_error("CoreVideo could not create the decoded-surface texture cache");
    }
  }

  ~AppleDecodeLifetime() {
    if (texture_cache_ != nullptr) {
      CFRelease(texture_cache_);
    }
  }

  AppleDecodeLifetime(const AppleDecodeLifetime &) = delete;
  AppleDecodeLifetime &operator=(const AppleDecodeLifetime &) = delete;

  [[nodiscard]] const runtime::gpu::DeviceIdentity &device_identity() const noexcept {
    return device_lease_.identity();
  }

  [[nodiscard]] CVMetalTextureCacheRef texture_cache() const noexcept { return texture_cache_; }

 private:
  runtime::gpu::DeviceLease device_lease_;
  CVMetalTextureCacheRef texture_cache_{nullptr};
};

class AppleDecodedSurfaceLease final : public NativeVideoSurfaceLease {
 public:
  AppleDecodedSurfaceLease(std::shared_ptr<AppleDecodeLifetime> lifetime,
                           std::shared_ptr<CounterState> counter_state,
                           const CVImageBufferRef image_buffer,
                           const HardwareDecodeRequest &request, const CMTime presentation_time,
                           const CMTime duration, const std::uint64_t lease_id)
      : lifetime_(std::move(lifetime)), counter_state_(std::move(counter_state)) {
    if (image_buffer == nullptr || !CMTIME_IS_NUMERIC(presentation_time) ||
        !CMTIME_IS_NUMERIC(duration) || duration.value <= 0 ||
        CVPixelBufferGetPixelFormatType(image_buffer) !=
            kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange ||
        !CVPixelBufferIsPlanar(image_buffer) || CVPixelBufferGetPlaneCount(image_buffer) != 2 ||
        CVPixelBufferGetWidth(image_buffer) != request.expected_profile.coded_width ||
        CVPixelBufferGetHeight(image_buffer) != request.expected_profile.coded_height ||
        !image_buffer_is_rec709(image_buffer)) {
      throw std::runtime_error(
          "VideoToolbox output violated the NV12 video-range Rec.709 contract");
    }

    pixel_buffer_ = CVPixelBufferRetain(image_buffer);
    const auto luma_width = CVPixelBufferGetWidthOfPlane(pixel_buffer_, 0);
    const auto luma_height = CVPixelBufferGetHeightOfPlane(pixel_buffer_, 0);
    const auto chroma_width = CVPixelBufferGetWidthOfPlane(pixel_buffer_, 1);
    const auto chroma_height = CVPixelBufferGetHeightOfPlane(pixel_buffer_, 1);
    const auto luma_status = CVMetalTextureCacheCreateTextureFromImage(
        kCFAllocatorDefault, lifetime_->texture_cache(), pixel_buffer_, nullptr,
        MTLPixelFormatR8Unorm, luma_width, luma_height, 0, &luma_view_);
    const auto chroma_status = CVMetalTextureCacheCreateTextureFromImage(
        kCFAllocatorDefault, lifetime_->texture_cache(), pixel_buffer_, nullptr,
        MTLPixelFormatRG8Unorm, chroma_width, chroma_height, 1, &chroma_view_);
    id<MTLTexture> luma_texture =
        luma_view_ == nullptr ? nil : CVMetalTextureGetTexture(luma_view_);
    id<MTLTexture> chroma_texture =
        chroma_view_ == nullptr ? nil : CVMetalTextureGetTexture(chroma_view_);
    const auto adapter_id = lifetime_->device_identity().adapter_id;
    if (luma_status != kCVReturnSuccess || chroma_status != kCVReturnSuccess ||
        luma_texture == nil || chroma_texture == nil ||
        static_cast<std::uint64_t>(luma_texture.device.registryID) != adapter_id ||
        static_cast<std::uint64_t>(chroma_texture.device.registryID) != adapter_id) {
      throw std::runtime_error("Decoded NV12 planes did not bind to the engine Metal adapter");
    }

    info_ = DecodedSurfaceInfo{
        .lease_id = lease_id,
        .source_frame_index = request.source_frame_index,
        .profile = request.expected_profile,
        .timing =
            {
                .presentation_time =
                    {
                        .value = presentation_time.value,
                        .timescale = presentation_time.timescale,
                    },
                .duration =
                    {
                        .value = duration.value,
                        .timescale = duration.timescale,
                    },
            },
        .device = lifetime_->device_identity(),
        .plane_count = 2,
    };
    counter_state_->increment(&MediaPathCounters::native_surface_allocations);
    counter_state_->increment(&MediaPathCounters::native_surface_plane_bindings, 2);
    counter_state_->increment(&MediaPathCounters::native_surface_leases_issued);
  }

  ~AppleDecodedSurfaceLease() override {
    if (luma_view_ != nullptr) {
      CFRelease(luma_view_);
    }
    if (chroma_view_ != nullptr) {
      CFRelease(chroma_view_);
    }
    if (pixel_buffer_ != nullptr) {
      CVPixelBufferRelease(pixel_buffer_);
    }
    counter_state_->increment(&MediaPathCounters::native_surface_leases_released);
  }

  [[nodiscard]] const DecodedSurfaceInfo &info() const noexcept override { return info_; }

  [[nodiscard]] std::uintptr_t luma_texture() const noexcept {
    id<MTLTexture> texture = luma_view_ == nullptr ? nil : CVMetalTextureGetTexture(luma_view_);
    return reinterpret_cast<std::uintptr_t>((__bridge void *)texture);
  }

  [[nodiscard]] std::uintptr_t chroma_texture() const noexcept {
    id<MTLTexture> texture = chroma_view_ == nullptr ? nil : CVMetalTextureGetTexture(chroma_view_);
    return reinterpret_cast<std::uintptr_t>((__bridge void *)texture);
  }

  [[nodiscard]] std::uint32_t luma_width() const noexcept {
    return static_cast<std::uint32_t>(CVPixelBufferGetWidthOfPlane(pixel_buffer_, 0));
  }

  [[nodiscard]] std::uint32_t luma_height() const noexcept {
    return static_cast<std::uint32_t>(CVPixelBufferGetHeightOfPlane(pixel_buffer_, 0));
  }

  [[nodiscard]] std::uint32_t chroma_width() const noexcept {
    return static_cast<std::uint32_t>(CVPixelBufferGetWidthOfPlane(pixel_buffer_, 1));
  }

  [[nodiscard]] std::uint32_t chroma_height() const noexcept {
    return static_cast<std::uint32_t>(CVPixelBufferGetHeightOfPlane(pixel_buffer_, 1));
  }

 private:
  std::shared_ptr<AppleDecodeLifetime> lifetime_;
  std::shared_ptr<CounterState> counter_state_;
  CVPixelBufferRef pixel_buffer_{nullptr};
  CVMetalTextureRef luma_view_{nullptr};
  CVMetalTextureRef chroma_view_{nullptr};
  DecodedSurfaceInfo info_;
};

struct DecodeCallbackContext final {
  std::mutex mutex;
  std::shared_ptr<AppleDecodeLifetime> lifetime;
  std::shared_ptr<CounterState> counter_state;
  HardwareDecodeRequest request;
  std::shared_ptr<const NativeVideoSurfaceLease> surface;
  std::string diagnostic;
  OSStatus status{noErr};
  std::uint64_t lease_id{0};
};

void decompression_output_callback(void *output_context, void *, const OSStatus status,
                                   const VTDecodeInfoFlags info_flags,
                                   CVImageBufferRef image_buffer, const CMTime presentation_time,
                                   const CMTime duration) {
  auto &context = *static_cast<DecodeCallbackContext *>(output_context);
  std::scoped_lock lock(context.mutex);
  if (status != noErr || (info_flags & kVTDecodeInfo_FrameDropped) != 0 ||
      image_buffer == nullptr) {
    context.status = status == noErr ? kVTVideoDecoderBadDataErr : status;
    context.diagnostic = "VideoToolbox did not return the requested frame";
    return;
  }

  try {
    auto surface = std::make_shared<AppleDecodedSurfaceLease>(
        context.lifetime, context.counter_state, image_buffer, context.request, presentation_time,
        duration, context.lease_id);
    context.counter_state->increment(&MediaPathCounters::hardware_frames_decoded);
    context.surface = std::move(surface);
  } catch (const std::exception &error) {
    context.status = kVTVideoDecoderBadDataErr;
    context.diagnostic = error.what();
  }
}

class AppleHardwareVideoDecoder final : public runtime::media::HardwareVideoDecoder {
 public:
  explicit AppleHardwareVideoDecoder(runtime::gpu::GpuDeviceService &gpu_device_service)
      : gpu_device_service_(gpu_device_service), counter_state_(std::make_shared<CounterState>()) {}

  [[nodiscard]] HardwareDecodeResult decode(const HardwareDecodeRequest &request) override {
    std::scoped_lock operation_lock(operation_mutex_);
    counter_state_->increment(&MediaPathCounters::hardware_decoder_queries);
    if (!request.valid()) {
      return failure(DecodeState::invalid_request, "RFX-MEDIA-DECODE-REQUEST-INVALID",
                     "The hardware decode request is incomplete or invalid");
    }

    std::optional<runtime::gpu::DeviceLease> gpu_lease;
    try {
      gpu_lease.emplace(gpu_device_service_.borrow());
    } catch (const std::exception &error) {
      return failure(DecodeState::device_unavailable, "RFX-MEDIA-GPU-NOT-READY",
                     std::string("The engine GPU device is not ready: ") + error.what());
    }
    if (!gpu_lease->valid() || gpu_lease->identity().backend != runtime::gpu::Backend::metal) {
      counter_state_->increment(&MediaPathCounters::cross_adapter_events);
      return failure(DecodeState::device_unavailable, "RFX-MEDIA-METAL-DEVICE-REQUIRED",
                     "Apple decode requires the engine Metal device");
    }
    if (!VTIsHardwareDecodeSupported(kCMVideoCodecType_H264)) {
      return failure(DecodeState::unsupported, "RFX-MEDIA-H264-HARDWARE-UNAVAILABLE",
                     "VideoToolbox reports no H.264 hardware decoder");
    }
    counter_state_->increment(&MediaPathCounters::hardware_decoder_admissions);

    std::ifstream input(request.source_path, std::ios::binary | std::ios::ate);
    if (!input) {
      return failure(DecodeState::source_open_failed, "RFX-MEDIA-SOURCE-OPEN",
                     "The bounded H.264 fixture could not be opened");
    }
    const auto end = input.tellg();
    if (end <= 0 || static_cast<std::uintmax_t>(end) > kMaximumFixtureBytes) {
      return failure(DecodeState::source_invalid, "RFX-MEDIA-SOURCE-BOUNDS",
                     "The bounded H.264 fixture has an invalid size");
    }
    std::vector<std::uint8_t> source(static_cast<std::size_t>(end));
    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char *>(source.data()),
               static_cast<std::streamsize>(source.size()));
    if (!input) {
      return failure(DecodeState::source_open_failed, "RFX-MEDIA-SOURCE-READ",
                     "The bounded H.264 fixture could not be read completely");
    }

    const auto parsed = parse_annex_b(source);
    if (parsed.parameter_set_sequence.empty() || parsed.parameter_set_picture.empty() ||
        request.source_frame_index >= parsed.access_units.size()) {
      return failure(DecodeState::source_invalid, "RFX-MEDIA-H264-ANNEXB",
                     "The fixture lacks SPS, PPS, AUD-framed access units, or "
                     "the requested frame");
    }

    const std::array<const std::uint8_t *, 2> parameter_sets{
        parsed.parameter_set_sequence.data(),
        parsed.parameter_set_picture.data(),
    };
    const std::array<std::size_t, 2> parameter_set_sizes{
        parsed.parameter_set_sequence.size(),
        parsed.parameter_set_picture.size(),
    };
    CMVideoFormatDescriptionRef format_description = nullptr;
    auto status = CMVideoFormatDescriptionCreateFromH264ParameterSets(
        kCFAllocatorDefault, parameter_sets.size(), parameter_sets.data(),
        parameter_set_sizes.data(), 4, &format_description);
    if (status != noErr || format_description == nullptr) {
      return failure(DecodeState::source_invalid, "RFX-MEDIA-H264-FORMAT",
                     "CoreMedia rejected the H.264 parameter sets");
    }

    const auto dimensions = CMVideoFormatDescriptionGetDimensions(format_description);
    const bool format_matches =
        CMFormatDescriptionGetMediaSubType(format_description) == kCMVideoCodecType_H264 &&
        dimensions.width == static_cast<std::int32_t>(request.expected_profile.coded_width) &&
        dimensions.height == static_cast<std::int32_t>(request.expected_profile.coded_height);
    if (!format_matches) {
      CFRelease(format_description);
      return failure(DecodeState::source_invalid, "RFX-MEDIA-H264-PROFILE-MISMATCH",
                     "The H.264 format does not match the admitted codec/extent: " +
                         std::to_string(dimensions.width) + "x" +
                         std::to_string(dimensions.height));
    }

    std::shared_ptr<AppleDecodeLifetime> lifetime;
    try {
      lifetime = std::make_shared<AppleDecodeLifetime>(std::move(*gpu_lease));
    } catch (const std::exception &error) {
      CFRelease(format_description);
      return failure(DecodeState::native_surface_interop_failed, "RFX-MEDIA-METAL-TEXTURE-CACHE",
                     error.what());
    }

    NSDictionary *decoder_specification = @{
      (__bridge NSString *)
      kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder : @YES,
      (__bridge NSString *)
      kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder : @YES,
    };
    NSDictionary *output_attributes = @{
      (__bridge NSString *)
      kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange),
      (__bridge NSString *)kCVPixelBufferWidthKey : @(request.expected_profile.coded_width),
      (__bridge NSString *)kCVPixelBufferHeightKey : @(request.expected_profile.coded_height),
      (__bridge NSString *)kCVPixelBufferMetalCompatibilityKey : @YES,
      (__bridge NSString *)kCVPixelBufferIOSurfacePropertiesKey : @{},
    };
    DecodeCallbackContext callback_context{
        .lifetime = lifetime,
        .counter_state = counter_state_,
        .request = request,
        .lease_id = next_lease_id_.fetch_add(1),
    };
    const VTDecompressionOutputCallbackRecord callback{
        .decompressionOutputCallback = decompression_output_callback,
        .decompressionOutputRefCon = &callback_context,
    };
    VTDecompressionSessionRef session = nullptr;
    status = VTDecompressionSessionCreate(
        kCFAllocatorDefault, format_description, (__bridge CFDictionaryRef)decoder_specification,
        (__bridge CFDictionaryRef)output_attributes, &callback, &session);
    if (status != noErr || session == nullptr) {
      CFRelease(format_description);
      return failure(DecodeState::session_failed, "RFX-MEDIA-VT-SESSION",
                     "VideoToolbox could not create a hardware-required "
                     "decompression session");
    }

    CFTypeRef hardware_property = nullptr;
    status = VTSessionCopyProperty(session,
                                   kVTDecompressionPropertyKey_UsingHardwareAcceleratedVideoDecoder,
                                   nullptr, &hardware_property);
    const bool using_hardware = status == noErr && hardware_property != nullptr &&
                                CFEqual(hardware_property, kCFBooleanTrue);
    if (hardware_property != nullptr) {
      CFRelease(hardware_property);
    }
    if (!using_hardware) {
      VTDecompressionSessionInvalidate(session);
      CFRelease(session);
      CFRelease(format_description);
      counter_state_->increment(&MediaPathCounters::software_decoder_selections);
      return failure(DecodeState::session_failed, "RFX-MEDIA-VT-HARDWARE-REQUIRED",
                     "VideoToolbox did not confirm the required hardware decoder");
    }
    counter_state_->increment(&MediaPathCounters::hardware_decoder_sessions);

    CMBlockBufferRef block_buffer = nullptr;
    CMSampleBufferRef sample_buffer = nullptr;
    try {
      const auto avcc_sample = make_avcc_sample(parsed.access_units[request.source_frame_index]);
      status = CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault, nullptr, avcc_sample.size(),
                                                  kCFAllocatorDefault, nullptr, 0,
                                                  avcc_sample.size(), 0, &block_buffer);
      if (status == noErr) {
        status =
            CMBlockBufferReplaceDataBytes(avcc_sample.data(), block_buffer, 0, avcc_sample.size());
      }
      const CMSampleTimingInfo timing{
          .duration = CMTimeMake(request.packet_timing.duration.value,
                                 request.packet_timing.duration.timescale),
          .presentationTimeStamp = CMTimeMake(request.packet_timing.presentation_time.value,
                                              request.packet_timing.presentation_time.timescale),
          .decodeTimeStamp = CMTimeMake(request.packet_timing.presentation_time.value,
                                        request.packet_timing.presentation_time.timescale),
      };
      const auto sample_size = avcc_sample.size();
      if (status == noErr) {
        status = CMSampleBufferCreateReady(kCFAllocatorDefault, block_buffer, format_description, 1,
                                           1, &timing, 1, &sample_size, &sample_buffer);
      }
    } catch (const std::exception &error) {
      callback_context.diagnostic = error.what();
      status = kVTVideoDecoderBadDataErr;
    }

    if (status == noErr && sample_buffer != nullptr) {
      VTDecodeInfoFlags info_flags = 0;
      counter_state_->increment(&MediaPathCounters::compressed_samples_submitted);
      status = VTDecompressionSessionDecodeFrame(session, sample_buffer,
                                                 kVTDecodeFrame_EnableAsynchronousDecompression,
                                                 nullptr, &info_flags);
      if (status == noErr) {
        status = VTDecompressionSessionWaitForAsynchronousFrames(session);
      }
    }

    if (sample_buffer != nullptr) {
      CFRelease(sample_buffer);
    }
    if (block_buffer != nullptr) {
      CFRelease(block_buffer);
    }
    VTDecompressionSessionInvalidate(session);
    CFRelease(session);
    CFRelease(format_description);

    std::shared_ptr<const NativeVideoSurfaceLease> surface;
    std::string callback_diagnostic;
    OSStatus callback_status = noErr;
    {
      std::scoped_lock callback_lock(callback_context.mutex);
      surface = callback_context.surface;
      callback_diagnostic = callback_context.diagnostic;
      callback_status = callback_context.status;
    }
    if (status != noErr || callback_status != noErr || !surface) {
      return failure(DecodeState::decode_failed, "RFX-MEDIA-VT-DECODE",
                     callback_diagnostic.empty()
                         ? "VideoToolbox rejected the compressed H.264 sample"
                         : std::move(callback_diagnostic));
    }

    return HardwareDecodeResult{
        .state = DecodeState::decoded,
        .hardware_decoder = true,
        .surface = std::move(surface),
        .counters = counter_state_->snapshot(),
        .code = "RFX-MEDIA-H264-HARDWARE-DECODED",
        .diagnostic = "VideoToolbox decoded H.264 to a same-device NV12 Metal "
                      "surface lease",
    };
  }

  [[nodiscard]] MediaPathCounters counters() const override { return counter_state_->snapshot(); }

 private:
  [[nodiscard]] HardwareDecodeResult failure(DecodeState state, std::string code,
                                             std::string diagnostic) const {
    return HardwareDecodeResult{
        .state = state,
        .hardware_decoder = false,
        .surface = nullptr,
        .counters = counter_state_->snapshot(),
        .code = std::move(code),
        .diagnostic = std::move(diagnostic),
    };
  }

  runtime::gpu::GpuDeviceService &gpu_device_service_;
  std::shared_ptr<CounterState> counter_state_;
  std::mutex operation_mutex_;
  std::atomic_uint64_t next_lease_id_{1};
};

}  // namespace

namespace apple {

bool MetalVideoSurfaceView::valid() const noexcept {
  return surface && luma_texture != 0 && chroma_texture != 0 && luma_width > 0 && luma_height > 0 &&
         chroma_width > 0 && chroma_height > 0;
}

std::optional<MetalVideoSurfaceView> borrow_metal_video_surface(
    std::shared_ptr<const runtime::media::NativeVideoSurfaceLease> surface) {
  const auto *apple_surface = dynamic_cast<const AppleDecodedSurfaceLease *>(surface.get());
  if (apple_surface == nullptr) {
    return std::nullopt;
  }
  MetalVideoSurfaceView view{
      .surface = std::move(surface),
      .luma_texture = apple_surface->luma_texture(),
      .chroma_texture = apple_surface->chroma_texture(),
      .luma_width = apple_surface->luma_width(),
      .luma_height = apple_surface->luma_height(),
      .chroma_width = apple_surface->chroma_width(),
      .chroma_height = apple_surface->chroma_height(),
  };
  if (!view.valid()) {
    return std::nullopt;
  }
  return view;
}

}  // namespace apple

std::unique_ptr<runtime::media::HardwareVideoDecoder> create_platform_hardware_video_decoder(
    runtime::gpu::GpuDeviceService &gpu_device_service) {
  return std::make_unique<AppleHardwareVideoDecoder>(gpu_device_service);
}

}  // namespace refusion::platform
