#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "refusion/core/ProjectClock.hpp"
#include "refusion/runtime/gpu/GpuDeviceService.hpp"

namespace refusion::runtime::presentation {

enum class NativeWindowSystem : std::uint8_t {
  cocoa_view,
  win32_hwnd,
  android_native_window,
  ui_view,
};

enum class PixelFormat : std::uint8_t {
  bgra8_unorm,
};

enum class FrameStatus : std::uint8_t {
  accepted,
  presented,
  skipped,
  rejected,
};

struct ViewportExtent final {
  std::uint32_t width_points{0};
  std::uint32_t height_points{0};
  float pixels_per_point{1.0F};

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] std::uint32_t width_pixels() const noexcept;
  [[nodiscard]] std::uint32_t height_pixels() const noexcept;
};

struct NativeViewportHost final {
  NativeWindowSystem window_system{NativeWindowSystem::cocoa_view};
  std::uintptr_t handle{0};

  [[nodiscard]] bool valid() const noexcept;
};

struct NativeFrameTarget final {
  gpu::Backend backend{gpu::Backend::metal};
  PixelFormat pixel_format{PixelFormat::bgra8_unorm};
  std::uintptr_t texture{0};
  std::uint32_t width_pixels{0};
  std::uint32_t height_pixels{0};
  std::uint64_t device_generation{0};

  [[nodiscard]] bool valid() const noexcept;
};

struct FixtureFrame final {
  std::uint64_t frame_index{0};
  std::uint64_t presentation_time_ns{0};
  std::uint64_t duration_ns{0};
  std::uint64_t loop_index{0};
  std::uint64_t transport_epoch_id{0};
};

struct PlaybackSpec final {
  std::uint64_t duration_ns{30'000'000'000};
  std::uint32_t frame_rate_numerator{30};
  std::uint32_t frame_rate_denominator{1};
  bool loop{true};

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] std::chrono::nanoseconds frame_interval() const noexcept;
  [[nodiscard]] std::uint64_t frame_count() const noexcept;
  [[nodiscard]] std::uint64_t frame_at_time(
      std::uint64_t position_ns) const noexcept;
  [[nodiscard]] std::uint64_t time_at_frame(
      std::uint64_t frame_index) const noexcept;
};

struct PlaybackState final {
  bool running{false};
  std::uint64_t position_ns{0};
  std::uint64_t duration_ns{0};
  std::uint64_t frame_index{0};
  std::uint64_t frame_count{0};
  std::uint64_t loop_index{0};
  std::uint64_t clock_epoch_id{0};
  std::uint64_t clock_source_generation{0};
  std::uint64_t clock_sample_sequence{0};
  FrameStatus last_frame_status{FrameStatus::skipped};
  std::string diagnostic;
};

enum class TransportCommandKind : std::uint8_t {
  play,
  pause,
  seek_to_frame,
};

struct TransportCommand final {
  TransportCommandKind kind{TransportCommandKind::play};
  std::uint64_t frame_index{0};
};

struct TransportCommandResult final {
  bool accepted{false};
  PlaybackState snapshot;
  std::string code;
  std::string diagnostic;
};

struct FrameResult final {
  FrameStatus status{FrameStatus::rejected};
  std::string diagnostic;

  [[nodiscard]] bool succeeded() const noexcept;
};

struct PresentationTelemetry final {
  std::uint64_t device_generation{0};
  gpu::DeviceStatus device_status{gpu::DeviceStatus::lost};
  std::uint64_t device_event_sequence{0};
  std::uint64_t frame_requests{0};
  std::uint64_t drawable_acquisitions{0};
  std::uint64_t renderer_submissions{0};
  std::uint64_t present_submissions{0};
  std::uint64_t skipped_frames{0};
  std::uint64_t rejected_frames{0};
  std::uint64_t visibility_suspends{0};
  std::uint64_t visibility_resumes{0};
  std::uint64_t occlusion_suspends{0};
  std::uint64_t occlusion_resumes{0};
  std::uint64_t occluded_frames{0};
  std::uint64_t device_suspended_frames{0};
  std::uint64_t device_loss_rejections{0};
  std::uint64_t stale_generation_rejections{0};
  std::uint64_t cpu_pixel_maps{0};
  std::uint64_t cpu_pixel_uploads{0};
  std::uint64_t gpu_readbacks{0};
  std::uint64_t unattributed_gpu_copies{0};

  [[nodiscard]] bool zero_cpu_pixel_transfer() const noexcept;
};

class ViewportFrameRenderer {
 public:
  virtual ~ViewportFrameRenderer() = default;

  [[nodiscard]] virtual const gpu::DeviceIdentity& device_identity()
      const noexcept = 0;
  [[nodiscard]] virtual FrameResult render(const NativeFrameTarget& target,
                                           const FixtureFrame& frame) = 0;
};

class ViewportPresenter {
 public:
  virtual ~ViewportPresenter() = default;

  [[nodiscard]] virtual FrameResult attach(NativeViewportHost host) = 0;
  virtual void detach() noexcept = 0;
  [[nodiscard]] virtual FrameResult resize(ViewportExtent extent) = 0;
  virtual void set_visible(bool visible) noexcept = 0;
  [[nodiscard]] virtual FrameResult present(const FixtureFrame& frame) = 0;
  [[nodiscard]] virtual PresentationTelemetry telemetry() const noexcept = 0;
};

class ViewportRenderSession final {
 public:
  using FrameObserver = std::function<void()>;
  using ClockNow = std::function<std::chrono::steady_clock::time_point()>;

  explicit ViewportRenderSession(ViewportPresenter& presenter,
                                 PlaybackSpec playback_spec = {},
                                 ClockNow clock_now = {});
  ~ViewportRenderSession();

  ViewportRenderSession(const ViewportRenderSession&) = delete;
  ViewportRenderSession& operator=(const ViewportRenderSession&) = delete;

  [[nodiscard]] FrameResult attach(NativeViewportHost host);
  void detach() noexcept;
  [[nodiscard]] FrameResult resize(ViewportExtent extent);
  void set_visible(bool visible) noexcept;
  void start_playback();
  void resume_playback();
  void pause_playback();
  void stop_playback();
  [[nodiscard]] FrameResult seek_to_frame(std::uint64_t frame_index);
  [[nodiscard]] TransportCommandResult submit_transport_command(
      TransportCommand command);
  [[nodiscard]] FrameResult render_once();
  [[nodiscard]] PresentationTelemetry telemetry() const noexcept;
  [[nodiscard]] PlaybackState playback_state() const;
  void set_frame_observer(FrameObserver observer);

 private:
  [[nodiscard]] FixtureFrame next_frame_locked(
      const core::ProjectClockSnapshot& clock_snapshot) noexcept;
  [[nodiscard]] core::ClockTick clock_tick(
      std::chrono::steady_clock::time_point now) const noexcept;
  void render_loop(std::stop_token stop_token);
  void notify_frame_observer();

  ViewportPresenter& presenter_;
  PlaybackSpec playback_spec_;
  core::ProjectClock project_clock_;
  mutable std::mutex mutex_;
  std::condition_variable_any wake_;
  ClockNow clock_now_;
  std::uint64_t next_frame_index_{0};
  FrameStatus last_frame_status_{FrameStatus::skipped};
  std::string diagnostic_;
  bool attached_{false};
  bool visible_{false};

  std::mutex observer_mutex_;
  FrameObserver frame_observer_;
  std::jthread render_thread_;
};

}  // namespace refusion::runtime::presentation
