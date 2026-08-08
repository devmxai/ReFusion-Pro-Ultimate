#include <chrono>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <string>

#include "refusion/runtime/gpu/GpuObservability.hpp"

namespace {

using namespace refusion::runtime::gpu;

void require(const bool condition, const std::source_location location =
                                       std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error("GPU observability requirement failed at line " +
                             std::to_string(location.line()));
  }
}

[[nodiscard]] DeviceIdentity device(const std::uint64_t generation = 1) {
  return {
      .backend = Backend::metal,
      .adapter_name = "portable-observability-test",
      .adapter_id = 71,
      .generation = generation,
  };
}

void prove_clean_attributed_trace() {
  auto service = std::make_shared<GpuObservabilityService>(
      device(), 64,
      GpuQualificationBudget{
          .device_tier_id = "portable-observability-tier",
          .maximum_peak_resident_bytes = 1'024,
          .maximum_fence_latency_ns = 1'000,
          .maximum_thermal_state = GpuThermalState::serious,
      });
  require(service->observes(device()));
  require(!service->observes(device(2)));
  {
    GpuObservedResourceLease context(
        service, GpuSubsystem::skia, GpuResourceKind::render_context, 1, 100);
    GpuObservedResourceLease surface(
        service, GpuSubsystem::media,
        GpuResourceKind::native_video_surface, 1, 200);
    require(service
                ->record_submission(GpuSubsystem::skia,
                                    service->issue_object_id(), 1)
                .accepted);
    require(service
                ->record_copy(GpuSubsystem::skia,
                              service->issue_object_id(), 1, 64, false)
                .accepted);
    auto fence = std::make_shared<GpuObservedFenceLease>(
        service, GpuSubsystem::presentation, 1);
    require(fence->complete(500));
  }
  require(service->wait_until_quiescent(std::chrono::milliseconds(1)));
  require(service
              ->record_thermal_sample(GpuSubsystem::device,
                                      GpuThermalState::fair)
              .accepted);
  auto snapshot = service->snapshot();
  require(snapshot.strict_path_clean());
  require(snapshot.quiescent());
  require(snapshot.resource_leases_acquired == 2);
  require(snapshot.resource_leases_released == 2);
  require(snapshot.peak_resident_bytes == 300);
  require(snapshot.attributed_submissions == 1);
  require(snapshot.attributed_copies == 1);
  require(snapshot.attributed_copy_bytes == 64);
  require(snapshot.fences_issued == 1);
  require(snapshot.fences_completed == 1);
  require(snapshot.fence_latency_samples == 1);
  require(snapshot.fence_latency_total_ns == 500);
  require(snapshot.fence_latency_max_ns == 500);
  require(snapshot.thermal_samples == 1);
  require(snapshot.maximum_observed_thermal_state == GpuThermalState::fair);
  require(snapshot.within_qualification_budget());

  require(service->observe_device_loss(device(2)).accepted);
  require(service
              ->reject_stale_generation(GpuSubsystem::skia, 1)
              .accepted);
  {
    GpuObservedResourceLease replacement(
        service, GpuSubsystem::presentation, GpuResourceKind::drawable, 2, 50);
  }
  snapshot = service->snapshot();
  require(snapshot.device.generation == 2);
  require(snapshot.device_loss_events == 1);
  require(snapshot.stale_generation_rejections == 1);
  require(snapshot.stale_generation_resources_accepted == 0);
  require(snapshot.strict_path_clean());
  require(snapshot.quiescent());
}

void prove_fail_closed_contracts() {
  auto service = std::make_shared<GpuObservabilityService>(device(), 16);
  const auto stale = service->acquire_resource(
      GpuSubsystem::media, GpuResourceKind::native_video_surface, 1, 2, 100);
  require(!stale.accepted);
  require(stale.code == "RFX-GPU-OBS-STALE-RESOURCE");
  require(service->snapshot().stale_generation_resources_accepted == 0);

  const auto unattributed =
      service->record_submission(GpuSubsystem::skia, 0, 1);
  require(!unattributed.accepted);
  require(service->snapshot().unattributed_submissions == 1);
  require(!service->snapshot().strict_path_clean());

  auto abandoned_service =
      std::make_shared<GpuObservabilityService>(device(), 16);
  {
    GpuObservedFenceLease abandoned(
        abandoned_service, GpuSubsystem::presentation, 1);
  }
  const auto abandoned = abandoned_service->snapshot();
  require(abandoned.fences_issued == 1);
  require(abandoned.fences_abandoned == 1);
  require(!abandoned.strict_path_clean());
  require(!abandoned.quiescent());
}

}  // namespace

int main() {
  prove_clean_attributed_trace();
  prove_fail_closed_contracts();
}
