#include "refusion/platform/PlatformMediaCapability.hpp"
#include "refusion/platform/apple/AppleMediaSurface.hpp"

#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <Metal/Metal.h>
#import <VideoToolbox/VideoToolbox.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
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
#include <unordered_set>
#include <utility>
#include <vector>

namespace refusion::platform {
namespace {

using runtime::media::DecodedSurfaceInfo;
using runtime::media::DecodedSurfaceQueue;
using runtime::media::DecodeState;
using runtime::media::HardwareDecodeRequest;
using runtime::media::HardwareDecodeResult;
using runtime::media::HardwareDecodeSequenceRequest;
using runtime::media::HardwareDecodeSequenceResult;
using runtime::media::HardwareVideoPlaybackSession;
using runtime::media::HardwareVideoPlaybackSource;
using runtime::media::HardwareVideoPlaybackWindowRequest;
using runtime::media::HardwareVideoPlaybackWindowResult;
using runtime::media::MediaPathCounters;
using runtime::media::NativeVideoSurfaceLease;
using runtime::gpu::GpuObservabilityService;
using runtime::gpu::GpuObservedFenceLease;
using runtime::gpu::GpuObservedResourceLease;
using runtime::gpu::GpuResourceKind;
using runtime::gpu::GpuSubsystem;

constexpr std::uintmax_t kMaximumFixtureBytes = 8U * 1024U * 1024U;
constexpr std::uint64_t kMaximumCompressedSampleBytes = 64ULL * 1024ULL * 1024ULL;

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

struct AvcConfiguration final {
  std::vector<std::uint8_t> sequence_parameter_set;
  std::vector<std::uint8_t> picture_parameter_set;
  std::uint8_t nal_length_size{0};

  [[nodiscard]] bool valid() const noexcept {
    return !sequence_parameter_set.empty() && !picture_parameter_set.empty() &&
           (nal_length_size == 1 || nal_length_size == 2 ||
            nal_length_size == 4);
  }
};

[[nodiscard]] std::optional<AvcConfiguration> parse_avcc_configuration(
    const std::span<const std::uint8_t> bytes) {
  if (bytes.size() < 7 || bytes[0] != 1) return std::nullopt;
  AvcConfiguration result{
      .nal_length_size = static_cast<std::uint8_t>((bytes[4] & 0x03U) + 1U),
  };
  std::size_t cursor = 6;
  const auto sps_count = static_cast<std::size_t>(bytes[5] & 0x1fU);
  for (std::size_t index = 0; index < sps_count; ++index) {
    if (cursor + 2 > bytes.size()) return std::nullopt;
    const auto size = static_cast<std::size_t>(bytes[cursor] << 8U) |
                      static_cast<std::size_t>(bytes[cursor + 1]);
    cursor += 2;
    if (size == 0 || cursor + size > bytes.size()) return std::nullopt;
    if (result.sequence_parameter_set.empty()) {
      result.sequence_parameter_set.assign(bytes.begin() + cursor,
                                           bytes.begin() + cursor + size);
    }
    cursor += size;
  }
  if (cursor >= bytes.size()) return std::nullopt;
  const auto pps_count = static_cast<std::size_t>(bytes[cursor++]);
  for (std::size_t index = 0; index < pps_count; ++index) {
    if (cursor + 2 > bytes.size()) return std::nullopt;
    const auto size = static_cast<std::size_t>(bytes[cursor] << 8U) |
                      static_cast<std::size_t>(bytes[cursor + 1]);
    cursor += 2;
    if (size == 0 || cursor + size > bytes.size()) return std::nullopt;
    if (result.picture_parameter_set.empty()) {
      result.picture_parameter_set.assign(bytes.begin() + cursor,
                                          bytes.begin() + cursor + size);
    }
    cursor += size;
  }
  return result.valid() ? std::optional<AvcConfiguration>{std::move(result)}
                        : std::nullopt;
}

[[nodiscard]] CMVideoFormatDescriptionRef make_avcc_format_description(
    const AvcConfiguration& configuration) {
  const std::array<const std::uint8_t*, 2> parameter_sets{
      configuration.sequence_parameter_set.data(),
      configuration.picture_parameter_set.data(),
  };
  const std::array<std::size_t, 2> parameter_set_sizes{
      configuration.sequence_parameter_set.size(),
      configuration.picture_parameter_set.size(),
  };
  CMVideoFormatDescriptionRef result = nullptr;
  const auto status = CMVideoFormatDescriptionCreateFromH264ParameterSets(
      kCFAllocatorDefault, parameter_sets.size(), parameter_sets.data(),
      parameter_set_sizes.data(), configuration.nal_length_size, &result);
  return status == noErr ? result : nullptr;
}

[[nodiscard]] std::optional<std::vector<std::uint8_t>> read_compressed_sample(
    std::ifstream& input,
    const runtime::media::CompressedSampleDescriptor& sample) {
  if (sample.source_byte_size == 0 ||
      sample.source_byte_size > kMaximumCompressedSampleBytes ||
      sample.source_byte_offset >
          static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    return std::nullopt;
  }
  std::vector<std::uint8_t> bytes(sample.source_byte_size);
  input.clear();
  input.seekg(static_cast<std::streamoff>(sample.source_byte_offset),
              std::ios::beg);
  if (!input) return std::nullopt;
  input.read(reinterpret_cast<char*>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
  return input ? std::optional<std::vector<std::uint8_t>>{std::move(bytes)}
               : std::nullopt;
}

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

void apply_accepted_rec709_metadata(const CVImageBufferRef image_buffer) {
  if (image_buffer == nullptr) return;
  CVBufferSetAttachment(image_buffer, kCVImageBufferColorPrimariesKey,
                        kCVImageBufferColorPrimaries_ITU_R_709_2,
                        kCVAttachmentMode_ShouldPropagate);
  CVBufferSetAttachment(image_buffer, kCVImageBufferTransferFunctionKey,
                        kCVImageBufferTransferFunction_ITU_R_709_2,
                        kCVAttachmentMode_ShouldPropagate);
  CVBufferSetAttachment(image_buffer, kCVImageBufferYCbCrMatrixKey,
                        kCVImageBufferYCbCrMatrix_ITU_R_709_2,
                        kCVAttachmentMode_ShouldPropagate);
}

class AppleDecodeLifetime final {
 public:
  explicit AppleDecodeLifetime(runtime::gpu::BackendDeviceLease device_lease)
      : device_lease_(std::move(device_lease)) {
    id<MTLDevice> device = (__bridge id<MTLDevice>)(const_cast<void *>(
        device_lease_.backend_private_device()));
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
  runtime::gpu::BackendDeviceLease device_lease_;
  CVMetalTextureCacheRef texture_cache_{nullptr};
};

class AppleDecodedSurfaceLease final : public NativeVideoSurfaceLease {
 public:
  AppleDecodedSurfaceLease(std::shared_ptr<AppleDecodeLifetime> lifetime,
                           std::shared_ptr<CounterState> counter_state,
                           std::shared_ptr<GpuObservabilityService> observability,
                           const CVImageBufferRef image_buffer,
                           const HardwareDecodeRequest &request, const CMTime presentation_time,
                           const CMTime duration, const std::uint64_t lease_id)
      : lifetime_(std::move(lifetime)), counter_state_(std::move(counter_state)) {
    const auto expected_presentation_time =
        CMTimeMake(request.packet_timing.presentation_time.value,
                   request.packet_timing.presentation_time.timescale);
    const auto expected_duration =
        CMTimeMake(request.packet_timing.duration.value, request.packet_timing.duration.timescale);
    if (image_buffer == nullptr || !CMTIME_IS_NUMERIC(presentation_time) ||
        !CMTIME_IS_NUMERIC(duration) || duration.value <= 0 ||
        CMTimeCompare(presentation_time, expected_presentation_time) != 0 ||
        CMTimeCompare(duration, expected_duration) != 0 ||
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
    if (observability) {
      const auto resident_bytes =
          static_cast<std::uint64_t>(
              CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer_, 0)) *
              luma_height +
          static_cast<std::uint64_t>(
              CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer_, 1)) *
              chroma_height;
      observed_resource_ = std::make_unique<GpuObservedResourceLease>(
          std::move(observability), GpuSubsystem::media,
          GpuResourceKind::native_video_surface,
          lifetime_->device_identity().generation, resident_bytes);
    }
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
  std::unique_ptr<GpuObservedResourceLease> observed_resource_;
};

struct SequenceCallbackContext final {
  std::mutex mutex;
  std::shared_ptr<AppleDecodeLifetime> lifetime;
  std::shared_ptr<CounterState> counter_state;
  std::shared_ptr<GpuObservabilityService> observability;
  std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> surfaces;
  std::string diagnostic;
  OSStatus status{noErr};
};

struct SampleCallbackContext final {
  SequenceCallbackContext *sequence{nullptr};
  HardwareDecodeRequest request;
  std::uint64_t lease_id{0};
  bool retain_surface{true};
};

void decompression_output_callback(void *output_context, void *source_frame_context,
                                   const OSStatus status, const VTDecodeInfoFlags info_flags,
                                   CVImageBufferRef image_buffer, const CMTime presentation_time,
                                   const CMTime duration) {
  auto &sequence = *static_cast<SequenceCallbackContext *>(output_context);
  auto *sample = static_cast<SampleCallbackContext *>(source_frame_context);
  std::scoped_lock lock(sequence.mutex);
  if (status != noErr || (info_flags & kVTDecodeInfo_FrameDropped) != 0 ||
      image_buffer == nullptr || sample == nullptr || sample->sequence != &sequence) {
    sequence.status = status == noErr ? kVTVideoDecoderBadDataErr : status;
    sequence.diagnostic = "VideoToolbox did not return a requested sequence frame";
    return;
  }

  try {
    apply_accepted_rec709_metadata(image_buffer);
    sequence.counter_state->increment(&MediaPathCounters::hardware_frames_decoded);
    if (!sample->retain_surface) {
      return;
    }
    auto surface = std::make_shared<AppleDecodedSurfaceLease>(
        sequence.lifetime, sequence.counter_state, sequence.observability,
        image_buffer, sample->request, presentation_time, duration,
        sample->lease_id);
    sequence.surfaces.push_back(std::move(surface));
  } catch (const std::exception &error) {
    sequence.status = kVTVideoDecoderBadDataErr;
    sequence.diagnostic = error.what();
  }
}

class AppleHardwareVideoPlaybackSession final
    : public HardwareVideoPlaybackSession {
 public:
  AppleHardwareVideoPlaybackSession(
      HardwareVideoPlaybackSource source,
      runtime::gpu::GpuDeviceService& gpu_device_service,
      std::shared_ptr<CounterState> counter_state,
      std::shared_ptr<GpuObservabilityService> observability,
      std::atomic_uint64_t& next_lease_id)
      : source_(std::move(source)),
        counter_state_(std::move(counter_state)),
        observability_(std::move(observability)),
        next_lease_id_(next_lease_id),
        input_(source_.source_path, std::ios::binary | std::ios::ate) {
    if (!source_.valid() || !input_) {
      throw std::invalid_argument(
          "RFX-MEDIA-PLAYBACK-SOURCE-INVALID: source is unavailable");
    }
    const auto size = input_.tellg();
    if (size <= 0 || static_cast<std::uint64_t>(size) !=
                         source_.source_byte_size) {
      throw std::invalid_argument(
          "RFX-MEDIA-PLAYBACK-SOURCE-SIZE: accepted source size changed");
    }
    const auto configuration = parse_avcc_configuration(
        std::span<const std::uint8_t>(source_.codec_configuration));
    if (!configuration) {
      throw std::invalid_argument(
          "RFX-MEDIA-PLAYBACK-AVCC: codec configuration is not AVCDecoderConfigurationRecord");
    }
    format_description_ = make_avcc_format_description(*configuration);
    if (format_description_ == nullptr) {
      throw std::invalid_argument(
          "RFX-MEDIA-PLAYBACK-FORMAT: CoreMedia rejected AVC configuration");
    }
    const auto dimensions =
        CMVideoFormatDescriptionGetDimensions(format_description_);
    if (dimensions.width !=
            static_cast<std::int32_t>(source_.expected_profile.coded_width) ||
        dimensions.height !=
            static_cast<std::int32_t>(source_.expected_profile.coded_height)) {
      throw std::invalid_argument(
          "RFX-MEDIA-PLAYBACK-EXTENT: codec configuration differs from MediaIndex");
    }
    auto lease = gpu_device_service.borrow();
    if (!lease.valid() ||
        lease.identity().backend != runtime::gpu::Backend::metal ||
        !VTIsHardwareDecodeSupported(kCMVideoCodecType_H264)) {
      throw std::runtime_error(
          "RFX-MEDIA-PLAYBACK-HARDWARE: VideoToolbox hardware decode is unavailable");
    }
    counter_state_->increment(&MediaPathCounters::hardware_decoder_queries);
    counter_state_->increment(&MediaPathCounters::hardware_decoder_admissions);
    lifetime_ = std::make_shared<AppleDecodeLifetime>(std::move(lease));
    callback_context_.lifetime = lifetime_;
    callback_context_.counter_state = counter_state_;
    callback_context_.observability = observability_;
    create_session();
  }

  ~AppleHardwareVideoPlaybackSession() override {
    destroy_session();
    if (format_description_ != nullptr) {
      CFRelease(format_description_);
    }
  }

  [[nodiscard]] HardwareVideoPlaybackWindowResult decode_window(
      const HardwareVideoPlaybackWindowRequest& request) override {
    std::scoped_lock lock(operation_mutex_);
    if (!request.valid()) {
      return failure(DecodeState::invalid_request,
                     "RFX-MEDIA-PLAYBACK-WINDOW-INVALID",
                     "playback window request is invalid");
    }

    const auto target_decode_index = select_target_decode_index(
        request.target_presentation_time);
    if (!target_decode_index) {
      return failure(DecodeState::source_invalid,
                     "RFX-MEDIA-PLAYBACK-TARGET-BEFORE-STREAM",
                     "target precedes the first Video sample");
    }

    if (auto queue = resident_queue_for(request.target_presentation_time,
                                        request.maximum_surface_count,
                                        request.lookahead_surface_count)) {
      return success(std::move(queue));
    }

    const auto desired = desired_presentation_samples(
        *target_decode_index, request.maximum_surface_count);
    if (desired.empty()) {
      return failure(DecodeState::source_invalid,
                     "RFX-MEDIA-PLAYBACK-WINDOW-EMPTY",
                     "no presentation samples exist at the target");
    }
    std::size_t end_decode_index = *target_decode_index;
    std::unordered_set<std::uint64_t> retained_source_frames;
    retained_source_frames.reserve(desired.size());
    for (const auto index : desired) {
      end_decode_index = std::max(end_decode_index, index);
      retained_source_frames.insert(
          source_.samples_decode_order[index].source_frame_index);
    }

    const bool target_is_resident = resident_contains(
        request.target_presentation_time);
    constexpr std::size_t kMaximumSequentialCatchup = 96;
    if (session_ == nullptr ||
        (*target_decode_index < next_decode_index_ && !target_is_resident) ||
        (*target_decode_index > next_decode_index_ + kMaximumSequentialCatchup)) {
      reset_at_sync(*target_decode_index);
    }

    if (next_decode_index_ <= end_decode_index) {
      // Decode order can run ahead of presentation order for B-frames. Every
      // future presentation frame produced by this dependency range must stay
      // resident even when it was outside the initially requested presentation
      // subset. Dropping it would force a later seek back to the GOP sync frame
      // and cause progressively longer playback stalls.
      for (std::size_t index = next_decode_index_; index <= end_decode_index;
           ++index) {
        const auto& candidate = source_.samples_decode_order[index];
        if (runtime::media::compare_exact_media_time(
                candidate.timing.presentation_time,
                request.target_presentation_time) !=
            std::strong_ordering::less) {
          retained_source_frames.insert(candidate.source_frame_index);
        }
      }
      auto result = submit_samples(next_decode_index_, end_decode_index,
                                   retained_source_frames);
      if (!result.empty()) {
        return failure(DecodeState::decode_failed,
                       "RFX-MEDIA-PLAYBACK-DECODE", std::move(result));
      }
      next_decode_index_ = end_decode_index + 1;
    }

    auto queue = resident_queue_for(request.target_presentation_time,
                                    request.maximum_surface_count,
                                    request.lookahead_surface_count);
    if (!queue) {
      return failure(DecodeState::decode_failed,
                     "RFX-MEDIA-PLAYBACK-TARGET-MISSING",
                     "VideoToolbox did not publish the requested PTS window");
    }
    counter_state_->increment(&MediaPathCounters::surface_queues_published);
    return success(std::move(queue));
  }

  [[nodiscard]] MediaPathCounters counters() const override {
    return counter_state_->snapshot();
  }

 private:
  [[nodiscard]] HardwareVideoPlaybackWindowResult failure(
      const DecodeState state, std::string code, std::string diagnostic) const {
    return {
        .state = state,
        .hardware_decoder = false,
        .queue = nullptr,
        .counters = counter_state_->snapshot(),
        .code = std::move(code),
        .diagnostic = std::move(diagnostic),
    };
  }

  [[nodiscard]] HardwareVideoPlaybackWindowResult success(
      std::shared_ptr<const DecodedSurfaceQueue> queue) const {
    return {
        .state = DecodeState::decoded,
        .hardware_decoder = true,
        .queue = std::move(queue),
        .counters = counter_state_->snapshot(),
        .code = "RFX-MEDIA-PLAYBACK-WINDOW-DECODED",
        .diagnostic =
            "VideoToolbox published a bounded same-device NV12 playback window",
    };
  }

  void create_session() {
    if (session_ != nullptr) return;
    callback_context_.surfaces.clear();
    callback_context_.diagnostic.clear();
    callback_context_.status = noErr;
    NSDictionary* decoder_specification = @{
      (__bridge NSString*)
      kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder : @YES,
      (__bridge NSString*)
      kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder : @YES,
    };
    NSDictionary* output_attributes = @{
      (__bridge NSString*)kCVPixelBufferPixelFormatTypeKey :
          @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange),
      (__bridge NSString*)kCVPixelBufferWidthKey :
          @(source_.expected_profile.coded_width),
      (__bridge NSString*)kCVPixelBufferHeightKey :
          @(source_.expected_profile.coded_height),
      (__bridge NSString*)kCVPixelBufferMetalCompatibilityKey : @YES,
      (__bridge NSString*)kCVPixelBufferIOSurfacePropertiesKey : @{},
    };
    const VTDecompressionOutputCallbackRecord callback{
        .decompressionOutputCallback = decompression_output_callback,
        .decompressionOutputRefCon = &callback_context_,
    };
    auto status = VTDecompressionSessionCreate(
        kCFAllocatorDefault, format_description_,
        (__bridge CFDictionaryRef)decoder_specification,
        (__bridge CFDictionaryRef)output_attributes, &callback, &session_);
    if (status != noErr || session_ == nullptr) {
      session_ = nullptr;
      throw std::runtime_error(
          "RFX-MEDIA-PLAYBACK-VT-SESSION: cannot create hardware session");
    }
    CFTypeRef hardware_property = nullptr;
    status = VTSessionCopyProperty(
        session_, kVTDecompressionPropertyKey_UsingHardwareAcceleratedVideoDecoder,
        nullptr, &hardware_property);
    const bool hardware = status == noErr && hardware_property != nullptr &&
                          CFEqual(hardware_property, kCFBooleanTrue);
    if (hardware_property != nullptr) CFRelease(hardware_property);
    if (!hardware) {
      counter_state_->increment(&MediaPathCounters::software_decoder_selections);
      destroy_session();
      throw std::runtime_error(
          "RFX-MEDIA-PLAYBACK-VT-HARDWARE: decoder was not hardware accelerated");
    }
    counter_state_->increment(&MediaPathCounters::hardware_decoder_sessions);
  }

  void destroy_session() noexcept {
    if (session_ != nullptr) {
      VTDecompressionSessionInvalidate(session_);
      CFRelease(session_);
      session_ = nullptr;
    }
  }

  void reset_at_sync(const std::size_t target_decode_index) {
    std::size_t sync = target_decode_index;
    while (sync > 0 && !source_.samples_decode_order[sync].sync_sample) --sync;
    if (!source_.samples_decode_order[sync].sync_sample) {
      throw std::runtime_error(
          "RFX-MEDIA-PLAYBACK-SYNC: no sync sample precedes target");
    }
    destroy_session();
    resident_.clear();
    create_session();
    next_decode_index_ = sync;
  }

  [[nodiscard]] std::optional<std::size_t> select_target_decode_index(
      const runtime::media::ExactMediaTime target) const {
    std::optional<std::size_t> selected;
    for (std::size_t index = 0; index < source_.samples_decode_order.size();
         ++index) {
      const auto& sample = source_.samples_decode_order[index];
      if (runtime::media::compare_exact_media_time(
              sample.timing.presentation_time, target) !=
              std::strong_ordering::greater &&
          (!selected || runtime::media::compare_exact_media_time(
                            source_.samples_decode_order[*selected]
                                .timing.presentation_time,
                            sample.timing.presentation_time) ==
                            std::strong_ordering::less)) {
        selected = index;
      }
    }
    return selected;
  }

  [[nodiscard]] std::vector<std::size_t> desired_presentation_samples(
      const std::size_t target_decode_index,
      const std::size_t maximum_count) const {
    std::vector<std::size_t> order(source_.samples_decode_order.size());
    for (std::size_t index = 0; index < order.size(); ++index) order[index] = index;
    std::sort(order.begin(), order.end(), [this](const auto left, const auto right) {
      return runtime::media::compare_exact_media_time(
                 source_.samples_decode_order[left].timing.presentation_time,
                 source_.samples_decode_order[right].timing.presentation_time) ==
             std::strong_ordering::less;
    });
    const auto target_frame =
        source_.samples_decode_order[target_decode_index].source_frame_index;
    const auto iterator = std::find_if(order.begin(), order.end(),
                                       [this, target_frame](const auto index) {
      return source_.samples_decode_order[index].source_frame_index == target_frame;
    });
    if (iterator == order.end()) return {};
    const auto available = static_cast<std::size_t>(order.end() - iterator);
    const auto count = std::min(maximum_count, available);
    return {iterator, iterator + static_cast<std::ptrdiff_t>(count)};
  }

  [[nodiscard]] bool resident_contains(
      const runtime::media::ExactMediaTime target) const {
    if (resident_.empty()) return false;
    auto queue = DecodedSurfaceQueue::create(resident_);
    const auto selected = queue->select_at(target);
    if (!selected) return false;
    const auto& timing = selected->info().timing;
    if (timing.presentation_time.timescale != timing.duration.timescale ||
        timing.presentation_time.value >
            std::numeric_limits<std::int64_t>::max() - timing.duration.value) {
      return false;
    }
    return runtime::media::compare_exact_media_time(
               target,
               {.value = timing.presentation_time.value + timing.duration.value,
                .timescale = timing.presentation_time.timescale}) ==
           std::strong_ordering::less;
  }

  [[nodiscard]] std::shared_ptr<const DecodedSurfaceQueue> resident_queue_for(
      const runtime::media::ExactMediaTime target,
      const std::size_t maximum_count,
      const std::size_t required_lookahead) {
    if (!resident_contains(target)) return nullptr;
    auto sorted = resident_;
    std::sort(sorted.begin(), sorted.end(), [](const auto& left, const auto& right) {
      return runtime::media::compare_exact_media_time(
                 left->info().timing.presentation_time,
                 right->info().timing.presentation_time) ==
             std::strong_ordering::less;
    });
    auto queue = DecodedSurfaceQueue::create(sorted);
    const auto selected = queue->select_at(target);
    const auto iterator = std::find_if(
        sorted.begin(), sorted.end(), [&selected](const auto& value) {
          return value->info().lease_id == selected->info().lease_id;
        });
    if (iterator == sorted.end()) return nullptr;
    const auto available = static_cast<std::size_t>(sorted.end() - iterator);
    const bool source_exhausted =
        next_decode_index_ >= source_.samples_decode_order.size();
    if (available <= required_lookahead && !source_exhausted) return nullptr;
    const auto count = std::min(maximum_count, available);
    std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> bounded(
        iterator, iterator + static_cast<std::ptrdiff_t>(count));
    const auto selected_time = selected->info().timing.presentation_time;
    resident_.erase(
        std::remove_if(
            resident_.begin(), resident_.end(), [&selected_time](const auto& value) {
              return runtime::media::compare_exact_media_time(
                         value->info().timing.presentation_time,
                         selected_time) == std::strong_ordering::less;
            }),
        resident_.end());
    constexpr std::size_t kMaximumInternalResidentSurfaces = 16;
    if (resident_.size() > kMaximumInternalResidentSurfaces) {
      return nullptr;
    }
    return DecodedSurfaceQueue::create(std::move(bounded));
  }

  [[nodiscard]] std::string submit_samples(
      const std::size_t begin, const std::size_t end,
      const std::unordered_set<std::uint64_t>& retained_source_frames) {
    if (session_ == nullptr || begin > end ||
        end >= source_.samples_decode_order.size()) {
      return "invalid persistent decode range";
    }
    {
      std::scoped_lock callback_lock(callback_context_.mutex);
      callback_context_.surfaces.clear();
      callback_context_.diagnostic.clear();
      callback_context_.status = noErr;
    }
    std::vector<std::unique_ptr<SampleCallbackContext>> contexts;
    contexts.reserve(end - begin + 1);
    OSStatus status = noErr;
    for (std::size_t index = begin; index <= end; ++index) {
      const auto& descriptor = source_.samples_decode_order[index];
      const auto bytes = read_compressed_sample(input_, descriptor);
      if (!bytes) return "compressed sample byte range could not be read";
      CMBlockBufferRef block = nullptr;
      CMSampleBufferRef sample = nullptr;
      status = CMBlockBufferCreateWithMemoryBlock(
          kCFAllocatorDefault, nullptr, bytes->size(), kCFAllocatorDefault,
          nullptr, 0, bytes->size(), 0, &block);
      if (status == noErr) {
        status = CMBlockBufferReplaceDataBytes(bytes->data(), block, 0,
                                               bytes->size());
      }
      const CMSampleTimingInfo timing{
          .duration = CMTimeMake(descriptor.timing.duration.value,
                                 descriptor.timing.duration.timescale),
          .presentationTimeStamp =
              CMTimeMake(descriptor.timing.presentation_time.value,
                         descriptor.timing.presentation_time.timescale),
          .decodeTimeStamp = CMTimeMake(descriptor.decode_time.value,
                                       descriptor.decode_time.timescale),
      };
      const auto sample_size = bytes->size();
      if (status == noErr) {
        status = CMSampleBufferCreateReady(
            kCFAllocatorDefault, block, format_description_, 1, 1, &timing, 1,
            &sample_size, &sample);
      }
      if (status == noErr && sample != nullptr) {
        const auto attachments =
            CMSampleBufferGetSampleAttachmentsArray(sample, true);
        if (attachments != nullptr && CFArrayGetCount(attachments) > 0) {
          const auto attachment = static_cast<CFMutableDictionaryRef>(
              const_cast<void*>(CFArrayGetValueAtIndex(attachments, 0)));
          CFDictionarySetValue(attachment, kCMSampleAttachmentKey_NotSync,
                               descriptor.sync_sample ? kCFBooleanFalse
                                                      : kCFBooleanTrue);
        }
        contexts.push_back(std::make_unique<SampleCallbackContext>(
            SampleCallbackContext{
                .sequence = &callback_context_,
                .request = HardwareDecodeRequest{
                    .source_path = source_.source_path,
                    .expected_profile = source_.expected_profile,
                    .source_frame_index = descriptor.source_frame_index,
                    .packet_timing = descriptor.timing,
                },
                .lease_id = next_lease_id_.fetch_add(1),
                .retain_surface = retained_source_frames.contains(
                    descriptor.source_frame_index),
            }));
        VTDecodeInfoFlags flags = 0;
        counter_state_->increment(&MediaPathCounters::compressed_samples_submitted);
        status = VTDecompressionSessionDecodeFrame(
            session_, sample, kVTDecodeFrame_EnableAsynchronousDecompression,
            contexts.back().get(), &flags);
      }
      if (sample != nullptr) CFRelease(sample);
      if (block != nullptr) CFRelease(block);
      if (status != noErr) break;
    }
    const auto wait = VTDecompressionSessionWaitForAsynchronousFrames(session_);
    counter_state_->increment(&MediaPathCounters::hardware_decoder_flushes);
    if (status == noErr) status = wait;
    std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> produced;
    std::string diagnostic;
    OSStatus callback_status = noErr;
    {
      std::scoped_lock callback_lock(callback_context_.mutex);
      produced = std::move(callback_context_.surfaces);
      diagnostic = callback_context_.diagnostic;
      callback_status = callback_context_.status;
    }
    if (status != noErr || callback_status != noErr) {
      return diagnostic.empty() ? "VideoToolbox rejected a compressed playback sample"
                                : diagnostic;
    }
    for (auto& surface : produced) {
      const auto duplicate = std::any_of(
          resident_.begin(), resident_.end(), [&surface](const auto& existing) {
            return existing->info().source_frame_index ==
                   surface->info().source_frame_index;
          });
      if (!duplicate) resident_.push_back(std::move(surface));
    }
    return {};
  }

  HardwareVideoPlaybackSource source_;
  std::shared_ptr<CounterState> counter_state_;
  std::shared_ptr<GpuObservabilityService> observability_;
  std::atomic_uint64_t& next_lease_id_;
  std::ifstream input_;
  CMVideoFormatDescriptionRef format_description_{nullptr};
  VTDecompressionSessionRef session_{nullptr};
  std::shared_ptr<AppleDecodeLifetime> lifetime_;
  SequenceCallbackContext callback_context_;
  std::size_t next_decode_index_{0};
  std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> resident_;
  mutable std::mutex operation_mutex_;
};

class AppleHardwareVideoDecoder final : public runtime::media::HardwareVideoDecoder {
 public:
  AppleHardwareVideoDecoder(
      runtime::gpu::GpuDeviceService &gpu_device_service,
      std::shared_ptr<GpuObservabilityService> observability)
      : gpu_device_service_(gpu_device_service),
        counter_state_(std::make_shared<CounterState>()),
        observability_(std::move(observability)) {
    if (observability_ &&
        !observability_->observes(gpu_device_service_.identity())) {
      throw std::invalid_argument(
          "GPU observability and Apple decode must share one device identity");
    }
  }

  [[nodiscard]] HardwareDecodeResult decode(const HardwareDecodeRequest &request) override {
    HardwareDecodeSequenceRequest sequence_request{
        .source_path = request.source_path,
        .expected_profile = request.expected_profile,
    };
    if (request.valid()) {
      sequence_request.samples.push_back({
          .access_unit_index = request.source_frame_index,
          .source_frame_index = request.source_frame_index,
          .timing = request.packet_timing,
          .decode_time = request.packet_timing.presentation_time,
          .sync_sample = true,
      });
    }
    auto sequence = decode_sequence(sequence_request);
    if (!request.valid()) {
      sequence.code = "RFX-MEDIA-DECODE-REQUEST-INVALID";
      sequence.diagnostic = "The hardware decode request is incomplete or invalid";
    }
    return HardwareDecodeResult{
        .state = sequence.state,
        .hardware_decoder = sequence.hardware_decoder,
        .surface = sequence.queue ? sequence.queue->frame(0) : nullptr,
        .counters = sequence.counters,
        .code = sequence.admitted() ? "RFX-MEDIA-H264-HARDWARE-DECODED" : std::move(sequence.code),
        .diagnostic = sequence.admitted() ? "VideoToolbox decoded H.264 to a same-device NV12 "
                                            "Metal surface lease"
                                          : std::move(sequence.diagnostic),
    };
  }

  [[nodiscard]] HardwareDecodeSequenceResult decode_sequence(
      const HardwareDecodeSequenceRequest &request) override {
    std::scoped_lock operation_lock(operation_mutex_);
    counter_state_->increment(&MediaPathCounters::hardware_decoder_queries);
    if (!request.valid()) {
      return sequence_failure(DecodeState::invalid_request,
                              "RFX-MEDIA-DECODE-SEQUENCE-REQUEST-INVALID",
                              "The hardware decode sequence request is incomplete or invalid");
    }

    std::optional<runtime::gpu::BackendDeviceLease> gpu_lease;
    try {
      gpu_lease.emplace(gpu_device_service_.borrow());
    } catch (const std::exception &error) {
      return sequence_failure(DecodeState::device_unavailable, "RFX-MEDIA-GPU-NOT-READY",
                              std::string("The engine GPU device is not ready: ") + error.what());
    }
    if (!gpu_lease->valid() || gpu_lease->identity().backend != runtime::gpu::Backend::metal) {
      counter_state_->increment(&MediaPathCounters::cross_adapter_events);
      return sequence_failure(DecodeState::device_unavailable, "RFX-MEDIA-METAL-DEVICE-REQUIRED",
                              "Apple decode requires the engine Metal device");
    }
    if (!VTIsHardwareDecodeSupported(kCMVideoCodecType_H264)) {
      return sequence_failure(DecodeState::unsupported, "RFX-MEDIA-H264-HARDWARE-UNAVAILABLE",
                              "VideoToolbox reports no H.264 hardware decoder");
    }
    counter_state_->increment(&MediaPathCounters::hardware_decoder_admissions);

    std::ifstream input(request.source_path, std::ios::binary | std::ios::ate);
    if (!input) {
      return sequence_failure(DecodeState::source_open_failed, "RFX-MEDIA-SOURCE-OPEN",
                              "The bounded H.264 fixture could not be opened");
    }
    const auto end = input.tellg();
    if (end <= 0 || static_cast<std::uintmax_t>(end) > kMaximumFixtureBytes) {
      return sequence_failure(DecodeState::source_invalid, "RFX-MEDIA-SOURCE-BOUNDS",
                              "The bounded H.264 fixture has an invalid size");
    }
    std::vector<std::uint8_t> source(static_cast<std::size_t>(end));
    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char *>(source.data()),
               static_cast<std::streamsize>(source.size()));
    if (!input) {
      return sequence_failure(DecodeState::source_open_failed, "RFX-MEDIA-SOURCE-READ",
                              "The bounded H.264 fixture could not be read completely");
    }

    const auto parsed = parse_annex_b(source);
    const bool samples_in_bounds =
        std::all_of(request.samples.begin(), request.samples.end(), [&parsed](const auto &sample) {
          return sample.access_unit_index < parsed.access_units.size();
        });
    if (parsed.parameter_set_sequence.empty() || parsed.parameter_set_picture.empty() ||
        !samples_in_bounds) {
      return sequence_failure(DecodeState::source_invalid, "RFX-MEDIA-H264-ANNEXB",
                              "The fixture lacks SPS, PPS, AUD-framed access units, or a requested "
                              "access unit");
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
      return sequence_failure(DecodeState::source_invalid, "RFX-MEDIA-H264-FORMAT",
                              "CoreMedia rejected the H.264 parameter sets");
    }

    const auto dimensions = CMVideoFormatDescriptionGetDimensions(format_description);
    const bool format_matches =
        CMFormatDescriptionGetMediaSubType(format_description) == kCMVideoCodecType_H264 &&
        dimensions.width == static_cast<std::int32_t>(request.expected_profile.coded_width) &&
        dimensions.height == static_cast<std::int32_t>(request.expected_profile.coded_height);
    if (!format_matches) {
      CFRelease(format_description);
      return sequence_failure(DecodeState::source_invalid, "RFX-MEDIA-H264-PROFILE-MISMATCH",
                              "The H.264 format does not match the admitted codec/extent: " +
                                  std::to_string(dimensions.width) + "x" +
                                  std::to_string(dimensions.height));
    }

    std::shared_ptr<AppleDecodeLifetime> lifetime;
    try {
      lifetime = std::make_shared<AppleDecodeLifetime>(std::move(*gpu_lease));
    } catch (const std::exception &error) {
      CFRelease(format_description);
      return sequence_failure(DecodeState::native_surface_interop_failed,
                              "RFX-MEDIA-METAL-TEXTURE-CACHE", error.what());
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
    SequenceCallbackContext callback_context{
        .lifetime = lifetime,
        .counter_state = counter_state_,
        .observability = observability_,
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
      return sequence_failure(DecodeState::session_failed, "RFX-MEDIA-VT-SESSION",
                              "VideoToolbox could not create a hardware-required decompression "
                              "session");
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
      return sequence_failure(DecodeState::session_failed, "RFX-MEDIA-VT-HARDWARE-REQUIRED",
                              "VideoToolbox did not confirm the required hardware decoder");
    }
    counter_state_->increment(&MediaPathCounters::hardware_decoder_sessions);

    std::shared_ptr<GpuObservedFenceLease> decode_fence;
    auto decode_start = std::chrono::steady_clock::now();
    if (observability_) {
      try {
        decode_fence = std::make_shared<GpuObservedFenceLease>(
            observability_, GpuSubsystem::media,
            lifetime->device_identity().generation);
      } catch (const std::exception &error) {
        VTDecompressionSessionInvalidate(session);
        CFRelease(session);
        CFRelease(format_description);
        return sequence_failure(DecodeState::session_failed,
                                "RFX-MEDIA-GPU-OBS-FENCE", error.what());
      }
    }

    std::vector<std::unique_ptr<SampleCallbackContext>> sample_contexts;
    sample_contexts.reserve(request.samples.size());
    for (const auto &sample_descriptor : request.samples) {
      CMBlockBufferRef block_buffer = nullptr;
      CMSampleBufferRef sample_buffer = nullptr;
      try {
        const auto avcc_sample =
            make_avcc_sample(parsed.access_units[sample_descriptor.access_unit_index]);
        status = CMBlockBufferCreateWithMemoryBlock(
            kCFAllocatorDefault, nullptr, avcc_sample.size(), kCFAllocatorDefault, nullptr, 0,
            avcc_sample.size(), 0, &block_buffer);
        if (status == noErr) {
          status = CMBlockBufferReplaceDataBytes(avcc_sample.data(), block_buffer, 0,
                                                 avcc_sample.size());
        }
        const CMSampleTimingInfo timing{
            .duration = CMTimeMake(sample_descriptor.timing.duration.value,
                                   sample_descriptor.timing.duration.timescale),
            .presentationTimeStamp =
                CMTimeMake(sample_descriptor.timing.presentation_time.value,
                           sample_descriptor.timing.presentation_time.timescale),
            .decodeTimeStamp = CMTimeMake(sample_descriptor.decode_time.value,
                                          sample_descriptor.decode_time.timescale),
        };
        const auto sample_size = avcc_sample.size();
        if (status == noErr) {
          status = CMSampleBufferCreateReady(kCFAllocatorDefault, block_buffer, format_description,
                                             1, 1, &timing, 1, &sample_size, &sample_buffer);
        }
      } catch (const std::exception &error) {
        std::scoped_lock callback_lock(callback_context.mutex);
        callback_context.diagnostic = error.what();
        status = kVTVideoDecoderBadDataErr;
      }

      if (status == noErr && sample_buffer != nullptr) {
        const auto attachments = CMSampleBufferGetSampleAttachmentsArray(sample_buffer, true);
        if (attachments != nullptr && CFArrayGetCount(attachments) > 0) {
          const auto attachment = static_cast<CFMutableDictionaryRef>(
              const_cast<void *>(CFArrayGetValueAtIndex(attachments, 0)));
          CFDictionarySetValue(attachment, kCMSampleAttachmentKey_NotSync,
                               sample_descriptor.sync_sample ? kCFBooleanFalse : kCFBooleanTrue);
        }

        const auto lease_id = next_lease_id_.fetch_add(1);
        sample_contexts.push_back(std::make_unique<SampleCallbackContext>(SampleCallbackContext{
            .sequence = &callback_context,
            .request =
                HardwareDecodeRequest{
                    .source_path = request.source_path,
                    .expected_profile = request.expected_profile,
                    .source_frame_index = sample_descriptor.source_frame_index,
                    .packet_timing = sample_descriptor.timing,
                },
            .lease_id = lease_id,
        }));
        VTDecodeInfoFlags info_flags = 0;
        if (observability_) {
          const auto observation = observability_->record_submission(
              GpuSubsystem::media, observability_->issue_object_id(),
              lifetime->device_identity().generation);
          if (!observation.accepted) {
            status = kVTVideoDecoderBadDataErr;
          }
        }
        if (status == noErr) {
          counter_state_->increment(&MediaPathCounters::compressed_samples_submitted);
          status = VTDecompressionSessionDecodeFrame(
              session, sample_buffer,
              kVTDecodeFrame_EnableAsynchronousDecompression,
              sample_contexts.back().get(), &info_flags);
        }
      }

      if (sample_buffer != nullptr) {
        CFRelease(sample_buffer);
      }
      if (block_buffer != nullptr) {
        CFRelease(block_buffer);
      }
      if (status != noErr) {
        break;
      }
    }

    const auto wait_status = VTDecompressionSessionWaitForAsynchronousFrames(session);
    if (wait_status == noErr) {
      counter_state_->increment(&MediaPathCounters::hardware_decoder_flushes);
      if (decode_fence) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - decode_start);
        if (elapsed.count() < 0 ||
            !decode_fence->complete(static_cast<std::uint64_t>(elapsed.count()))) {
          status = kVTVideoDecoderBadDataErr;
        }
      }
    } else if (status == noErr) {
      status = wait_status;
    }
    VTDecompressionSessionInvalidate(session);
    CFRelease(session);
    CFRelease(format_description);

    std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> surfaces;
    std::string callback_diagnostic;
    OSStatus callback_status = noErr;
    {
      std::scoped_lock callback_lock(callback_context.mutex);
      surfaces = std::move(callback_context.surfaces);
      callback_diagnostic = callback_context.diagnostic;
      callback_status = callback_context.status;
    }
    if (status != noErr || callback_status != noErr || surfaces.size() != request.samples.size()) {
      return sequence_failure(DecodeState::decode_failed, "RFX-MEDIA-VT-SEQUENCE-DECODE",
                              callback_diagnostic.empty()
                                  ? "VideoToolbox rejected or omitted a compressed H.264 sequence "
                                    "sample"
                                  : std::move(callback_diagnostic));
    }

    std::shared_ptr<const DecodedSurfaceQueue> queue;
    try {
      queue = DecodedSurfaceQueue::create(std::move(surfaces));
    } catch (const std::exception &error) {
      return sequence_failure(DecodeState::decode_failed, "RFX-MEDIA-SURFACE-QUEUE", error.what());
    }
    counter_state_->increment(&MediaPathCounters::surface_queues_published);
    return HardwareDecodeSequenceResult{
        .state = DecodeState::decoded,
        .hardware_decoder = true,
        .queue = std::move(queue),
        .counters = counter_state_->snapshot(),
        .code = "RFX-MEDIA-H264-HARDWARE-SEQUENCE-DECODED",
        .diagnostic = "One hardware VideoToolbox session published an immutable, exact "
                      "PTS-indexed same-device NV12 surface queue",
    };
  }

  [[nodiscard]] std::unique_ptr<HardwareVideoPlaybackSession> open_playback(
      const HardwareVideoPlaybackSource& source) override {
    try {
      return std::make_unique<AppleHardwareVideoPlaybackSession>(
          source, gpu_device_service_, counter_state_, observability_,
          next_lease_id_);
    } catch (...) {
      return nullptr;
    }
  }

  [[nodiscard]] MediaPathCounters counters() const override { return counter_state_->snapshot(); }

 private:
  [[nodiscard]] HardwareDecodeSequenceResult sequence_failure(DecodeState state, std::string code,
                                                              std::string diagnostic) const {
    return HardwareDecodeSequenceResult{
        .state = state,
        .hardware_decoder = false,
        .queue = nullptr,
        .counters = counter_state_->snapshot(),
        .code = std::move(code),
        .diagnostic = std::move(diagnostic),
    };
  }

  runtime::gpu::GpuDeviceService &gpu_device_service_;
  std::shared_ptr<CounterState> counter_state_;
  std::shared_ptr<GpuObservabilityService> observability_;
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
    runtime::gpu::GpuDeviceService &gpu_device_service,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability) {
  return std::make_unique<AppleHardwareVideoDecoder>(gpu_device_service,
                                                     std::move(observability));
}

}  // namespace refusion::platform
