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
  return duration_ns != 0 && frame_rate_numerator != 0 &&
         frame_rate_denominator != 0 && frame_interval().count() > 0;
}

std::chrono::nanoseconds PlaybackSpec::frame_interval() const noexcept {
  if (frame_rate_numerator == 0 || frame_rate_denominator == 0) {
    return std::chrono::nanoseconds::zero();
  }
  constexpr std::uint64_t nanoseconds_per_second = 1'000'000'000;
  const auto scaled_second =
      nanoseconds_per_second * static_cast<std::uint64_t>(frame_rate_denominator);
  const auto interval =
      scaled_second / static_cast<std::uint64_t>(frame_rate_numerator);
  if (interval == 0 ||
      interval > static_cast<std::uint64_t>(
                     std::numeric_limits<std::chrono::nanoseconds::rep>::max())) {
    return std::chrono::nanoseconds::zero();
  }
  return std::chrono::nanoseconds(interval);
}

bool FrameResult::succeeded() const noexcept {
  return status == FrameStatus::accepted || status == FrameStatus::presented;
}

bool PresentationTelemetry::zero_cpu_pixel_transfer() const noexcept {
  return cpu_pixel_maps == 0 && cpu_pixel_uploads == 0 && gpu_readbacks == 0 &&
         unattributed_gpu_copies == 0;
}

ViewportRenderSession::ViewportRenderSession(ViewportPresenter& presenter,
                                             PlaybackSpec playback_spec)
    : presenter_(presenter),
      playback_spec_(playback_spec),
      epoch_(std::chrono::steady_clock::now()),
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
  std::scoped_lock lock(mutex_);
  epoch_ = std::chrono::steady_clock::now();
  next_frame_index_ = 0;
  position_ns_ = 0;
  loop_index_ = 0;
  running_ = true;
  diagnostic_.clear();
  wake_.notify_all();
}

void ViewportRenderSession::stop_playback() noexcept {
  std::scoped_lock lock(mutex_);
  running_ = false;
  wake_.notify_all();
}

FixtureFrame ViewportRenderSession::next_frame_locked(
    const std::chrono::steady_clock::time_point now) noexcept {
  const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
      now - epoch_);
  const auto elapsed_ns = elapsed.count() < 0
                              ? 0ULL
                              : static_cast<std::uint64_t>(elapsed.count());
  loop_index_ = elapsed_ns / playback_spec_.duration_ns;
  position_ns_ = elapsed_ns % playback_spec_.duration_ns;
  if (!playback_spec_.loop && elapsed_ns >= playback_spec_.duration_ns) {
    position_ns_ = playback_spec_.duration_ns;
    running_ = false;
  }
  return FixtureFrame{
      .frame_index = next_frame_index_++,
      .presentation_time_ns = position_ns_,
      .duration_ns = playback_spec_.duration_ns,
      .loop_index = loop_index_,
  };
}

FrameResult ViewportRenderSession::render_once() {
  FrameResult result;
  {
    std::scoped_lock lock(mutex_);
    result = presenter_.present(next_frame_locked(std::chrono::steady_clock::now()));
    last_frame_status_ = result.status;
    diagnostic_ = result.diagnostic;
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
      const auto now = std::chrono::steady_clock::now();
      result = presenter_.present(next_frame_locked(now));
      last_frame_status_ = result.status;
      diagnostic_ = result.diagnostic;
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
