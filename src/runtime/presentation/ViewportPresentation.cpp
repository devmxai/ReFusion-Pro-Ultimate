#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace refusion::runtime::presentation {
namespace {

[[nodiscard]] std::uint32_t scaled_extent(const std::uint32_t points,
                                          const float scale) noexcept {
  if (points == 0 || !std::isfinite(scale) || scale <= 0.0F) {
    return 0;
  }
  const double pixels =
      static_cast<double>(points) * static_cast<double>(scale);
  if (pixels > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
    return 0;
  }
  return static_cast<std::uint32_t>(std::lround(pixels));
}

[[nodiscard]] core::ProjectClockSpec project_clock_spec(
    const PlaybackSpec& playback_spec) noexcept {
  return core::ProjectClockSpec{
      .duration_ns = playback_spec.duration_ns,
      .frame_rate =
          core::RationalRate{
              .numerator = playback_spec.frame_rate_numerator,
              .denominator = playback_spec.frame_rate_denominator,
          },
      .loop = playback_spec.loop,
  };
}

}  // namespace

bool ViewportExtent::valid() const noexcept {
  return width_pixels() != 0 && height_pixels() != 0;
}

std::uint32_t ViewportExtent::width_pixels() const noexcept {
  return scaled_extent(width_points, pixels_per_point);
}

std::uint32_t ViewportExtent::height_pixels() const noexcept {
  return scaled_extent(height_points, pixels_per_point);
}

bool NativeViewportHost::valid() const noexcept { return handle != 0; }

bool NativeFrameTarget::valid() const noexcept {
  return texture != 0 && width_pixels != 0 && height_pixels != 0 &&
         device_generation != 0;
}

bool PlaybackSpec::valid() const noexcept {
  return project_clock_spec(*this).valid();
}

std::chrono::nanoseconds PlaybackSpec::frame_interval() const noexcept {
  const auto interval = project_clock_spec(*this).frame_interval_ns();
  if (interval == 0 ||
      interval >
          static_cast<std::uint64_t>(
              std::numeric_limits<std::chrono::nanoseconds::rep>::max())) {
    return std::chrono::nanoseconds::zero();
  }
  return std::chrono::nanoseconds(interval);
}

std::uint64_t PlaybackSpec::frame_count() const noexcept {
  return project_clock_spec(*this).frame_count();
}

std::uint64_t PlaybackSpec::frame_at_time(
    const std::uint64_t position_ns) const noexcept {
  return project_clock_spec(*this).frame_at_time(position_ns);
}

std::uint64_t PlaybackSpec::time_at_frame(
    const std::uint64_t frame_index) const noexcept {
  return project_clock_spec(*this).time_at_frame(frame_index);
}

bool FrameResult::succeeded() const noexcept {
  return status == FrameStatus::accepted || status == FrameStatus::presented;
}

bool PresentationTelemetry::zero_cpu_pixel_transfer() const noexcept {
  return cpu_pixel_maps == 0 && cpu_pixel_uploads == 0 && gpu_readbacks == 0 &&
         unattributed_gpu_copies == 0;
}

ViewportRenderSession::ViewportRenderSession(ViewportPresenter& presenter,
                                             PlaybackSpec playback_spec,
                                             ClockNow clock_now)
    : presenter_(presenter),
      playback_spec_(playback_spec),
      project_clock_(project_clock_spec(playback_spec)),
      clock_now_(clock_now ? std::move(clock_now)
                           : [] { return std::chrono::steady_clock::now(); }),
      render_thread_([this](const std::stop_token stop_token) {
        render_loop(stop_token);
      }) {
  if (!playback_spec_.valid()) {
    throw std::invalid_argument("viewport playback specification is invalid");
  }
}

ViewportRenderSession::~ViewportRenderSession() {
  render_thread_.request_stop();
  wake_.notify_all();
  if (render_thread_.joinable()) {
    render_thread_.join();
  }
  set_frame_observer({});
}

FrameResult ViewportRenderSession::attach(const NativeViewportHost host) {
  std::scoped_lock lock(mutex_);
  auto result = presenter_.attach(host);
  attached_ = result.succeeded();
  last_frame_status_ = result.status;
  diagnostic_ = result.diagnostic;
  wake_.notify_all();
  return result;
}

void ViewportRenderSession::detach() noexcept {
  std::scoped_lock lock(mutex_);
  attached_ = false;
  visible_ = false;
  presenter_.detach();
  wake_.notify_all();
}

FrameResult ViewportRenderSession::resize(const ViewportExtent extent) {
  std::scoped_lock lock(mutex_);
  auto result = presenter_.resize(extent);
  last_frame_status_ = result.status;
  diagnostic_ = result.diagnostic;
  wake_.notify_all();
  return result;
}

void ViewportRenderSession::set_visible(const bool visible) noexcept {
  std::scoped_lock lock(mutex_);
  visible_ = visible;
  presenter_.set_visible(visible);
  wake_.notify_all();
}

void ViewportRenderSession::start_playback() {
  {
    std::scoped_lock lock(mutex_);
    next_frame_index_ = 0;
    const auto result = project_clock_.start(clock_tick(clock_now_()));
    diagnostic_ = result.accepted ? std::string{} : result.diagnostic;
    wake_.notify_all();
  }
  notify_frame_observer();
}

void ViewportRenderSession::resume_playback() {
  {
    std::scoped_lock lock(mutex_);
    const auto result = project_clock_.play(clock_tick(clock_now_()));
    diagnostic_ = result.accepted ? std::string{} : result.diagnostic;
    wake_.notify_all();
  }
  notify_frame_observer();
}

void ViewportRenderSession::pause_playback() {
  {
    std::scoped_lock lock(mutex_);
    const auto result = project_clock_.pause(clock_tick(clock_now_()));
    diagnostic_ = result.accepted ? std::string{} : result.diagnostic;
    wake_.notify_all();
  }
  notify_frame_observer();
}

void ViewportRenderSession::stop_playback() {
  {
    std::scoped_lock lock(mutex_);
    const auto result = project_clock_.stop();
    diagnostic_ = result.accepted ? std::string{} : result.diagnostic;
    wake_.notify_all();
  }
  notify_frame_observer();
}

core::ClockTick ViewportRenderSession::clock_tick(
    const std::chrono::steady_clock::time_point now) const noexcept {
  const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
      now.time_since_epoch());
  if (elapsed.count() < 0) {
    return {};
  }
  return core::ClockTick{
      .nanoseconds = static_cast<std::uint64_t>(elapsed.count()),
      .source_generation = 1,
  };
}

FixtureFrame ViewportRenderSession::next_frame_locked(
    const core::ProjectClockSnapshot& clock_snapshot) noexcept {
  return FixtureFrame{
      .frame_index = next_frame_index_++,
      .presentation_time_ns = clock_snapshot.position_ns,
      .duration_ns = playback_spec_.duration_ns,
      .loop_index = clock_snapshot.loop_index,
      .transport_epoch_id = clock_snapshot.epoch_id,
  };
}

FrameResult ViewportRenderSession::seek_to_frame(
    const std::uint64_t frame_index) {
  FrameResult result{.status = FrameStatus::accepted};
  {
    std::scoped_lock lock(mutex_);
    const auto now = clock_now_();
    const auto clock_result =
        project_clock_.seek_to_frame(frame_index, clock_tick(now));
    if (!clock_result.accepted) {
      return {
          .status = FrameStatus::rejected,
          .diagnostic = "RFX-TRANSPORT-SEEK-RANGE: " + clock_result.code +
                        ": " + clock_result.diagnostic,
      };
    }
    diagnostic_.clear();
    if (attached_) {
      result = presenter_.present(next_frame_locked(clock_result.snapshot));
      last_frame_status_ = result.status;
      diagnostic_ = result.diagnostic;
      if (result.status == FrameStatus::rejected) {
        static_cast<void>(project_clock_.pause(clock_tick(now)));
      }
    }
    wake_.notify_all();
  }
  notify_frame_observer();
  return result;
}

TransportCommandResult ViewportRenderSession::submit_transport_command(
    const TransportCommand command) {
  FrameResult frame_result{.status = FrameStatus::accepted};
  switch (command.kind) {
    case TransportCommandKind::play:
      resume_playback();
      break;
    case TransportCommandKind::pause:
      pause_playback();
      break;
    case TransportCommandKind::seek_to_frame:
      frame_result = seek_to_frame(command.frame_index);
      break;
  }

  const auto state = playback_state();
  const bool accepted = frame_result.status != FrameStatus::rejected;
  return TransportCommandResult{
      .accepted = accepted,
      .snapshot = state,
      .code = accepted ? "RFX-TRANSPORT-ACCEPTED" : "RFX-TRANSPORT-REJECTED",
      .diagnostic = accepted ? std::string{} : frame_result.diagnostic,
  };
}

FrameResult ViewportRenderSession::render_once() {
  FrameResult result;
  {
    std::scoped_lock lock(mutex_);
    const auto now = clock_now_();
    const auto clock_result = project_clock_.sample(clock_tick(now));
    if (!clock_result.accepted) {
      diagnostic_ = clock_result.code + ": " + clock_result.diagnostic;
      static_cast<void>(project_clock_.suspend());
      return {
          .status = FrameStatus::rejected,
          .diagnostic = diagnostic_,
      };
    }
    result = presenter_.present(next_frame_locked(clock_result.snapshot));
    last_frame_status_ = result.status;
    diagnostic_ = result.diagnostic;
    if (result.status == FrameStatus::rejected) {
      static_cast<void>(project_clock_.pause(clock_tick(now)));
    }
  }
  notify_frame_observer();
  return result;
}

PresentationTelemetry ViewportRenderSession::telemetry() const noexcept {
  std::scoped_lock lock(mutex_);
  return presenter_.telemetry();
}

PlaybackState ViewportRenderSession::playback_state() const {
  std::scoped_lock lock(mutex_);
  const auto clock_snapshot = project_clock_.snapshot();
  return PlaybackState{
      .running = clock_snapshot.running(),
      .position_ns = clock_snapshot.position_ns,
      .duration_ns = clock_snapshot.duration_ns,
      .frame_index = clock_snapshot.frame_index,
      .frame_count = clock_snapshot.frame_count,
      .loop_index = clock_snapshot.loop_index,
      .clock_epoch_id = clock_snapshot.epoch_id,
      .clock_source_generation = clock_snapshot.clock_source_generation,
      .clock_sample_sequence = clock_snapshot.sample_sequence,
      .last_frame_status = last_frame_status_,
      .diagnostic = diagnostic_,
  };
}

void ViewportRenderSession::set_frame_observer(FrameObserver observer) {
  std::scoped_lock lock(observer_mutex_);
  frame_observer_ = std::move(observer);
}

void ViewportRenderSession::notify_frame_observer() {
  std::scoped_lock lock(observer_mutex_);
  if (frame_observer_) {
    frame_observer_();
  }
}

void ViewportRenderSession::render_loop(const std::stop_token stop_token) {
  const auto frame_interval = playback_spec_.frame_interval();
  while (!stop_token.stop_requested()) {
    FrameResult result;
    std::chrono::steady_clock::time_point next_deadline;
    {
      std::unique_lock lock(mutex_);
      wake_.wait(lock, stop_token, [this] {
        return project_clock_.snapshot().running() && attached_ && visible_;
      });
      if (stop_token.stop_requested()) {
        break;
      }
      const auto now = clock_now_();
      const auto clock_result = project_clock_.sample(clock_tick(now));
      if (!clock_result.accepted) {
        last_frame_status_ = FrameStatus::rejected;
        diagnostic_ = clock_result.code + ": " + clock_result.diagnostic;
        static_cast<void>(project_clock_.suspend());
        continue;
      }
      result = presenter_.present(next_frame_locked(clock_result.snapshot));
      last_frame_status_ = result.status;
      diagnostic_ = result.diagnostic;
      if (result.status == FrameStatus::rejected) {
        static_cast<void>(project_clock_.pause(clock_tick(now)));
      }
      next_deadline = now + frame_interval;
    }
    notify_frame_observer();

    std::unique_lock lock(mutex_);
    wake_.wait_until(lock, stop_token, next_deadline, [this] {
      return !project_clock_.snapshot().running() || !attached_ || !visible_;
    });
  }
}

}  // namespace refusion::runtime::presentation
