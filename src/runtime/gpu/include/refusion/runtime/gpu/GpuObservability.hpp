#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "refusion/runtime/gpu/GpuDeviceService.hpp"

namespace refusion::runtime::gpu {

enum class GpuSubsystem : std::uint8_t {
  device,
  presentation,
  skia,
  media,
};

enum class GpuResourceKind : std::uint8_t {
  viewport_layer,
  drawable,
  render_context,
  native_video_surface,
};

enum class GpuThermalState : std::uint8_t {
  nominal,
  fair,
  serious,
  critical,
  unknown = 255,
};

struct GpuQualificationBudget final {
  std::string device_tier_id;
  std::uint64_t maximum_peak_resident_bytes{0};
  std::uint64_t maximum_fence_latency_ns{0};
  GpuThermalState maximum_thermal_state{GpuThermalState::unknown};

  [[nodiscard]] bool valid() const noexcept;
};

enum class GpuTraceKind : std::uint8_t {
  resource_acquired,
  resource_released,
  submission_attributed,
  copy_attributed,
  conversion_attributed,
  fence_issued,
  fence_completed,
  fence_abandoned,
  stale_generation_rejected,
  device_lost,
  thermal_sampled,
};

struct GpuTraceEvent final {
  std::uint64_t sequence{0};
  GpuSubsystem subsystem{GpuSubsystem::device};
  GpuTraceKind kind{GpuTraceKind::resource_acquired};
  std::uint64_t object_id{0};
  std::uint64_t device_generation{0};
  std::uint64_t bytes{0};
  std::uint64_t latency_ns{0};
  GpuThermalState thermal_state{GpuThermalState::unknown};
};

struct GpuObservationResult final {
  bool accepted{false};
  std::uint64_t event_sequence{0};
  std::string code;
  std::string diagnostic;
};

struct GpuObservabilitySnapshot final {
  DeviceIdentity device;
  GpuQualificationBudget qualification_budget;
  std::uint64_t event_sequence{0};
  std::uint64_t device_loss_events{0};
  std::uint64_t resource_leases_acquired{0};
  std::uint64_t resource_leases_released{0};
  std::uint64_t fences_issued{0};
  std::uint64_t fences_completed{0};
  std::uint64_t fences_abandoned{0};
  std::uint64_t attributed_submissions{0};
  std::uint64_t attributed_copies{0};
  std::uint64_t attributed_conversions{0};
  std::uint64_t attributed_copy_bytes{0};
  std::uint64_t current_resident_bytes{0};
  std::uint64_t peak_resident_bytes{0};
  std::uint64_t fence_latency_samples{0};
  std::uint64_t fence_latency_total_ns{0};
  std::uint64_t fence_latency_max_ns{0};
  std::uint64_t thermal_samples{0};
  GpuThermalState maximum_observed_thermal_state{GpuThermalState::nominal};
  std::uint64_t stale_generation_rejections{0};
  std::uint64_t stale_generation_resources_accepted{0};
  std::uint64_t unattributed_submissions{0};
  std::uint64_t unattributed_copies{0};
  std::uint64_t unattributed_conversions{0};
  std::uint64_t contract_rejections{0};
  std::uint64_t trace_events_dropped{0};
  std::vector<GpuTraceEvent> trace;

  [[nodiscard]] bool strict_path_clean() const noexcept;
  [[nodiscard]] bool quiescent() const noexcept;
  [[nodiscard]] bool within_qualification_budget() const noexcept;
};

// Process-local measurement only. This service owns no GPU device, queue,
// project time, render scheduling or revision authority.
class GpuObservabilityService final {
 public:
  explicit GpuObservabilityService(DeviceIdentity device,
                                   std::size_t maximum_trace_events = 4096,
                                   GpuQualificationBudget budget = {});
  ~GpuObservabilityService();

  GpuObservabilityService(const GpuObservabilityService&) = delete;
  GpuObservabilityService& operator=(const GpuObservabilityService&) = delete;

  [[nodiscard]] std::uint64_t issue_object_id();
  [[nodiscard]] bool observes(const DeviceIdentity& candidate) const;
  [[nodiscard]] GpuObservationResult acquire_resource(
      GpuSubsystem subsystem, GpuResourceKind kind, std::uint64_t object_id,
      std::uint64_t device_generation, std::uint64_t resident_bytes);
  [[nodiscard]] GpuObservationResult release_resource(
      GpuSubsystem subsystem, std::uint64_t object_id,
      std::uint64_t device_generation);
  [[nodiscard]] GpuObservationResult record_submission(
      GpuSubsystem subsystem, std::uint64_t attribution_id,
      std::uint64_t device_generation);
  [[nodiscard]] GpuObservationResult record_copy(
      GpuSubsystem subsystem, std::uint64_t attribution_id,
      std::uint64_t device_generation, std::uint64_t bytes,
      bool conversion);
  [[nodiscard]] GpuObservationResult issue_fence(
      GpuSubsystem subsystem, std::uint64_t fence_id,
      std::uint64_t device_generation);
  [[nodiscard]] GpuObservationResult complete_fence(
      GpuSubsystem subsystem, std::uint64_t fence_id,
      std::uint64_t device_generation, std::uint64_t latency_ns);
  [[nodiscard]] GpuObservationResult abandon_fence(
      GpuSubsystem subsystem, std::uint64_t fence_id,
      std::uint64_t device_generation);
  [[nodiscard]] GpuObservationResult observe_device_loss(
      const DeviceIdentity& replacement);
  [[nodiscard]] GpuObservationResult reject_stale_generation(
      GpuSubsystem subsystem, std::uint64_t candidate_generation);
  [[nodiscard]] GpuObservationResult record_thermal_sample(
      GpuSubsystem subsystem, GpuThermalState state);

  [[nodiscard]] GpuObservabilitySnapshot snapshot() const;
  [[nodiscard]] bool wait_until_quiescent(
      std::chrono::milliseconds timeout) const;

 private:
  struct Implementation;
  std::unique_ptr<Implementation> implementation_;
};

class GpuObservedResourceLease final {
 public:
  GpuObservedResourceLease(std::shared_ptr<GpuObservabilityService> service,
                           GpuSubsystem subsystem, GpuResourceKind kind,
                           std::uint64_t device_generation,
                           std::uint64_t resident_bytes);
  ~GpuObservedResourceLease();

  GpuObservedResourceLease(const GpuObservedResourceLease&) = delete;
  GpuObservedResourceLease& operator=(const GpuObservedResourceLease&) = delete;
  GpuObservedResourceLease(GpuObservedResourceLease&&) = delete;
  GpuObservedResourceLease& operator=(GpuObservedResourceLease&&) = delete;

  [[nodiscard]] std::uint64_t object_id() const noexcept;

 private:
  std::shared_ptr<GpuObservabilityService> service_;
  GpuSubsystem subsystem_{GpuSubsystem::device};
  std::uint64_t object_id_{0};
  std::uint64_t device_generation_{0};
};

class GpuObservedFenceLease final {
 public:
  GpuObservedFenceLease(std::shared_ptr<GpuObservabilityService> service,
                        GpuSubsystem subsystem,
                        std::uint64_t device_generation);
  ~GpuObservedFenceLease();

  GpuObservedFenceLease(const GpuObservedFenceLease&) = delete;
  GpuObservedFenceLease& operator=(const GpuObservedFenceLease&) = delete;
  GpuObservedFenceLease(GpuObservedFenceLease&&) = delete;
  GpuObservedFenceLease& operator=(GpuObservedFenceLease&&) = delete;

  [[nodiscard]] std::uint64_t object_id() const noexcept;
  [[nodiscard]] bool complete(std::uint64_t latency_ns);

 private:
  std::shared_ptr<GpuObservabilityService> service_;
  GpuSubsystem subsystem_{GpuSubsystem::device};
  std::uint64_t object_id_{0};
  std::uint64_t device_generation_{0};
  bool completed_{false};
};

}  // namespace refusion::runtime::gpu
