#include <memory>

#include "refusion/platform/PlatformMediaCapability.hpp"

namespace refusion::platform {
namespace {

class WindowsMediaCapabilityProbe final
    : public runtime::media::MediaCapabilityProbe {
 public:
  explicit WindowsMediaCapabilityProbe(runtime::gpu::GpuDeviceService&) {}

  [[nodiscard]] runtime::media::DecodeCapability probe(
      const runtime::media::StrictDecodeProfile&) override {
    ++counters_.hardware_decoder_queries;
    return runtime::media::DecodeCapability{
        .state = runtime::media::CapabilityState::unsupported,
        .hardware_decoder = false,
        .native_gpu_surface = false,
        .native_plane_count = 0,
        .device = std::nullopt,
        .counters = counters_,
        .code = "RFX-MEDIA-WINDOWS-NOT-QUALIFIED",
        .diagnostic =
            "The Windows hardware-media surface probe requires "
            "G1-WP04 device evidence",
    };
  }

  [[nodiscard]] runtime::media::MediaPathCounters counters() const override {
    return counters_;
  }

 private:
  runtime::media::MediaPathCounters counters_;
};

class WindowsHardwareVideoDecoder final
    : public runtime::media::HardwareVideoDecoder {
 public:
  explicit WindowsHardwareVideoDecoder(runtime::gpu::GpuDeviceService&) {}

  [[nodiscard]] runtime::media::HardwareDecodeResult decode(
      const runtime::media::HardwareDecodeRequest&) override {
    ++counters_.hardware_decoder_queries;
    return runtime::media::HardwareDecodeResult{
        .state = runtime::media::DecodeState::unsupported,
        .hardware_decoder = false,
        .surface = nullptr,
        .counters = counters_,
        .code = "RFX-MEDIA-WINDOWS-DECODE-NOT-QUALIFIED",
        .diagnostic =
            "The Windows hardware decoder remains fail-closed until "
            "G1-WP04 physical evidence",
    };
  }

  [[nodiscard]] runtime::media::HardwareDecodeSequenceResult decode_sequence(
      const runtime::media::HardwareDecodeSequenceRequest&) override {
    ++counters_.hardware_decoder_queries;
    return runtime::media::HardwareDecodeSequenceResult{
        .state = runtime::media::DecodeState::unsupported,
        .hardware_decoder = false,
        .queue = nullptr,
        .counters = counters_,
        .code = "RFX-MEDIA-WINDOWS-SEQUENCE-NOT-QUALIFIED",
        .diagnostic =
            "The Windows hardware decode queue remains fail-closed until "
            "G1-WP04 physical evidence",
    };
  }

  [[nodiscard]] runtime::media::MediaPathCounters counters() const override {
    return counters_;
  }

 private:
  runtime::media::MediaPathCounters counters_;
};

}  // namespace

std::unique_ptr<runtime::media::MediaCapabilityProbe>
create_platform_media_capability_probe(
    runtime::gpu::GpuDeviceService& gpu_device_service) {
  return std::make_unique<WindowsMediaCapabilityProbe>(gpu_device_service);
}

std::unique_ptr<runtime::media::HardwareVideoDecoder>
create_platform_hardware_video_decoder(
    runtime::gpu::GpuDeviceService& gpu_device_service,
    std::shared_ptr<runtime::gpu::GpuObservabilityService>) {
  return std::make_unique<WindowsHardwareVideoDecoder>(gpu_device_service);
}

}  // namespace refusion::platform
