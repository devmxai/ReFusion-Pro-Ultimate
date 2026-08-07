#include <iostream>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <string>

#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformMediaCapability.hpp"

namespace {

void require(const bool condition, const std::source_location location =
                                       std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error(
        std::string("Apple hardware decode requirement failed at line ") +
        std::to_string(location.line()));
  }
}

}  // namespace

int main() {
  using namespace refusion::runtime::media;

  auto gpu = refusion::platform::create_platform_gpu_device_service();
  require(gpu != nullptr);
  auto decoder =
      refusion::platform::create_platform_hardware_video_decoder(*gpu);
  require(decoder != nullptr);

  const HardwareDecodeRequest request{
      .source_path = REFUSION_TEST_H264_FIXTURE_PATH,
      .expected_profile =
          {
              .coded_width = 320,
              .coded_height = 180,
          },
      .source_frame_index = 3,
      .packet_timing =
          {
              .presentation_time = {.value = 3, .timescale = 30},
              .duration = {.value = 1, .timescale = 30},
          },
  };
  auto decoded = decoder->decode(request);
  if (!decoded.admitted()) {
    std::cerr << decoded.code << ": " << decoded.diagnostic << '\n';
  }
  require(decoded.admitted());
  require(decoded.code == "RFX-MEDIA-H264-HARDWARE-DECODED");
  require(decoded.surface != nullptr);
  const auto info = decoded.surface->info();
  require(info.valid());
  require(info.source_frame_index == 3);
  require(info.profile.coded_width == 320);
  require(info.profile.coded_height == 180);
  require(info.profile.pixel_format == VideoPixelFormat::nv12_8bit_video_range);
  require(info.profile.color.primaries == ColorPrimaries::bt709);
  require(!info.profile.color.full_range);
  require(info.timing.presentation_time.value == 3);
  require(info.timing.presentation_time.timescale == 30);
  require(info.timing.duration.value == 1);
  require(info.timing.duration.timescale == 30);
  require(info.device.backend == refusion::runtime::gpu::Backend::metal);
  require(info.device.adapter_id == gpu->identity().adapter_id);
  require(info.device.generation == gpu->identity().generation);
  require(info.plane_count == 2);

  require(decoded.counters.hardware_decoder_queries == 1);
  require(decoded.counters.hardware_decoder_admissions == 1);
  require(decoded.counters.hardware_decoder_sessions == 1);
  require(decoded.counters.compressed_samples_submitted == 1);
  require(decoded.counters.hardware_frames_decoded == 1);
  require(decoded.counters.native_surface_allocations == 1);
  require(decoded.counters.native_surface_plane_bindings == 2);
  require(decoded.counters.native_surface_leases_issued == 1);
  require(decoded.counters.native_surface_leases_released == 0);
  require(decoded.counters.strict_path_clean());

  std::weak_ptr<const NativeVideoSurfaceLease> weak_surface = decoded.surface;
  decoded.surface.reset();
  require(weak_surface.expired());
  const auto after_release = decoder->counters();
  require(after_release.native_surface_leases_released == 1);
  require(after_release.strict_path_clean());

  const auto invalid = decoder->decode({});
  require(!invalid.admitted());
  require(invalid.state == DecodeState::invalid_request);
  require(invalid.code == "RFX-MEDIA-DECODE-REQUEST-INVALID");
  require(invalid.counters.strict_path_clean());

  const auto suspended_health = gpu->handle_lifecycle_event(
      refusion::runtime::gpu::DeviceLifecycleEvent::will_sleep);
  require(suspended_health.status ==
          refusion::runtime::gpu::DeviceStatus::suspended);
  const auto suspended = decoder->decode(request);
  require(!suspended.admitted());
  require(suspended.state == DecodeState::device_unavailable);
  require(suspended.code == "RFX-MEDIA-GPU-NOT-READY");
  require(suspended.counters.strict_path_clean());
  require(gpu->handle_lifecycle_event(
                 refusion::runtime::gpu::DeviceLifecycleEvent::did_wake)
              .ready());

  const auto counters = decoder->counters();
  std::cout << "{\"profile\":\"h264-high-nv12-video-range-bt709\","
            << "\"source_frame_index\":" << info.source_frame_index << ','
            << "\"pts\":{\"value\":" << info.timing.presentation_time.value
            << ",\"timescale\":" << info.timing.presentation_time.timescale
            << "},\"duration\":{\"value\":" << info.timing.duration.value
            << ",\"timescale\":" << info.timing.duration.timescale << "},"
            << "\"adapter_id\":" << info.device.adapter_id << ','
            << "\"device_generation\":" << info.device.generation << ','
            << "\"hardware_decoder_sessions\":"
            << counters.hardware_decoder_sessions << ','
            << "\"compressed_samples_submitted\":"
            << counters.compressed_samples_submitted << ','
            << "\"hardware_frames_decoded\":"
            << counters.hardware_frames_decoded << ','
            << "\"native_surface_plane_bindings\":"
            << counters.native_surface_plane_bindings << ','
            << "\"native_surface_leases_issued\":"
            << counters.native_surface_leases_issued << ','
            << "\"native_surface_leases_released\":"
            << counters.native_surface_leases_released << ','
            << "\"software_decoder_selections\":"
            << counters.software_decoder_selections << ','
            << "\"cpu_pixel_maps\":" << counters.cpu_pixel_maps << ','
            << "\"cpu_pixel_conversions\":" << counters.cpu_pixel_conversions
            << ',' << "\"cpu_pixel_uploads\":" << counters.cpu_pixel_uploads
            << ',' << "\"gpu_readbacks\":" << counters.gpu_readbacks << ','
            << "\"cross_adapter_events\":" << counters.cross_adapter_events
            << ',' << "\"unattributed_gpu_copies\":"
            << counters.unattributed_gpu_copies << "}\n";
}
