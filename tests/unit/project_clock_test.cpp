#include <source_location>
#include <stdexcept>
#include <string>

#include "refusion/core/ProjectClock.hpp"

namespace {

void require(const bool condition, const std::source_location where =
                                       std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error("project clock test failed at line " +
                             std::to_string(where.line()));
  }
}

}  // namespace

int main() {
  using namespace refusion::core;

  const ProjectClockSpec spec{
      .duration_ns = 30'000'000'000,
      .frame_rate = RationalRate{.numerator = 30, .denominator = 1},
      .loop = true,
  };
  require(spec.valid());
  require(spec.frame_interval_ns() == 33'333'333);
  require(spec.frame_count() == 900);
  require(spec.frame_at_time(10'000'000'000) == 300);
  require(spec.time_at_frame(300) == 10'000'000'000);

  ProjectClock clock(spec);
  auto state = clock.snapshot();
  require(state.state == ProjectClockState::stopped);
  require(state.epoch_id == 0);
  require(state.position_ns == 0);

  const ClockTick origin{.nanoseconds = 1'000, .source_generation = 7};
  const auto started = clock.start(origin);
  require(started.accepted);
  require(started.snapshot.running());
  require(started.snapshot.epoch_id == 1);
  require(started.snapshot.clock_source_generation == 7);

  const auto ten_seconds = clock.sample({
      .nanoseconds = 10'000'001'000,
      .source_generation = 7,
  });
  require(ten_seconds.accepted);
  require(ten_seconds.snapshot.position_ns == 10'000'000'000);
  require(ten_seconds.snapshot.frame_index == 300);

  const auto backwards = clock.sample({
      .nanoseconds = 999,
      .source_generation = 7,
  });
  require(!backwards.accepted);
  require(backwards.code == "RFX-CLOCK-SOURCE-DISCONTINUITY");
  require(backwards.snapshot.position_ns == 10'000'000'000);

  const auto wrong_source = clock.sample({
      .nanoseconds = 10'000'001'001,
      .source_generation = 8,
  });
  require(!wrong_source.accepted);
  require(wrong_source.snapshot.clock_source_generation == 7);

  const auto paused = clock.pause({
      .nanoseconds = 10'000'001'000,
      .source_generation = 7,
  });
  require(paused.accepted);
  require(paused.snapshot.state == ProjectClockState::paused);
  require(paused.snapshot.epoch_id == 2);

  const auto paused_sample = clock.sample({
      .nanoseconds = 20'000'001'000,
      .source_generation = 7,
  });
  require(paused_sample.accepted);
  require(paused_sample.snapshot.position_ns == 10'000'000'000);

  const auto seeked = clock.seek_to_frame(
      450, {.nanoseconds = 20'000'001'000, .source_generation = 7});
  require(seeked.accepted);
  require(seeked.snapshot.position_ns == 15'000'000'000);
  require(seeked.snapshot.frame_index == 450);
  require(seeked.snapshot.epoch_id == 3);

  const auto rejected_seek = clock.seek_to_frame(
      901, {.nanoseconds = 20'000'001'000, .source_generation = 7});
  require(!rejected_seek.accepted);
  require(rejected_seek.code == "RFX-CLOCK-SEEK-RANGE");
  require(rejected_seek.snapshot.position_ns == 15'000'000'000);

  const auto resumed = clock.play({
      .nanoseconds = 20'000'001'000,
      .source_generation = 7,
  });
  require(resumed.accepted);
  require(resumed.snapshot.running());
  require(resumed.snapshot.epoch_id == 4);

  const auto looped = clock.sample({
      .nanoseconds = 36'000'001'000,
      .source_generation = 7,
  });
  require(looped.accepted);
  require(looped.snapshot.position_ns == 1'000'000'000);
  require(looped.snapshot.loop_index == 1);

  const auto loop_paused = clock.pause({
      .nanoseconds = 36'000'001'000,
      .source_generation = 7,
  });
  require(loop_paused.accepted);
  const auto loop_resumed = clock.play({
      .nanoseconds = 40'000'001'000,
      .source_generation = 7,
  });
  require(loop_resumed.accepted);
  const auto after_resume = clock.sample({
      .nanoseconds = 41'000'001'000,
      .source_generation = 7,
  });
  require(after_resume.accepted);
  require(after_resume.snapshot.position_ns == 2'000'000'000);
  require(after_resume.snapshot.loop_index == 1);

  const auto stopped = clock.stop();
  require(stopped.accepted);
  require(stopped.snapshot.state == ProjectClockState::stopped);
  require(stopped.snapshot.position_ns == 0);
  require(stopped.snapshot.clock_source_generation == 0);

  const ProjectClockSpec ntsc{
      .duration_ns = 10'000'000'000,
      .frame_rate = RationalRate{.numerator = 30'000, .denominator = 1'001},
      .loop = false,
  };
  require(ntsc.valid());
  require(ntsc.frame_interval_ns() == 33'366'667);
  require(ntsc.frame_count() == 300);
  require(ntsc.time_at_frame(1) == 33'366'667);
  require(ntsc.frame_at_time(33'366'666) == 0);
  require(ntsc.frame_at_time(33'366'667) == 1);
}
