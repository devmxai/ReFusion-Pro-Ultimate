#include "refusion/platform/PlatformMediaCapability.hpp"

#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <VideoToolbox/VideoToolbox.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace refusion::platform {
namespace {

using runtime::media::CapabilityState;
using runtime::media::DecodeCapability;
using runtime::media::MediaPathCounters;
using runtime::media::StrictDecodeProfile;

class AppleMediaCapabilityProbe final
    : public runtime::media::MediaCapabilityProbe {
 public:
  explicit AppleMediaCapabilityProbe(
      runtime::gpu::GpuDeviceService& gpu_device_service)
      : gpu_device_service_(gpu_device_service) {}

  [[nodiscard]] DecodeCapability probe(
      const StrictDecodeProfile& profile) override {
    std::scoped_lock lock(mutex_);
    ++counters_.hardware_decoder_queries;

    if (!profile.valid()) {
      return failure(CapabilityState::invalid_request,
                     "RFX-MEDIA-PROFILE-INVALID",
                     "The strict decode profile has an invalid coded extent");
    }

    std::optional<runtime::gpu::BackendDeviceLease> gpu_lease;
    try {
      gpu_lease.emplace(gpu_device_service_.borrow());
    } catch (const std::exception& error) {
      return failure(CapabilityState::device_unavailable,
                     "RFX-MEDIA-GPU-NOT-READY",
                     std::string("The engine GPU device is not ready: ") +
                         error.what());
    }
    if (!gpu_lease->valid() ||
        gpu_lease->identity().backend != runtime::gpu::Backend::metal) {
      ++counters_.cross_adapter_events;
      return failure(CapabilityState::device_unavailable,
                     "RFX-MEDIA-METAL-DEVICE-REQUIRED",
                     "Apple media admission requires the engine Metal device");
    }

    if (!VTIsHardwareDecodeSupported(kCMVideoCodecType_H264)) {
      return failure(CapabilityState::unsupported,
                     "RFX-MEDIA-H264-HARDWARE-UNAVAILABLE",
                     "VideoToolbox reports no H.264 hardware decoder");
    }
    ++counters_.hardware_decoder_admissions;

    id<MTLDevice> metal_device = (__bridge id<MTLDevice>)(const_cast<void *>(
        gpu_lease->backend_private_device()));
    if (metal_device == nil ||
        static_cast<std::uint64_t>(metal_device.registryID) !=
            gpu_lease->identity().adapter_id) {
      ++counters_.cross_adapter_events;
      return failure(CapabilityState::device_unavailable,
                     "RFX-MEDIA-METAL-IDENTITY-MISMATCH",
                     "The media probe did not receive the admitted Metal device");
    }

    CVMetalTextureCacheRef texture_cache = nullptr;
    CVReturn status = CVMetalTextureCacheCreate(
        kCFAllocatorDefault, nullptr, metal_device, nullptr, &texture_cache);
    if (status != kCVReturnSuccess || texture_cache == nullptr) {
      return failure(CapabilityState::native_surface_interop_failed,
                     "RFX-MEDIA-METAL-TEXTURE-CACHE",
                     "CoreVideo could not create a Metal texture cache");
    }

    NSDictionary* attributes = @{
      (__bridge NSString*)kCVPixelBufferMetalCompatibilityKey : @YES,
      (__bridge NSString*)kCVPixelBufferIOSurfacePropertiesKey : @{},
    };
    CVPixelBufferRef pixel_buffer = nullptr;
    status = CVPixelBufferCreate(
        kCFAllocatorDefault,
        static_cast<size_t>(profile.coded_width),
        static_cast<size_t>(profile.coded_height),
        kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
        (__bridge CFDictionaryRef)attributes,
        &pixel_buffer);
    if (status != kCVReturnSuccess || pixel_buffer == nullptr) {
      CFRelease(texture_cache);
      return failure(CapabilityState::native_surface_interop_failed,
                     "RFX-MEDIA-NATIVE-SURFACE-ALLOCATE",
                     "CoreVideo could not allocate the strict native surface");
    }
    ++counters_.native_surface_allocations;

    CVMetalTextureRef luma_view = nullptr;
    CVMetalTextureRef chroma_view = nullptr;
    const size_t luma_width = CVPixelBufferGetWidthOfPlane(pixel_buffer, 0);
    const size_t luma_height = CVPixelBufferGetHeightOfPlane(pixel_buffer, 0);
    const size_t chroma_width = CVPixelBufferGetWidthOfPlane(pixel_buffer, 1);
    const size_t chroma_height = CVPixelBufferGetHeightOfPlane(pixel_buffer, 1);

    const CVReturn luma_status = CVMetalTextureCacheCreateTextureFromImage(
        kCFAllocatorDefault, texture_cache, pixel_buffer, nullptr,
        MTLPixelFormatR8Unorm, luma_width, luma_height, 0, &luma_view);
    const CVReturn chroma_status = CVMetalTextureCacheCreateTextureFromImage(
        kCFAllocatorDefault, texture_cache, pixel_buffer, nullptr,
        MTLPixelFormatRG8Unorm, chroma_width, chroma_height, 1, &chroma_view);

    id<MTLTexture> luma_texture = luma_view == nullptr
                                      ? nil
                                      : CVMetalTextureGetTexture(luma_view);
    id<MTLTexture> chroma_texture = chroma_view == nullptr
                                        ? nil
                                        : CVMetalTextureGetTexture(chroma_view);
    const bool planes_valid =
        luma_status == kCVReturnSuccess && chroma_status == kCVReturnSuccess &&
        luma_texture != nil && chroma_texture != nil &&
        static_cast<std::uint64_t>(luma_texture.device.registryID) ==
            gpu_lease->identity().adapter_id &&
        static_cast<std::uint64_t>(chroma_texture.device.registryID) ==
            gpu_lease->identity().adapter_id;
    const bool adapter_mismatch =
        luma_texture != nil && chroma_texture != nil &&
        (static_cast<std::uint64_t>(luma_texture.device.registryID) !=
             gpu_lease->identity().adapter_id ||
         static_cast<std::uint64_t>(chroma_texture.device.registryID) !=
             gpu_lease->identity().adapter_id);

    if (luma_view != nullptr) {
      CFRelease(luma_view);
    }
    if (chroma_view != nullptr) {
      CFRelease(chroma_view);
    }
    CFRelease(pixel_buffer);
    CFRelease(texture_cache);

    if (!planes_valid) {
      if (adapter_mismatch) {
        ++counters_.cross_adapter_events;
      }
      return failure(CapabilityState::native_surface_interop_failed,
                     "RFX-MEDIA-NATIVE-SURFACE-BIND",
                     "The native surface planes could not bind to the admitted Metal device");
    }
    counters_.native_surface_plane_bindings += 2;

    return DecodeCapability{
        .state = CapabilityState::admitted,
        .hardware_decoder = true,
        .native_gpu_surface = true,
        .native_plane_count = 2,
        .device = gpu_lease->identity(),
        .counters = counters_,
        .code = "RFX-MEDIA-H264-NV12-METAL-ADMITTED",
        .diagnostic =
            "H.264 hardware decode and same-device NV12 Metal surfaces are available",
    };
  }

  [[nodiscard]] MediaPathCounters counters() const override {
    std::scoped_lock lock(mutex_);
    return counters_;
  }

 private:
  [[nodiscard]] DecodeCapability failure(CapabilityState state,
                                         std::string code,
                                         std::string diagnostic) const {
    return DecodeCapability{
        .state = state,
        .hardware_decoder = false,
        .native_gpu_surface = false,
        .native_plane_count = 0,
        .device = std::nullopt,
        .counters = counters_,
        .code = std::move(code),
        .diagnostic = std::move(diagnostic),
    };
  }

  runtime::gpu::GpuDeviceService& gpu_device_service_;
  mutable std::mutex mutex_;
  MediaPathCounters counters_;
};

}  // namespace

std::unique_ptr<runtime::media::MediaCapabilityProbe>
create_platform_media_capability_probe(
    runtime::gpu::GpuDeviceService& gpu_device_service) {
  return std::make_unique<AppleMediaCapabilityProbe>(gpu_device_service);
}

}  // namespace refusion::platform
