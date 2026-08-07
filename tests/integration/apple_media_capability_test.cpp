#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformMediaCapability.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("Apple media capability requirement failed");
  }
}

}  // namespace

int main() {
  auto gpu = refusion::platform::create_platform_gpu_device_service();
  require(gpu != nullptr);
  auto media = refusion::platform::create_platform_media_capability_probe(*gpu);
  require(media != nullptr);

  const refusion::runtime::media::StrictDecodeProfile profile{
      .coded_width = 1920,
      .coded_height = 1080,
  };
  const auto capability = media->probe(profile);
  require(capability.admitted());
  require(capability.code == "RFX-MEDIA-H264-NV12-METAL-ADMITTED");
  require(capability.device.has_value());
  require(capability.device->adapter_id == gpu->identity().adapter_id);
  require(capability.device->generation == gpu->identity().generation);
  require(capability.native_plane_count == 2);
  require(capability.counters.hardware_decoder_queries == 1);
  require(capability.counters.hardware_decoder_admissions == 1);
  require(capability.counters.native_surface_allocations == 1);
  require(capability.counters.native_surface_plane_bindings == 2);
  require(capability.counters.strict_path_clean());

  const auto invalid = media->probe({});
  require(!invalid.admitted());
  require(invalid.state ==
          refusion::runtime::media::CapabilityState::invalid_request);
  require(invalid.code == "RFX-MEDIA-PROFILE-INVALID");
  require(invalid.counters.strict_path_clean());

  const auto suspended_health = gpu->handle_lifecycle_event(
      refusion::runtime::gpu::DeviceLifecycleEvent::will_sleep);
  require(suspended_health.status ==
          refusion::runtime::gpu::DeviceStatus::suspended);
  const auto suspended = media->probe(profile);
  require(!suspended.admitted());
  require(suspended.state ==
          refusion::runtime::media::CapabilityState::device_unavailable);
  require(suspended.code == "RFX-MEDIA-GPU-NOT-READY");
  require(suspended.counters.strict_path_clean());
  const auto resumed_health = gpu->handle_lifecycle_event(
      refusion::runtime::gpu::DeviceLifecycleEvent::did_wake);
  require(resumed_health.ready());

  const auto counters = media->counters();
  require(counters.hardware_decoder_queries == 3);
  require(counters.hardware_decoder_admissions == 1);
  require(counters.software_decoder_selections == 0);
  require(counters.cpu_pixel_maps == 0);
  require(counters.cpu_pixel_conversions == 0);
  require(counters.cpu_pixel_uploads == 0);
  require(counters.gpu_readbacks == 0);
  require(counters.cross_adapter_events == 0);
  require(counters.unattributed_gpu_copies == 0);

  std::cout
      << "{\"profile\":\"h264-avc-8bit-420-sdr-bt709\"," 
      << "\"extent\":\"1920x1080\"," 
      << "\"hardware_decoder_admissions\":"
      << counters.hardware_decoder_admissions << ','
      << "\"native_surface_allocations\":"
      << counters.native_surface_allocations << ','
      << "\"native_surface_plane_bindings\":"
      << counters.native_surface_plane_bindings << ','
      << "\"software_decoder_selections\":"
      << counters.software_decoder_selections << ','
      << "\"cpu_pixel_maps\":" << counters.cpu_pixel_maps << ','
      << "\"cpu_pixel_conversions\":" << counters.cpu_pixel_conversions << ','
      << "\"cpu_pixel_uploads\":" << counters.cpu_pixel_uploads << ','
      << "\"gpu_readbacks\":" << counters.gpu_readbacks << ','
      << "\"cross_adapter_events\":" << counters.cross_adapter_events << ','
      << "\"unattributed_gpu_copies\":"
      << counters.unattributed_gpu_copies << "}\n";
}
