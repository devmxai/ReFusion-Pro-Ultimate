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
  const double pixels = static_cast<double>(points) * static_cast<double>(scale);
  if (pixels > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
    return 0;
  }
  return static_cast<std::uint32_t>(std::lround(pixels));
}

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
  constexpr std::uint64_t nanoseconds_per_second = 1'000'000'000;
  return duration_ns != 0 && frame_rate_numerator != 0 &&
         frame_rate_denominator != 0 &&
         multiply_fits(nanoseconds_per_second, frame_rate_denominator) &&
         multiply_fits(duration_ns, frame_rate_numerator) &&
         frame_interval().count() > 0 && frame_count() > 0;
}

std::chrono::nanoseconds PlaybackSpec::frame_interval() const noexcept {
  if (frame_rate_numerator == 0 || frame_rate_denominator == 0) {
    return std::chrono::nanoseconds::zero();
  }
  constexpr std::uint64_t nanoseconds_per_second = 1'000'000'000;
  const auto scaled_second =
      nanoseconds_per_second * static_cast<std::uint64_t>(frame_rate_denominator);
  const auto rate = static_cast<std::uint64_t>(frame_rate_numerator);
  const auto quotient = scaled_second / rate;
  const auto remainder = scaled_second % rate;
  const auto interval = quotient +
                        static_cast<std::uint64_t>(
                            remainder >= (rate + 1) / 2);
  if (interval == 0 ||
      interval > static_cast<std::uint64_t>(
                     std::numeric_limits<std::chrono::nanoseconds::rep>::max())) {
    return std::chrono::nanoseconds::zero();
  }
  return std::chrono::nanoseconds(interval);
}

std::uint64_t PlaybackSpec::frame_count() const noexcept {
  constexpr std::uint64_t nanoseconds_per_second = 1'000'000'000;
  if (duration_ns == 0 || frame_rate_numerator == 0 ||
      frame_rate_denominator == 0 ||
      !multiply_fits(duration_ns, frame_rate_numerator) ||
      !multiply_fits(nanoseconds_per_second, frame_rate_denominator)) {
    return 0;
  }
  const auto scaled_duration =
      duration_ns * static_cast<std::uint64_t>(frame_rate_numerator);
  const auto frame_denominator =
      nanoseconds_per_second *
      static_cast<std::uint64_t>(frame_rate_denominator);
  return scaled_duration / frame_denominator +
         static_cast<std::uint64_t>(scaled_duration % frame_denominator != 0);
}

std::uint64_t PlaybackSpec::frame_at_time(
    const std::uint64_t position_ns) const noexcept {
  constexpr std::uint64_t nanoseconds_per_second = 1'000'000'000;
  if (frame_rate_numerator == 0 || frame_rate_denominator == 0 ||
      !multiply_fits(position_ns, frame_rate_numerator) ||
      !multiply_fits(nanoseconds_per_second, frame_rate_denominator)) {
    return 0;
  }
  const auto frame_denominator =
      nanoseconds_per_second *
      static_cast<std::uint64_t>(frame_rate_denominator);
  return (position_ns * static_cast<std::uint64_t>(frame_rate_numerator)) /
         frame_denominator;
}

std::uint64_t PlaybackSpec::time_at_frame(
    const std::uint64_t frame_index) const noexcept {
  constexpr std::uint64_t nanoseconds_per_second = 1'000'000'000;
  if (frame_rate_numerator == 0 || frame_rate_denominator == 0 ||
      !multiply_fits(frame_index, frame_rate_denominator)) {
    return 0;
  }
  const auto scaled_frame =
      frame_index * static_cast<std::uint64_t>(frame_rate_denominator);
  if (!multiply_fits(scaled_frame, nanoseconds_per_second)) {
    return 0;
  }
  const auto scaled_time = scaled_frame * nanoseconds_per_second;
  const auto rate = static_cast<std::uint64_t>(frame_rate_numerator);
  return scaled_time / rate +
         static_cast<std::uint64_t>(scaled_time % rate != 0);
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
      clock_now_(clock_now ? std::move(clock_now)
                           : [] { return std::chrono::steady_clock::now(); }),
      epoch_(clock_now_()),
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
    epoch_ = clock_now_();
    playback_origin_ns_ = 0;
    next_frame_index_ = 0;
    position_ns_ = 0;
    loop_index_ = 0;
    running_ = true;
    diagnostic_.clear();
    wake_.notify_all();
  }
  notify_frame_observer();
}

void ViewportRenderSession::resume_playback() {
  {
    std::scoped_lock lock(mutex_);
    if (running_) {
      return;
    }
    if (position_ns_ >= playback_spec_.duration_ns) {
      position_ns_ = 0;
      loop_index_ = 0;
    }
    const auto loop_offset =
        multiply_fits(loop_index_, playback_spec_.duration_ns)
            ? loop_index_ * playback_spec_.duration_ns
            : std::numeric_limits<std::uint64_t>::max();
    playback_origin_ns_ = saturating_add(loop_offset, position_ns_);
    epoch_ = clock_now_();
    running_ = true;
    diagnostic_.clear();
    wake_.notify_all();
  }
  notify_frame_observer();
}

void ViewportRenderSession::pause_playback() noexcept {
  {
    std::scoped_lock lock(mutex_);
    if (!running_) {
      return;
    }
    update_position_locked(clock_now_());
    running_ = false;
    wake_.notify_all();
  }
  notify_frame_observer();
}

void ViewportRenderSession::stop_playback() noexcept {
  {
    std::scoped_lock lock(mutex_);
    running_ = false;
    playback_origin_ns_ = 0;
    position_ns_ = 0;
    loop_index_ = 0;
    wake_.notify_all();
  }
  notify_frame_observer();
}

void ViewportRenderSession::update_position_locked(
    const std::chrono::steady_clock::time_point now) noexcept {
  if (!running_) {
    return;
  }
  const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
      now - epoch_);
  const auto elapsed_ns = elapsed.count() < 0
                              ? 0ULL
                              : static_cast<std::uint64_t>(elapsed.count());
  const auto absolute_ns = saturating_add(playback_origin_ns_, elapsed_ns);
  std::uint64_t unsnapped_position_ns = 0;
  if (playback_spec_.loop) {
    loop_index_ = absolute_ns / playback_spec_.duration_ns;
    unsnapped_position_ns = absolute_ns % playback_spec_.duration_ns;
  } else if (absolute_ns >= playback_spec_.duration_ns) {
    position_ns_ = playback_spec_.duration_ns;
    running_ = false;
    loop_index_ = 0;
    return;
  } else {
    unsnapped_position_ns = absolute_ns;
    loop_index_ = 0;
  }

  const auto frame_index = playback_spec_.frame_at_time(unsnapped_position_ns);
  position_ns_ = playback_spec_.time_at_frame(frame_index);
}

FixtureFrame ViewportRenderSession::current_frame_locked() noexcept {
  return FixtureFrame{
      .frame_index = next_frame_index_++,
      .presentation_time_ns = position_ns_,
      .duration_ns = playback_spec_.duration_ns,
      .loop_index = loop_index_,
  };
}

FixtureFrame ViewportRenderSession::next_frame_locked(
    const std::chrono::steady_clock::time_point now) noexcept {
  update_position_locked(now);
  return current_frame_locked();
}

FrameResult ViewportRenderSession::seek_to_frame(
    const std::uint64_t frame_index) {
  FrameResult result{.status = FrameStatus::accepted};
  {
    std::scoped_lock lock(mutex_);
    const auto total_frames = playback_spec_.frame_count();
    if (frame_index > total_frames) {
      return {
          .status = FrameStatus::rejected,
          .diagnostic = "RFX-TRANSPORT-SEEK-RANGE: frame is outside the composition",
      };
    }
    position_ns_ = frame_index == total_frames
                       ? playback_spec_.duration_ns
                       : playback_spec_.time_at_frame(frame_index);
    loop_index_ = 0;
    playback_origin_ns_ = position_ns_;
    epoch_ = clock_now_();
    diagnostic_.clear();
    if (attached_) {
      result = presenter_.present(current_frame_locked());
      last_frame_status_ = result.status;
      diagnostic_ = result.diagnostic;
      if (result.status == FrameStatus::rejected) {
        running_ = false;
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
      .code = accepted ? "RFX-TRANSPORT-ACCEPTED"
                       : "RFX-TRANSPORT-REJECTED",
      .diagnostic = accepted ? std::string{} : frame_result.diagnostic,
  };
}

FrameResult ViewportRenderSession::render_once() {
  FrameResult result;
  {
    std::scoped_lock lock(mutex_);
    result = presenter_.present(next_frame_locked(clock_now_()));
    last_frame_status_ = result.status;
    diagnostic_ = result.diagnostic;
    if (result.status == FrameStatus::rejected) {
      running_ = false;
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
  return PlaybackState{
      .running = running_,
      .position_ns = position_ns_,
      .duration_ns = playback_spec_.duration_ns,
      .frame_index = position_ns_ >= playback_spec_.duration_ns
                         ? playback_spec_.frame_count()
                         : playback_spec_.frame_at_time(position_ns_),
      .frame_count = playback_spec_.frame_count(),
      .loop_index = loop_index_,
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
        return running_ && attached_ && visible_;
      });
      if (stop_token.stop_requested()) {
        break;
      }
      const auto now = clock_now_();
      result = presenter_.present(next_frame_locked(now));
      last_frame_status_ = result.status;
      diagnostic_ = result.diagnostic;
      if (result.status == FrameStatus::rejected) {
        running_ = false;
      }
      next_deadline = now + frame_interval;
    }
    notify_frame_observer();

    std::unique_lock lock(mutex_);
    wake_.wait_until(lock, stop_token, next_deadline, [this] {
      return !running_ || !attached_ || !visible_;
    });
  }
}

}  // namespace refusion::runtime::presentation
