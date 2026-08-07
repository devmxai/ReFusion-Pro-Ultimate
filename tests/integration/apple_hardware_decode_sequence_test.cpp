#include <iostream>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <string>
#include <vector>

#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformMediaCapability.hpp"

namespace {

void require(const bool condition, const std::source_location location =
                                       std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error(
        "Apple hardware sequence decode requirement failed at line " +
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

  HardwareDecodeSequenceRequest request{
      .source_path = REFUSION_TEST_H264_FIXTURE_PATH,
      .expected_profile =
          {
              .coded_width = 320,
              .coded_height = 180,
          },
  };
  for (std::uint64_t index = 0; index < 8; ++index) {
    request.samples.push_back({
        .access_unit_index = index,
        .source_frame_index = index,
        .timing =
            {
                .presentation_time =
                    {
                        .value = static_cast<std::int64_t>(index),
                        .timescale = 30,
                    },
                .duration = {.value = 1, .timescale = 30},
            },
        .decode_time =
            {
                .value = static_cast<std::int64_t>(index),
                .timescale = 30,
            },
        .sync_sample = true,
    });
  }

  auto decoded = decoder->decode_sequence(request);
  if (!decoded.admitted()) {
    std::cerr << decoded.code << ": " << decoded.diagnostic << '\n';
  }
  require(decoded.admitted());
  require(decoded.code == "RFX-MEDIA-H264-HARDWARE-SEQUENCE-DECODED");
  require(decoded.queue != nullptr);
  require(decoded.queue->size() == 8);
  require(decoded.queue->device_identity().backend == gpu->identity().backend);
  require(decoded.queue->device_identity().adapter_id ==
          gpu->identity().adapter_id);
  require(decoded.queue->device_identity().generation ==
          gpu->identity().generation);
  for (std::size_t index = 0; index < decoded.queue->size(); ++index) {
    const auto& info = decoded.queue->frame(index)->info();
    require(info.source_frame_index == index);
    require(info.timing.presentation_time.value ==
            static_cast<std::int64_t>(index));
    require(info.timing.presentation_time.timescale == 30);
    require(info.device.backend == gpu->identity().backend);
    require(info.device.adapter_id == gpu->identity().adapter_id);
    require(info.device.generation == gpu->identity().generation);
  }

  require(decoded.queue->select_at({.value = -1, .timescale = 30}) == nullptr);
  require(decoded.queue
              ->select_at({.value = 33'333'333, .timescale = 1'000'000'000})
              ->info()
              .source_frame_index == 0);
  require(decoded.queue
              ->select_at({.value = 33'333'334, .timescale = 1'000'000'000})
              ->info()
              .source_frame_index == 1);
  require(decoded.queue
              ->select_at({.value = 100'000'000, .timescale = 1'000'000'000})
              ->info()
              .source_frame_index == 3);
  require(decoded.queue->select_at({.value = 1, .timescale = 1})
              ->info()
              .source_frame_index == 7);

  const auto counters = decoded.counters;
  require(counters.hardware_decoder_queries == 1);
  require(counters.hardware_decoder_admissions == 1);
  require(counters.hardware_decoder_sessions == 1);
  require(counters.compressed_samples_submitted == 8);
  require(counters.hardware_frames_decoded == 8);
  require(counters.hardware_decoder_flushes == 1);
  require(counters.native_surface_allocations == 8);
  require(counters.native_surface_plane_bindings == 16);
  require(counters.native_surface_leases_issued == 8);
  require(counters.native_surface_leases_released == 0);
  require(counters.surface_queues_published == 1);
  require(counters.strict_path_clean());

  std::vector<std::weak_ptr<const NativeVideoSurfaceLease>> weak_surfaces;
  for (std::size_t index = 0; index < decoded.queue->size(); ++index) {
    weak_surfaces.push_back(decoded.queue->frame(index));
  }
  decoded.queue.reset();
  for (const auto& weak_surface : weak_surfaces) {
    require(weak_surface.expired());
  }
  const auto released = decoder->counters();
  require(released.native_surface_leases_released == 8);
  require(released.strict_path_clean());

  std::cout << "{\"hardware_decoder_sessions\":"
            << released.hardware_decoder_sessions
            << ",\"compressed_samples_submitted\":"
            << released.compressed_samples_submitted
            << ",\"hardware_frames_decoded\":"
            << released.hardware_frames_decoded
            << ",\"surface_queues_published\":"
            << released.surface_queues_published
            << ",\"native_surface_leases_released\":"
            << released.native_surface_leases_released << "}\n";
}
