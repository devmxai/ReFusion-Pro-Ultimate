#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "TestComposition.hpp"
#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformViewportPresenter.hpp"

#include <windows.h>

#include <cstdint>
#include <memory>
#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error(
        "D3D12 viewport presenter test requirement failed");
  }
}

LRESULT CALLBACK test_window_proc(HWND window, UINT message, WPARAM wparam,
                                  LPARAM lparam) {
  return DefWindowProcW(window, message, wparam, lparam);
}

class TestWindow final {
 public:
  TestWindow() {
    instance_ = GetModuleHandleW(nullptr);
    WNDCLASSEXW window_class{
        .cbSize = sizeof(WNDCLASSEXW),
        .style = CS_OWNDC,
        .lpfnWndProc = test_window_proc,
        .hInstance = instance_,
        .lpszClassName = L"ReFusion.D3D12PresenterTest",
    };
    atom_ = RegisterClassExW(&window_class);
    require(atom_ != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS);
    window_ = CreateWindowExW(
        0, window_class.lpszClassName, L"ReFusion D3D12 Presenter Test",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 360, nullptr,
        nullptr, instance_, nullptr);
    require(window_ != nullptr);
    ShowWindow(window_, SW_SHOWNA);
  }

  ~TestWindow() {
    if (window_ != nullptr) {
      DestroyWindow(window_);
    }
    if (atom_ != 0) {
      UnregisterClassW(MAKEINTATOM(atom_), instance_);
    }
  }

  [[nodiscard]] HWND get() const noexcept { return window_; }

 private:
  HINSTANCE instance_{nullptr};
  ATOM atom_{0};
  HWND window_{nullptr};
};

}  // namespace

int main() {
  using namespace refusion::runtime::presentation;

  TestWindow window;
  auto device_service = refusion::platform::create_platform_gpu_device_service();
  require(device_service->identity().backend ==
          refusion::runtime::gpu::Backend::direct3d12);
  auto render_program = std::make_shared<const
      refusion::runtime::render::VisualRenderProgram>(test_render_program());
  auto renderer = refusion::adapters::skia::SkiaGpuContexts::create(
      device_service->borrow());
  auto presenter = refusion::platform::create_platform_viewport_presenter(
      *device_service, *renderer);

  auto host = refusion::platform::acquire_platform_viewport_host(
      reinterpret_cast<std::uintptr_t>(window.get()));
  auto wrong_host = host;
  wrong_host.window_system = NativeWindowSystem::cocoa_view;
  require(presenter->attach(wrong_host).status == FrameStatus::rejected);
  require(presenter->attach(host).succeeded());
  require(presenter->resize(ViewportExtent{
      .width_points = 640,
      .height_points = 360,
      .pixels_per_point = 1.0F,
  }).succeeded());
  presenter->set_visible(true);

  const auto request = [&](const std::uint64_t sequence,
                           const std::uint64_t project_time_ns) {
    return PresentationFrameRequest{
        .request_sequence = sequence,
        .project_time_ns = project_time_ns,
        .transport_epoch_id = 1,
        .device = renderer->device_identity(),
        .render_program = render_program,
    };
  };
  for (std::uint64_t frame = 0; frame < 3; ++frame) {
    require(presenter->present(request(frame, frame * 16'666'667)).succeeded());
  }
  require(presenter->resize(ViewportExtent{
      .width_points = 800,
      .height_points = 450,
      .pixels_per_point = 1.0F,
  }).succeeded());
  require(presenter->present(request(3, 50'000'001)).succeeded());

  presenter->set_visible(false);
  require(presenter->present(request(4, 66'666'668)).status ==
          FrameStatus::skipped);
  const auto telemetry = presenter->telemetry();
  require(telemetry.renderer_submissions == 4);
  require(telemetry.present_submissions == 4);
  require(telemetry.skipped_frames == 1);
  require(telemetry.zero_cpu_pixel_transfer());
  presenter->detach();
}
