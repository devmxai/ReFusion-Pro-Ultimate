#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "refusion/core/ProjectClock.hpp"
#include "refusion/runtime/gpu/GpuDeviceService.hpp"
#include "refusion/runtime/render/VisualOutputContract.hpp"
#include "refusion/runtime/render/ViewportMapping.hpp"

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

enum class FrameFailureKind : std::uint8_t {
  none,
  unavailable,
  incompatible,
  occluded,
  timed_out,
  device_lost,
  stale_generation,
};

struct ViewportExtent final {
  std::uint32_t width_points{0};
  std::uint32_t height_points{0};
  float pixels_per_point{1.0F};

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] std::uint32_t width_pixels() const noexcept;
  [[nodiscard]] std::uint32_t height_pixels() const noexcept;
};

// Lifetime-bearing native host acquired by a platform adapter. Studio passes
// only this lease; no raw window/view handle crosses into Runtime.
struct NativeViewportHostLease final {
  NativeWindowSystem window_system{NativeWindowSystem::cocoa_view};
  std::uint64_t host_id{0};
  std::shared_ptr<const void> backend_private_state;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] const void* backend_private_host() const noexcept;
};

// One acquired backend render target with explicit identity, extent, format,
// device generation and lifetime. Native API state is type-erased; only the
// matching native renderer bridge may inspect it.
struct BackendFrameTargetLease final {
  gpu::DeviceIdentity device;
  PixelFormat pixel_format{PixelFormat::bgra8_unorm};
  std::uint64_t target_id{0};
  std::uint32_t width_pixels{0};
  std::uint32_t height_pixels{0};
  std::shared_ptr<const void> backend_private_state;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] const void* backend_private_target() const noexcept;
};

// One coherent immutable rendering request. The program lease binds accepted
// project/revision/composition identity; exact Core time/epoch and device
// generation complete the EvaluationStamp used by the common renderer.
struct PresentationFrameRequest final {
  std::uint64_t request_sequence{0};
  core::ProjectTimeNs project_time_ns{0};
  std::uint64_t loop_index{0};
  std::uint64_t transport_epoch_id{0};
  gpu::DeviceIdentity device;
  render::VisualOutputConsumer output_consumer{
      render::VisualOutputConsumer::interactive_preview};
  render::CanvasViewportState canvas_view;
  std::shared_ptr<const render::VisualRenderProgram> render_program;

  [[nodiscard]] bool valid() const noexcept;
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
  FrameFailureKind failure{FrameFailureKind::none};
  std::string code;
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
  std::uint64_t native_wait_timeouts{0};
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
  [[nodiscard]] virtual FrameResult render(
      const BackendFrameTargetLease& target,
      const PresentationFrameRequest& frame) = 0;
  // Presenters call this after native GPU completion and before invalidating
  // frame targets. The renderer must release completed backend references.
  [[nodiscard]] virtual FrameResult retire_frame_targets() = 0;
};

class ViewportPresenter {
 public:
  virtual ~ViewportPresenter() = default;

  [[nodiscard]] virtual gpu::DeviceIdentity device_identity() const noexcept = 0;
  [[nodiscard]] virtual FrameResult attach(NativeViewportHostLease host) = 0;
  virtual void detach() noexcept = 0;
  [[nodiscard]] virtual FrameResult resize(ViewportExtent extent) = 0;
  virtual void set_visible(bool visible) noexcept = 0;
  [[nodiscard]] virtual FrameResult present(
      const PresentationFrameRequest& frame) = 0;
  [[nodiscard]] virtual PresentationTelemetry telemetry() const noexcept = 0;
};

class ViewportRenderSession final {
 public:
  using FrameObserver = std::function<void()>;
  using ClockNow = std::function<std::chrono::steady_clock::time_point()>;

  explicit ViewportRenderSession(ViewportPresenter& presenter,
                                 PlaybackSpec playback_spec = {},
                                 ClockNow clock_now = {},
                                 std::shared_ptr<const
                                     render::VisualRenderProgram>
                                     render_program = nullptr);
  ~ViewportRenderSession();

  ViewportRenderSession(const ViewportRenderSession&) = delete;
  ViewportRenderSession& operator=(const ViewportRenderSession&) = delete;

  [[nodiscard]] FrameResult attach(NativeViewportHostLease host);
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
  [[nodiscard]] render::CanvasViewportState canvas_view() const noexcept;
  [[nodiscard]] bool set_canvas_view(
      render::CanvasViewportState canvas_view) noexcept;
  void set_frame_observer(FrameObserver observer);
  void publish_render_program(
      std::shared_ptr<const render::VisualRenderProgram> render_program) noexcept;

 private:
  [[nodiscard]] PresentationFrameRequest next_frame_locked(
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
  render::CanvasViewportState canvas_view_;
  std::shared_ptr<const render::VisualRenderProgram> render_program_;
  FrameStatus last_frame_status_{FrameStatus::skipped};
  std::string diagnostic_;
  bool attached_{false};
  bool visible_{false};

  std::mutex observer_mutex_;
  FrameObserver frame_observer_;
  std::jthread render_thread_;
};

}  // namespace refusion::runtime::presentation
