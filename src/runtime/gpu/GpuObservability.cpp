#include "refusion/runtime/gpu/GpuObservability.hpp"

#include <algorithm>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace refusion::runtime::gpu {
namespace {

struct ResourceRecord final {
  GpuSubsystem subsystem{GpuSubsystem::device};
  GpuResourceKind kind{GpuResourceKind::render_context};
  std::uint64_t generation{0};
  std::uint64_t bytes{0};
};

struct FenceRecord final {
  GpuSubsystem subsystem{GpuSubsystem::device};
  std::uint64_t generation{0};
};

[[nodiscard]] bool valid_device(const DeviceIdentity& device) noexcept {
  return device.adapter_id != 0 && device.generation != 0;
}

[[nodiscard]] std::uint8_t thermal_rank(const GpuThermalState state) noexcept {
  switch (state) {
    case GpuThermalState::nominal:
      return 0;
    case GpuThermalState::fair:
      return 1;
    case GpuThermalState::serious:
      return 2;
    case GpuThermalState::critical:
      return 3;
    case GpuThermalState::unknown:
      return std::numeric_limits<std::uint8_t>::max();
  }
  return std::numeric_limits<std::uint8_t>::max();
}

}  // namespace

struct GpuObservabilityService::Implementation final {
  explicit Implementation(DeviceIdentity initial_device,
                          const std::size_t trace_limit,
                          GpuQualificationBudget qualification_budget)
      : device(std::move(initial_device)), maximum_trace_events(trace_limit),
        budget(std::move(qualification_budget)) {}

  [[nodiscard]] GpuObservationResult accepted_locked(std::string code) const {
    return {
        .accepted = true,
        .event_sequence = snapshot.event_sequence,
        .code = std::move(code),
    };
  }

  [[nodiscard]] GpuObservationResult rejected_locked(
      std::string code, std::string diagnostic) {
    ++snapshot.contract_rejections;
    return {
        .accepted = false,
        .event_sequence = snapshot.event_sequence,
        .code = std::move(code),
        .diagnostic = std::move(diagnostic),
    };
  }

  [[nodiscard]] bool current_generation_locked(
      const std::uint64_t generation) const noexcept {
    return generation != 0 && generation == device.generation;
  }

  void append_locked(const GpuSubsystem subsystem, const GpuTraceKind kind,
                     const std::uint64_t object_id,
                     const std::uint64_t generation,
                     const std::uint64_t bytes = 0,
                     const std::uint64_t latency_ns = 0,
                     const GpuThermalState thermal_state =
                         GpuThermalState::unknown) {
    if (snapshot.event_sequence != std::numeric_limits<std::uint64_t>::max()) {
      ++snapshot.event_sequence;
    }
    if (snapshot.trace.size() < maximum_trace_events) {
      snapshot.trace.push_back({
          .sequence = snapshot.event_sequence,
          .subsystem = subsystem,
          .kind = kind,
          .object_id = object_id,
          .device_generation = generation,
          .bytes = bytes,
          .latency_ns = latency_ns,
          .thermal_state = thermal_state,
      });
    } else {
      ++snapshot.trace_events_dropped;
    }
  }

  [[nodiscard]] bool quiescent_locked() const noexcept {
    return resources.empty() && fences.empty() &&
           snapshot.current_resident_bytes == 0;
  }

  mutable std::mutex mutex;
  mutable std::condition_variable condition;
  DeviceIdentity device;
  std::size_t maximum_trace_events{0};
  std::uint64_t next_object_id{1};
  GpuQualificationBudget budget;
  GpuObservabilitySnapshot snapshot;
  std::unordered_map<std::uint64_t, ResourceRecord> resources;
  std::unordered_map<std::uint64_t, FenceRecord> fences;
};

bool GpuQualificationBudget::valid() const noexcept {
  return !device_tier_id.empty() && maximum_peak_resident_bytes != 0 &&
         maximum_fence_latency_ns != 0 &&
         maximum_thermal_state != GpuThermalState::unknown;
}

bool GpuObservabilitySnapshot::strict_path_clean() const noexcept {
  return stale_generation_resources_accepted == 0 &&
         unattributed_submissions == 0 && unattributed_copies == 0 &&
         unattributed_conversions == 0 && fences_abandoned == 0 &&
         contract_rejections == 0 && trace_events_dropped == 0;
}

bool GpuObservabilitySnapshot::quiescent() const noexcept {
  return resource_leases_acquired == resource_leases_released &&
         fences_issued == fences_completed && current_resident_bytes == 0;
}

bool GpuObservabilitySnapshot::within_qualification_budget() const noexcept {
  return qualification_budget.valid() && resource_leases_acquired != 0 &&
         fence_latency_samples != 0 && thermal_samples != 0 &&
         peak_resident_bytes <=
             qualification_budget.maximum_peak_resident_bytes &&
         fence_latency_max_ns <=
             qualification_budget.maximum_fence_latency_ns &&
         thermal_rank(maximum_observed_thermal_state) <=
             thermal_rank(qualification_budget.maximum_thermal_state);
}

GpuObservabilityService::GpuObservabilityService(
    DeviceIdentity device, const std::size_t maximum_trace_events,
    GpuQualificationBudget budget)
    : implementation_(std::make_unique<Implementation>(
          std::move(device), maximum_trace_events, std::move(budget))) {
  if (!valid_device(implementation_->device) || maximum_trace_events == 0) {
    throw std::invalid_argument(
        "GPU observability requires a device generation and trace capacity");
  }
  if (!implementation_->budget.device_tier_id.empty() &&
      !implementation_->budget.valid()) {
    throw std::invalid_argument(
        "A named GPU qualification tier requires complete memory, latency and "
        "thermal budgets");
  }
  implementation_->snapshot.device = implementation_->device;
  implementation_->snapshot.qualification_budget = implementation_->budget;
}

GpuObservabilityService::~GpuObservabilityService() = default;

std::uint64_t GpuObservabilityService::issue_object_id() {
  std::scoped_lock lock(implementation_->mutex);
  if (implementation_->next_object_id ==
      std::numeric_limits<std::uint64_t>::max()) {
    throw std::overflow_error("GPU observation object ID overflowed");
  }
  return implementation_->next_object_id++;
}

bool GpuObservabilityService::observes(
    const DeviceIdentity& candidate) const {
  std::scoped_lock lock(implementation_->mutex);
  return candidate.backend == implementation_->device.backend &&
         candidate.adapter_id == implementation_->device.adapter_id &&
         candidate.generation == implementation_->device.generation;
}

GpuObservationResult GpuObservabilityService::acquire_resource(
    const GpuSubsystem subsystem, const GpuResourceKind kind,
    const std::uint64_t object_id, const std::uint64_t device_generation,
    const std::uint64_t resident_bytes) {
  std::scoped_lock lock(implementation_->mutex);
  if (object_id == 0) {
    return implementation_->rejected_locked(
        "RFX-GPU-OBS-RESOURCE-INVALID",
        "Observed GPU resources require a stable non-zero ID");
  }
  if (!implementation_->current_generation_locked(device_generation)) {
    ++implementation_->snapshot.stale_generation_rejections;
    implementation_->append_locked(
        subsystem, GpuTraceKind::stale_generation_rejected, object_id,
        device_generation);
    return {
        .accepted = false,
        .event_sequence = implementation_->snapshot.event_sequence,
        .code = "RFX-GPU-OBS-STALE-RESOURCE",
        .diagnostic = "A stale-generation resource lease was rejected",
    };
  }
  if (implementation_->resources.contains(object_id)) {
    return implementation_->rejected_locked(
        "RFX-GPU-OBS-RESOURCE-DUPLICATE",
        "The GPU resource ID already has an active lease");
  }
  implementation_->resources.emplace(
      object_id,
      ResourceRecord{.subsystem = subsystem,
                     .kind = kind,
                     .generation = device_generation,
                     .bytes = resident_bytes});
  ++implementation_->snapshot.resource_leases_acquired;
  implementation_->snapshot.current_resident_bytes += resident_bytes;
  implementation_->snapshot.peak_resident_bytes = std::max(
      implementation_->snapshot.peak_resident_bytes,
      implementation_->snapshot.current_resident_bytes);
  implementation_->append_locked(subsystem, GpuTraceKind::resource_acquired,
                                 object_id, device_generation, resident_bytes);
  return implementation_->accepted_locked("RFX-GPU-OBS-RESOURCE-ACQUIRED");
}

GpuObservationResult GpuObservabilityService::release_resource(
    const GpuSubsystem subsystem, const std::uint64_t object_id,
    const std::uint64_t device_generation) {
  std::scoped_lock lock(implementation_->mutex);
  const auto iterator = implementation_->resources.find(object_id);
  if (iterator == implementation_->resources.end() ||
      iterator->second.subsystem != subsystem ||
      iterator->second.generation != device_generation) {
    return implementation_->rejected_locked(
        "RFX-GPU-OBS-RESOURCE-RELEASE-INVALID",
        "The GPU resource release does not match an active lease");
  }
  const auto bytes = iterator->second.bytes;
  implementation_->resources.erase(iterator);
  ++implementation_->snapshot.resource_leases_released;
  implementation_->snapshot.current_resident_bytes -= bytes;
  implementation_->append_locked(subsystem, GpuTraceKind::resource_released,
                                 object_id, device_generation, bytes);
  implementation_->condition.notify_all();
  return implementation_->accepted_locked("RFX-GPU-OBS-RESOURCE-RELEASED");
}

GpuObservationResult GpuObservabilityService::record_submission(
    const GpuSubsystem subsystem, const std::uint64_t attribution_id,
    const std::uint64_t device_generation) {
  std::scoped_lock lock(implementation_->mutex);
  if (attribution_id == 0) {
    ++implementation_->snapshot.unattributed_submissions;
    return implementation_->rejected_locked(
        "RFX-GPU-OBS-SUBMISSION-UNATTRIBUTED",
        "Every GPU submission requires a non-zero attribution ID");
  }
  if (!implementation_->current_generation_locked(device_generation)) {
    ++implementation_->snapshot.stale_generation_rejections;
    implementation_->append_locked(
        subsystem, GpuTraceKind::stale_generation_rejected, attribution_id,
        device_generation);
    return {
        .accepted = false,
        .event_sequence = implementation_->snapshot.event_sequence,
        .code = "RFX-GPU-OBS-STALE-SUBMISSION",
        .diagnostic = "A stale-generation GPU submission was rejected",
    };
  }
  ++implementation_->snapshot.attributed_submissions;
  implementation_->append_locked(subsystem,
                                 GpuTraceKind::submission_attributed,
                                 attribution_id, device_generation);
  return implementation_->accepted_locked("RFX-GPU-OBS-SUBMISSION-RECORDED");
}

GpuObservationResult GpuObservabilityService::record_copy(
    const GpuSubsystem subsystem, const std::uint64_t attribution_id,
    const std::uint64_t device_generation, const std::uint64_t bytes,
    const bool conversion) {
  std::scoped_lock lock(implementation_->mutex);
  if (attribution_id == 0 || bytes == 0) {
    if (conversion) {
      ++implementation_->snapshot.unattributed_conversions;
    } else {
      ++implementation_->snapshot.unattributed_copies;
    }
    return implementation_->rejected_locked(
        "RFX-GPU-OBS-COPY-UNATTRIBUTED",
        "Every GPU copy or conversion requires attribution and byte extent");
  }
  if (!implementation_->current_generation_locked(device_generation)) {
    ++implementation_->snapshot.stale_generation_rejections;
    implementation_->append_locked(
        subsystem, GpuTraceKind::stale_generation_rejected, attribution_id,
        device_generation);
    return {
        .accepted = false,
        .event_sequence = implementation_->snapshot.event_sequence,
        .code = "RFX-GPU-OBS-STALE-COPY",
        .diagnostic = "A stale-generation GPU copy was rejected",
    };
  }
  if (conversion) {
    ++implementation_->snapshot.attributed_conversions;
  } else {
    ++implementation_->snapshot.attributed_copies;
  }
  implementation_->snapshot.attributed_copy_bytes += bytes;
  implementation_->append_locked(
      subsystem, conversion ? GpuTraceKind::conversion_attributed
                            : GpuTraceKind::copy_attributed,
      attribution_id, device_generation, bytes);
  return implementation_->accepted_locked("RFX-GPU-OBS-COPY-RECORDED");
}

GpuObservationResult GpuObservabilityService::issue_fence(
    const GpuSubsystem subsystem, const std::uint64_t fence_id,
    const std::uint64_t device_generation) {
  std::scoped_lock lock(implementation_->mutex);
  if (fence_id == 0 ||
      !implementation_->current_generation_locked(device_generation) ||
      implementation_->fences.contains(fence_id)) {
    return implementation_->rejected_locked(
        "RFX-GPU-OBS-FENCE-ISSUE-INVALID",
        "A fence requires a unique ID on the current GPU generation");
  }
  implementation_->fences.emplace(
      fence_id,
      FenceRecord{.subsystem = subsystem, .generation = device_generation});
  ++implementation_->snapshot.fences_issued;
  implementation_->append_locked(subsystem, GpuTraceKind::fence_issued,
                                 fence_id, device_generation);
  return implementation_->accepted_locked("RFX-GPU-OBS-FENCE-ISSUED");
}

GpuObservationResult GpuObservabilityService::complete_fence(
    const GpuSubsystem subsystem, const std::uint64_t fence_id,
    const std::uint64_t device_generation, const std::uint64_t latency_ns) {
  std::scoped_lock lock(implementation_->mutex);
  const auto iterator = implementation_->fences.find(fence_id);
  if (iterator == implementation_->fences.end() ||
      iterator->second.subsystem != subsystem ||
      iterator->second.generation != device_generation) {
    return implementation_->rejected_locked(
        "RFX-GPU-OBS-FENCE-COMPLETE-INVALID",
        "Fence completion does not match an issued fence lease");
  }
  implementation_->fences.erase(iterator);
  ++implementation_->snapshot.fences_completed;
  ++implementation_->snapshot.fence_latency_samples;
  implementation_->snapshot.fence_latency_total_ns += latency_ns;
  implementation_->snapshot.fence_latency_max_ns = std::max(
      implementation_->snapshot.fence_latency_max_ns, latency_ns);
  implementation_->append_locked(subsystem, GpuTraceKind::fence_completed,
                                 fence_id, device_generation, 0, latency_ns);
  implementation_->condition.notify_all();
  return implementation_->accepted_locked("RFX-GPU-OBS-FENCE-COMPLETED");
}

GpuObservationResult GpuObservabilityService::abandon_fence(
    const GpuSubsystem subsystem, const std::uint64_t fence_id,
    const std::uint64_t device_generation) {
  std::scoped_lock lock(implementation_->mutex);
  const auto iterator = implementation_->fences.find(fence_id);
  if (iterator == implementation_->fences.end() ||
      iterator->second.subsystem != subsystem ||
      iterator->second.generation != device_generation) {
    return implementation_->rejected_locked(
        "RFX-GPU-OBS-FENCE-ABANDON-INVALID",
        "Fence abandonment does not match an issued fence lease");
  }
  implementation_->fences.erase(iterator);
  ++implementation_->snapshot.fences_abandoned;
  implementation_->append_locked(subsystem, GpuTraceKind::fence_abandoned,
                                 fence_id, device_generation);
  implementation_->condition.notify_all();
  return implementation_->accepted_locked("RFX-GPU-OBS-FENCE-ABANDONED");
}

GpuObservationResult GpuObservabilityService::observe_device_loss(
    const DeviceIdentity& replacement) {
  std::scoped_lock lock(implementation_->mutex);
  if (valid_device(replacement) &&
      replacement.backend == implementation_->device.backend &&
      replacement.adapter_id == implementation_->device.adapter_id &&
      replacement.generation == implementation_->device.generation) {
    return implementation_->accepted_locked(
        "RFX-GPU-OBS-DEVICE-LOSS-ALREADY-OBSERVED");
  }
  if (!valid_device(replacement) ||
      replacement.backend != implementation_->device.backend ||
      replacement.adapter_id != implementation_->device.adapter_id ||
      replacement.generation <= implementation_->device.generation) {
    return implementation_->rejected_locked(
        "RFX-GPU-OBS-DEVICE-LOSS-INVALID",
        "Device-loss observation must advance the admitted adapter generation");
  }
  implementation_->device = replacement;
  implementation_->snapshot.device = replacement;
  ++implementation_->snapshot.device_loss_events;
  implementation_->append_locked(GpuSubsystem::device,
                                 GpuTraceKind::device_lost, 0,
                                 replacement.generation);
  return implementation_->accepted_locked("RFX-GPU-OBS-DEVICE-LOST");
}

GpuObservationResult GpuObservabilityService::reject_stale_generation(
    const GpuSubsystem subsystem, const std::uint64_t candidate_generation) {
  std::scoped_lock lock(implementation_->mutex);
  if (candidate_generation == 0 ||
      candidate_generation == implementation_->device.generation) {
    return implementation_->rejected_locked(
        "RFX-GPU-OBS-STALE-REJECTION-INVALID",
        "A stale-generation rejection requires a different non-zero generation");
  }
  ++implementation_->snapshot.stale_generation_rejections;
  implementation_->append_locked(
      subsystem, GpuTraceKind::stale_generation_rejected, 0,
      candidate_generation);
  return implementation_->accepted_locked("RFX-GPU-OBS-STALE-REJECTED");
}

GpuObservationResult GpuObservabilityService::record_thermal_sample(
    const GpuSubsystem subsystem, const GpuThermalState state) {
  std::scoped_lock lock(implementation_->mutex);
  if (state == GpuThermalState::unknown) {
    return implementation_->rejected_locked(
        "RFX-GPU-OBS-THERMAL-UNKNOWN",
        "A GPU thermal qualification sample must map to a typed state");
  }
  ++implementation_->snapshot.thermal_samples;
  if (thermal_rank(state) > thermal_rank(
                                implementation_->snapshot
                                    .maximum_observed_thermal_state)) {
    implementation_->snapshot.maximum_observed_thermal_state = state;
  }
  implementation_->append_locked(
      subsystem, GpuTraceKind::thermal_sampled, 0,
      implementation_->device.generation, 0, 0, state);
  return implementation_->accepted_locked("RFX-GPU-OBS-THERMAL-RECORDED");
}

GpuObservabilitySnapshot GpuObservabilityService::snapshot() const {
  std::scoped_lock lock(implementation_->mutex);
  return implementation_->snapshot;
}

bool GpuObservabilityService::wait_until_quiescent(
    const std::chrono::milliseconds timeout) const {
  std::unique_lock lock(implementation_->mutex);
  return implementation_->condition.wait_for(
      lock, timeout,
      [this] { return implementation_->quiescent_locked(); });
}

GpuObservedResourceLease::GpuObservedResourceLease(
    std::shared_ptr<GpuObservabilityService> service,
    const GpuSubsystem subsystem, const GpuResourceKind kind,
    const std::uint64_t device_generation,
    const std::uint64_t resident_bytes)
    : service_(std::move(service)), subsystem_(subsystem),
      device_generation_(device_generation) {
  if (!service_) {
    throw std::invalid_argument("Observed GPU resource requires a service");
  }
  object_id_ = service_->issue_object_id();
  const auto result = service_->acquire_resource(
      subsystem_, kind, object_id_, device_generation_, resident_bytes);
  if (!result.accepted) {
    throw std::runtime_error(result.code + ": " + result.diagnostic);
  }
}

GpuObservedResourceLease::~GpuObservedResourceLease() {
  if (service_ && object_id_ != 0) {
    static_cast<void>(service_->release_resource(
        subsystem_, object_id_, device_generation_));
  }
}

std::uint64_t GpuObservedResourceLease::object_id() const noexcept {
  return object_id_;
}

GpuObservedFenceLease::GpuObservedFenceLease(
    std::shared_ptr<GpuObservabilityService> service,
    const GpuSubsystem subsystem, const std::uint64_t device_generation)
    : service_(std::move(service)), subsystem_(subsystem),
      device_generation_(device_generation) {
  if (!service_) {
    throw std::invalid_argument("Observed GPU fence requires a service");
  }
  object_id_ = service_->issue_object_id();
  const auto result =
      service_->issue_fence(subsystem_, object_id_, device_generation_);
  if (!result.accepted) {
    throw std::runtime_error(result.code + ": " + result.diagnostic);
  }
}

GpuObservedFenceLease::~GpuObservedFenceLease() {
  if (service_ && !completed_ && object_id_ != 0) {
    static_cast<void>(service_->abandon_fence(
        subsystem_, object_id_, device_generation_));
  }
}

std::uint64_t GpuObservedFenceLease::object_id() const noexcept {
  return object_id_;
}

bool GpuObservedFenceLease::complete(const std::uint64_t latency_ns) {
  if (!service_ || completed_) {
    return false;
  }
  const auto result = service_->complete_fence(
      subsystem_, object_id_, device_generation_, latency_ns);
  completed_ = result.accepted;
  return completed_;
}

}  // namespace refusion::runtime::gpu
