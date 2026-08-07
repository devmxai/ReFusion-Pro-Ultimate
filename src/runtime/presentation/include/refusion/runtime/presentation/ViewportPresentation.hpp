#pragma once

#include "refusion/runtime/gpu/GpuDeviceService.hpp"

#include <cstdint>
#include <chrono>
#include <string>

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
};

struct FrameResult final {
  FrameStatus status{FrameStatus::rejected};
  std::string diagnostic;

  [[nodiscard]] bool succeeded() const noexcept;
};

struct PresentationTelemetry final {
  std::uint64_t device_generation{0};
  std::uint64_t frame_requests{0};
  std::uint64_t drawable_acquisitions{0};
  std::uint64_t renderer_submissions{0};
  std::uint64_t present_submissions{0};
  std::uint64_t skipped_frames{0};
  std::uint64_t rejected_frames{0};
  std::uint64_t cpu_pixel_maps{0};
  std::uint64_t cpu_pixel_uploads{0};
  std::uint64_t gpu_readbacks{0};
  std::uint64_t unattributed_gpu_copies{0};

  [[nodiscard]] bool zero_cpu_pixel_transfer() const noexcept;
};

class ViewportFrameRenderer {
 public:
  virtual ~ViewportFrameRenderer() = default;

  [[nodiscard]] virtual const gpu::DeviceIdentity& device_identity() const noexcept = 0;
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
  explicit ViewportRenderSession(ViewportPresenter& presenter) noexcept;

  [[nodiscard]] FrameResult attach(NativeViewportHost host);
  void detach() noexcept;
  [[nodiscard]] FrameResult resize(ViewportExtent extent);
  void set_visible(bool visible) noexcept;
  [[nodiscard]] FrameResult render_once();
  [[nodiscard]] PresentationTelemetry telemetry() const noexcept;

 private:
  ViewportPresenter& presenter_;
  std::chrono::steady_clock::time_point epoch_;
  std::uint64_t next_frame_index_{0};
};

}  // namespace refusion::runtime::presentation
