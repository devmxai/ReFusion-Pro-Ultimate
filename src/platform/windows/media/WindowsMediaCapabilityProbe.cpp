#include "refusion/platform/PlatformMediaCapability.hpp"

#include <memory>

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
            "The Windows hardware-media surface probe requires G1-WP04 device evidence",
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

}  // namespace refusion::platform
