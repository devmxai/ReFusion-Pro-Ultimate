#include "refusion/core/ProjectClock.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace refusion::core {
namespace {

constexpr std::uint64_t kNanosecondsPerSecond = 1'000'000'000;

[[nodiscard]] bool multiply_fits(const std::uint64_t lhs,
                                 const std::uint64_t rhs) noexcept {
  return lhs == 0 || rhs <= std::numeric_limits<std::uint64_t>::max() / lhs;
}

[[nodiscard]] std::uint64_t saturating_add(const std::uint64_t lhs,
                                           const std::uint64_t rhs) noexcept {
  if (rhs > std::numeric_limits<std::uint64_t>::max() - lhs) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  return lhs + rhs;
}

}  // namespace

bool ClockTick::valid() const noexcept { return source_generation != 0; }

bool ProjectClockSpec::valid() const noexcept {
  return duration_ns != 0 && frame_rate.valid() &&
         multiply_fits(kNanosecondsPerSecond, frame_rate.denominator) &&
         multiply_fits(duration_ns, frame_rate.numerator) &&
         frame_interval_ns() != 0 && frame_count() != 0;
}

std::uint64_t ProjectClockSpec::frame_interval_ns() const noexcept {
  if (!frame_rate.valid() ||
      !multiply_fits(kNanosecondsPerSecond, frame_rate.denominator)) {
    return 0;
  }
  const auto scaled_second = kNanosecondsPerSecond *
                             static_cast<std::uint64_t>(frame_rate.denominator);
  const auto rate = static_cast<std::uint64_t>(frame_rate.numerator);
  const auto quotient = scaled_second / rate;
  const auto remainder = scaled_second % rate;
  return quotient + static_cast<std::uint64_t>(remainder >= (rate + 1) / 2);
}

std::uint64_t ProjectClockSpec::frame_count() const noexcept {
  if (!frame_rate.valid() ||
      !multiply_fits(duration_ns, frame_rate.numerator) ||
      !multiply_fits(kNanosecondsPerSecond, frame_rate.denominator)) {
    return 0;
  }
  const auto scaled_duration =
      duration_ns * static_cast<std::uint64_t>(frame_rate.numerator);
  const auto frame_denominator =
      kNanosecondsPerSecond *
      static_cast<std::uint64_t>(frame_rate.denominator);
  return scaled_duration / frame_denominator +
         static_cast<std::uint64_t>(scaled_duration % frame_denominator != 0);
}

std::uint64_t ProjectClockSpec::frame_at_time(
    const ProjectTimeNs position_ns) const noexcept {
  if (!frame_rate.valid() ||
      !multiply_fits(position_ns, frame_rate.numerator) ||
      !multiply_fits(kNanosecondsPerSecond, frame_rate.denominator)) {
    return 0;
  }
  const auto frame_denominator =
      kNanosecondsPerSecond *
      static_cast<std::uint64_t>(frame_rate.denominator);
  return (position_ns * static_cast<std::uint64_t>(frame_rate.numerator)) /
         frame_denominator;
}

ProjectTimeNs ProjectClockSpec::time_at_frame(
    const std::uint64_t frame_index) const noexcept {
  if (!frame_rate.valid() ||
      !multiply_fits(frame_index, frame_rate.denominator)) {
    return 0;
  }
  const auto scaled_frame =
      frame_index * static_cast<std::uint64_t>(frame_rate.denominator);
  if (!multiply_fits(scaled_frame, kNanosecondsPerSecond)) {
    return 0;
  }
  const auto scaled_time = scaled_frame * kNanosecondsPerSecond;
  const auto rate = static_cast<std::uint64_t>(frame_rate.numerator);
  return scaled_time / rate +
         static_cast<std::uint64_t>(scaled_time % rate != 0);
}

bool ProjectClockSnapshot::running() const noexcept {
  return state == ProjectClockState::running;
}

ProjectClock::ProjectClock(ProjectClockSpec spec) : spec_(spec) {
  if (!spec_.valid()) {
    throw std::invalid_argument("project clock specification is invalid");
  }
}

ProjectClockSpec ProjectClock::spec() const noexcept { return spec_; }

ProjectClockSnapshot ProjectClock::snapshot() const {
  std::scoped_lock lock(mutex_);
  return snapshot_locked();
}

ProjectClockSnapshot ProjectClock::snapshot_locked() const noexcept {
  return ProjectClockSnapshot{
      .state = state_,
      .position_ns = position_ns_,
      .duration_ns = spec_.duration_ns,
      .frame_index = position_ns_ >= spec_.duration_ns
                         ? spec_.frame_count()
                         : spec_.frame_at_time(position_ns_),
      .frame_count = spec_.frame_count(),
      .loop_index = loop_index_,
      .epoch_id = epoch_id_,
      .clock_source_generation = clock_anchor_.source_generation,
      .sample_sequence = sample_sequence_,
  };
}

ProjectClockResult ProjectClock::accepted_locked(std::string code) const {
  return ProjectClockResult{
      .accepted = true,
      .snapshot = snapshot_locked(),
      .code = std::move(code),
  };
}

ProjectClockResult ProjectClock::rejected_locked(std::string code,
                                                 std::string diagnostic) const {
  return ProjectClockResult{
      .accepted = false,
      .snapshot = snapshot_locked(),
      .code = std::move(code),
      .diagnostic = std::move(diagnostic),
  };
}

bool ProjectClock::begin_epoch_locked(const ClockTick tick) noexcept {
  if (!tick.valid() || epoch_id_ == std::numeric_limits<std::uint64_t>::max()) {
    return false;
  }
  ++epoch_id_;
  project_anchor_ns_ = absolute_position_locked();
  clock_anchor_ = tick;
  return true;
}

ProjectTimeNs ProjectClock::absolute_position_locked() const noexcept {
  if (!spec_.loop || loop_index_ == 0) {
    return position_ns_;
  }
  if (!multiply_fits(loop_index_, spec_.duration_ns)) {
    return std::numeric_limits<ProjectTimeNs>::max();
  }
  return saturating_add(loop_index_ * spec_.duration_ns, position_ns_);
}

bool ProjectClock::sample_locked(const ClockTick tick) noexcept {
  if (state_ != ProjectClockState::running) {
    return true;
  }
  if (!tick.valid() ||
      tick.source_generation != clock_anchor_.source_generation ||
      tick.nanoseconds < clock_anchor_.nanoseconds ||
      sample_sequence_ == std::numeric_limits<std::uint64_t>::max()) {
    return false;
  }

  const auto elapsed_ns = tick.nanoseconds - clock_anchor_.nanoseconds;
  const auto absolute_ns = saturating_add(project_anchor_ns_, elapsed_ns);
  if (spec_.loop) {
    loop_index_ = absolute_ns / spec_.duration_ns;
    position_ns_ = absolute_ns % spec_.duration_ns;
  } else if (absolute_ns >= spec_.duration_ns) {
    position_ns_ = spec_.duration_ns;
    loop_index_ = 0;
    state_ = ProjectClockState::completed;
  } else {
    position_ns_ = absolute_ns;
    loop_index_ = 0;
  }
  ++sample_sequence_;
  return true;
}

ProjectClockResult ProjectClock::start(const ClockTick tick) {
  std::scoped_lock lock(mutex_);
  if (!tick.valid() || epoch_id_ == std::numeric_limits<std::uint64_t>::max()) {
    return rejected_locked("RFX-CLOCK-EPOCH-REJECTED",
                           "clock tick is invalid or epoch ID overflowed");
  }
  ++epoch_id_;
  position_ns_ = 0;
  project_anchor_ns_ = 0;
  clock_anchor_ = tick;
  loop_index_ = 0;
  state_ = ProjectClockState::running;
  return accepted_locked("RFX-CLOCK-STARTED");
}

ProjectClockResult ProjectClock::play(const ClockTick tick) {
  std::scoped_lock lock(mutex_);
  if (state_ == ProjectClockState::running) {
    if (!sample_locked(tick)) {
      return rejected_locked(
          "RFX-CLOCK-SOURCE-DISCONTINUITY",
          "clock source changed generation or moved backwards");
    }
    return accepted_locked("RFX-CLOCK-ALREADY-RUNNING");
  }
  if (!tick.valid() || epoch_id_ == std::numeric_limits<std::uint64_t>::max()) {
    return rejected_locked("RFX-CLOCK-EPOCH-REJECTED",
                           "clock tick is invalid or epoch ID overflowed");
  }
  if (position_ns_ >= spec_.duration_ns) {
    position_ns_ = 0;
    loop_index_ = 0;
  }
  if (!begin_epoch_locked(tick)) {
    return rejected_locked("RFX-CLOCK-EPOCH-REJECTED",
                           "clock tick is invalid or epoch ID overflowed");
  }
  state_ = ProjectClockState::running;
  return accepted_locked("RFX-CLOCK-PLAYING");
}

ProjectClockResult ProjectClock::pause(const ClockTick tick) {
  std::scoped_lock lock(mutex_);
  if (state_ != ProjectClockState::running) {
    return accepted_locked("RFX-CLOCK-ALREADY-PAUSED");
  }
  if (epoch_id_ == std::numeric_limits<std::uint64_t>::max()) {
    return rejected_locked("RFX-CLOCK-EPOCH-OVERFLOW",
                           "project clock epoch ID overflowed");
  }
  if (!sample_locked(tick)) {
    return rejected_locked(
        "RFX-CLOCK-SOURCE-DISCONTINUITY",
        "clock source changed generation or moved backwards");
  }
  if (!begin_epoch_locked(tick)) {
    return rejected_locked("RFX-CLOCK-EPOCH-REJECTED",
                           "clock tick is invalid or epoch ID overflowed");
  }
  state_ = ProjectClockState::paused;
  return accepted_locked("RFX-CLOCK-PAUSED");
}

ProjectClockResult ProjectClock::suspend() {
  std::scoped_lock lock(mutex_);
  if (epoch_id_ == std::numeric_limits<std::uint64_t>::max()) {
    return rejected_locked("RFX-CLOCK-EPOCH-OVERFLOW",
                           "project clock epoch ID overflowed");
  }
  ++epoch_id_;
  state_ = ProjectClockState::paused;
  project_anchor_ns_ = position_ns_;
  clock_anchor_ = {};
  return accepted_locked("RFX-CLOCK-SUSPENDED");
}

ProjectClockResult ProjectClock::stop() {
  std::scoped_lock lock(mutex_);
  if (epoch_id_ == std::numeric_limits<std::uint64_t>::max()) {
    return rejected_locked("RFX-CLOCK-EPOCH-OVERFLOW",
                           "project clock epoch ID overflowed");
  }
  ++epoch_id_;
  state_ = ProjectClockState::stopped;
  position_ns_ = 0;
  project_anchor_ns_ = 0;
  loop_index_ = 0;
  clock_anchor_ = {};
  return accepted_locked("RFX-CLOCK-STOPPED");
}

ProjectClockResult ProjectClock::seek_to_frame(const std::uint64_t frame_index,
                                               const ClockTick tick) {
  std::scoped_lock lock(mutex_);
  const auto total_frames = spec_.frame_count();
  if (frame_index > total_frames) {
    return rejected_locked("RFX-CLOCK-SEEK-RANGE",
                           "frame is outside the composition");
  }
  if (!tick.valid()) {
    return rejected_locked("RFX-CLOCK-TICK-INVALID",
                           "seek requires a valid clock correlation tick");
  }
  if (epoch_id_ == std::numeric_limits<std::uint64_t>::max()) {
    return rejected_locked("RFX-CLOCK-EPOCH-OVERFLOW",
                           "project clock epoch ID overflowed");
  }
  const auto was_running = state_ == ProjectClockState::running;
  position_ns_ = frame_index == total_frames ? spec_.duration_ns
                                             : spec_.time_at_frame(frame_index);
  loop_index_ = 0;
  if (!begin_epoch_locked(tick)) {
    return rejected_locked("RFX-CLOCK-EPOCH-REJECTED",
                           "clock tick is invalid or epoch ID overflowed");
  }
  state_ = was_running ? ProjectClockState::running : ProjectClockState::paused;
  return accepted_locked("RFX-CLOCK-SEEKED");
}

ProjectClockResult ProjectClock::sample(const ClockTick tick) {
  std::scoped_lock lock(mutex_);
  if (!sample_locked(tick)) {
    return rejected_locked(
        "RFX-CLOCK-SOURCE-DISCONTINUITY",
        "clock source changed generation or moved backwards");
  }
  return accepted_locked("RFX-CLOCK-SAMPLED");
}

}  // namespace refusion::core
