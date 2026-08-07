#pragma once

#include <cstdint>
#include <mutex>
#include <string>

#include "refusion/core/ProjectDocument.hpp"

namespace refusion::core {

struct ClockTick final {
  std::uint64_t nanoseconds{0};
  std::uint64_t source_generation{0};

  [[nodiscard]] bool valid() const noexcept;
};

struct ProjectClockSpec final {
  ProjectTimeNs duration_ns{30'000'000'000};
  RationalRate frame_rate{.numerator = 30, .denominator = 1};
  bool loop{true};

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] std::uint64_t frame_interval_ns() const noexcept;
  [[nodiscard]] std::uint64_t frame_count() const noexcept;
  [[nodiscard]] std::uint64_t frame_at_time(
      ProjectTimeNs position_ns) const noexcept;
  [[nodiscard]] ProjectTimeNs time_at_frame(
      std::uint64_t frame_index) const noexcept;
};

enum class ProjectClockState : std::uint8_t {
  stopped,
  paused,
  running,
  completed,
};

struct ProjectClockSnapshot final {
  ProjectClockState state{ProjectClockState::stopped};
  ProjectTimeNs position_ns{0};
  ProjectTimeNs duration_ns{0};
  std::uint64_t frame_index{0};
  std::uint64_t frame_count{0};
  std::uint64_t loop_index{0};
  std::uint64_t epoch_id{0};
  std::uint64_t clock_source_generation{0};
  std::uint64_t sample_sequence{0};

  [[nodiscard]] bool running() const noexcept;
};

struct ProjectClockResult final {
  bool accepted{false};
  ProjectClockSnapshot snapshot;
  std::string code;
  std::string diagnostic;
};

// The sole mutable authority for project playback time. A ClockTick is only an
// external pulse; it cannot mutate ProjectTime except through this authority.
class ProjectClock final {
 public:
  explicit ProjectClock(ProjectClockSpec spec);

  [[nodiscard]] ProjectClockSpec spec() const noexcept;
  [[nodiscard]] ProjectClockSnapshot snapshot() const;
  [[nodiscard]] ProjectClockResult start(ClockTick tick);
  [[nodiscard]] ProjectClockResult play(ClockTick tick);
  [[nodiscard]] ProjectClockResult pause(ClockTick tick);
  [[nodiscard]] ProjectClockResult suspend();
  [[nodiscard]] ProjectClockResult stop();
  [[nodiscard]] ProjectClockResult seek_to_frame(std::uint64_t frame_index,
                                                 ClockTick tick);
  [[nodiscard]] ProjectClockResult sample(ClockTick tick);

 private:
  [[nodiscard]] ProjectClockSnapshot snapshot_locked() const noexcept;
  [[nodiscard]] ProjectClockResult accepted_locked(std::string code) const;
  [[nodiscard]] ProjectClockResult rejected_locked(
      std::string code, std::string diagnostic) const;
  [[nodiscard]] ProjectTimeNs absolute_position_locked() const noexcept;
  [[nodiscard]] bool begin_epoch_locked(ClockTick tick) noexcept;
  [[nodiscard]] bool sample_locked(ClockTick tick) noexcept;

  ProjectClockSpec spec_;
  mutable std::mutex mutex_;
  ProjectClockState state_{ProjectClockState::stopped};
  ProjectTimeNs position_ns_{0};
  ProjectTimeNs project_anchor_ns_{0};
  ClockTick clock_anchor_;
  std::uint64_t loop_index_{0};
  std::uint64_t epoch_id_{0};
  std::uint64_t sample_sequence_{0};
};

}  // namespace refusion::core
