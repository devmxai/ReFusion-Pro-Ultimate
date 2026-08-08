#include <iostream>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "LongGopMediaFixture.hpp"
#include "refusion/core/ProjectClock.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformMediaCapability.hpp"
#include "refusion/runtime/media/HardwareVideoDecodeScheduler.hpp"

namespace {

using namespace refusion;
using namespace refusion::runtime::media;

void require(const bool condition, const std::source_location location =
                                       std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error(
        "Apple hardware seek decode requirement failed at line " +
        std::to_string(location.line()));
  }
}

[[nodiscard]] HardwareSeekDecodeRequest request_at(
    const runtime::gpu::DeviceIdentity& device,
    const std::uint64_t epoch, const std::int64_t pts,
    const std::size_t maximum_residency) {
  return {
      .source_path = REFUSION_TEST_H264_SEEK_FIXTURE_PATH,
      .expected_profile = {.coded_width = 320, .coded_height = 180},
      .samples = refusion_test::long_gop_samples(),
      .target_presentation_time = {.value = pts, .timescale = 30'000},
      .stamp = {.transport_epoch_id = epoch, .device = device},
      .residency = {.maximum_surface_count = maximum_residency},
  };
}

}  // namespace

int main() {
  auto gpu = platform::create_platform_gpu_device_service();
  require(gpu != nullptr);
  auto decoder = platform::create_platform_hardware_video_decoder(*gpu);
  require(decoder != nullptr);
  HardwareVideoDecodeScheduler scheduler(*decoder);

  core::ProjectClock clock({
      .duration_ns = 10'000'000'000,
      .frame_rate = {.numerator = 30, .denominator = 1},
      .loop = false,
  });
  const auto started = clock.start({.nanoseconds = 1, .source_generation = 1});
  require(started.accepted);
  require(started.snapshot.epoch_id == 1);
  const auto initial_device = gpu->identity();

  auto long_gop = scheduler.decode_seek(request_at(
      initial_device, started.snapshot.epoch_id, 99'500, 3));
  if (!long_gop.admitted()) {
    std::cerr << long_gop.code << ": " << long_gop.diagnostic << '\n';
  }
  require(long_gop.admitted());
  require(long_gop.target_source_frame_index == 9);
  require(long_gop.dependency_start_access_unit == 0);
  require(long_gop.target_access_unit == 10);
  require(long_gop.telemetry.dependency_samples_submitted == 11);
  require(long_gop.telemetry.peak_surface_residency == 11);
  require(long_gop.telemetry.surface_residency_limit == 3);
  require(long_gop.telemetry.surfaces_retained == 2);
  require(long_gop.telemetry.surfaces_evicted_for_backpressure == 9);
  require(long_gop.counters.native_surface_leases_released == 9);
  auto published = scheduler.publish_if_current(
      std::move(long_gop),
      {.transport_epoch_id = started.snapshot.epoch_id,
       .device = initial_device});
  require(published.accepted);
  require(published.selected_surface->info().source_frame_index == 9);
  require(published.selected_surface->info().timing.presentation_time.value ==
          99'000);
  require(published.counters.strict_path_clean());
  published.selected_surface.reset();
  published.queue.reset();
  require(decoder->counters().native_surface_leases_released == 11);

  const auto seeked = clock.seek_to_frame(
      60, {.nanoseconds = 2, .source_generation = 1});
  require(seeked.accepted);
  require(seeked.snapshot.epoch_id == 2);
  auto vfr = scheduler.decode_seek(
      request_at(initial_device, seeked.snapshot.epoch_id, 102'500, 2));
  require(vfr.admitted());
  require(vfr.target_source_frame_index == 12);
  require(vfr.dependency_start_access_unit == 11);
  require(vfr.target_access_unit == 13);
  require(vfr.telemetry.dependency_samples_submitted == 3);
  require(vfr.telemetry.surfaces_retained == 2);

  const auto superseding_seek = clock.seek_to_frame(
      90, {.nanoseconds = 3, .source_generation = 1});
  require(superseding_seek.accepted);
  require(superseding_seek.snapshot.epoch_id == 3);
  auto stale_epoch = scheduler.publish_if_current(
      std::move(vfr),
      {.transport_epoch_id = superseding_seek.snapshot.epoch_id,
       .device = initial_device});
  require(!stale_epoch.accepted);
  require(stale_epoch.code == "RFX-MEDIA-SEEK-STALE-EPOCH");
  require(stale_epoch.telemetry.stale_epoch_rejections == 1);
  require(stale_epoch.queue == nullptr);

  auto stale_generation_candidate = scheduler.decode_seek(request_at(
      initial_device, superseding_seek.snapshot.epoch_id, 104'500, 2));
  require(stale_generation_candidate.admitted());
  require(stale_generation_candidate.target_source_frame_index == 14);
  require(stale_generation_candidate.dependency_start_access_unit == 11);
  require(stale_generation_candidate.target_access_unit == 15);
  const auto lost = gpu->report_device_loss(
      "injected after decode and before publication");
  require(lost.identity.generation == initial_device.generation + 1);
  auto stale_device = scheduler.publish_if_current(
      std::move(stale_generation_candidate),
      {.transport_epoch_id = superseding_seek.snapshot.epoch_id,
       .device = lost.identity});
  require(!stale_device.accepted);
  require(stale_device.code ==
          "RFX-MEDIA-SEEK-STALE-DEVICE-GENERATION");
  require(stale_device.telemetry.stale_device_generation_rejections == 1);
  require(stale_device.queue == nullptr);

  const auto counters = decoder->counters();
  require(counters.hardware_decoder_sessions == 3);
  require(counters.compressed_samples_submitted == 19);
  require(counters.hardware_frames_decoded == 19);
  require(counters.hardware_decoder_flushes == 3);
  require(counters.surface_queues_published == 3);
  require(counters.native_surface_allocations == 19);
  require(counters.native_surface_plane_bindings == 38);
  require(counters.native_surface_leases_issued == 19);
  require(counters.native_surface_leases_released == 19);
  require(counters.strict_path_clean());

  std::cout << "{\"hardware_decoder_sessions\":"
            << counters.hardware_decoder_sessions
            << ",\"compressed_samples_submitted\":"
            << counters.compressed_samples_submitted
            << ",\"hardware_frames_decoded\":"
            << counters.hardware_frames_decoded
            << ",\"hardware_decoder_flushes\":"
            << counters.hardware_decoder_flushes
            << ",\"native_surface_leases_released\":"
            << counters.native_surface_leases_released
            << ",\"stale_epoch_rejections\":"
            << stale_epoch.telemetry.stale_epoch_rejections
            << ",\"stale_device_generation_rejections\":"
            << stale_device.telemetry.stale_device_generation_rejections
            << "}\n";
}
