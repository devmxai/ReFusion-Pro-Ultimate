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

#include <cstdio>
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

void pump_window_messages() {
  MSG message{};
  while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
}

void require_presented(
    const refusion::runtime::presentation::FrameResult& result,
    const std::uint64_t sequence) {
  if (result.succeeded()) {
    return;
  }
  std::fprintf(stderr, "frame %llu failed: %s: %s\n",
               static_cast<unsigned long long>(sequence), result.code.c_str(),
               result.diagnostic.c_str());
  std::fflush(stderr);
  require(false);
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
      UnregisterClassW(L"ReFusion.D3D12PresenterTest", instance_);
    }
  }

  [[nodiscard]] HWND get() const noexcept { return window_; }

 private:
  HINSTANCE instance_{nullptr};
  ATOM atom_{0};
  HWND window_{nullptr};
};

}  // namespace

int run_test() {
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
  const auto wrong_host_result = presenter->attach(wrong_host);
  require(wrong_host_result.status == FrameStatus::rejected);
  require(wrong_host_result.failure == FrameFailureKind::incompatible);
  require_presented(presenter->attach(host), 0);
  require_presented(presenter->resize(ViewportExtent{
                        .width_points = 640,
                        .height_points = 360,
                        .pixels_per_point = 1.0F,
                    }),
                    0);
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
  constexpr std::uint64_t frames_before_resize = 120;
  for (std::uint64_t frame = 0; frame < frames_before_resize; ++frame) {
    pump_window_messages();
    require_presented(presenter->present(request(frame, frame * 16'666'667)),
                      frame);
  }
  require_presented(presenter->resize(ViewportExtent{
                        .width_points = 800,
                        .height_points = 450,
                        .pixels_per_point = 1.0F,
                    }),
                    frames_before_resize);
  constexpr std::uint64_t frames_after_resize = 120;
  for (std::uint64_t frame = 0; frame < frames_after_resize; ++frame) {
    const auto sequence = frames_before_resize + frame;
    pump_window_messages();
    require_presented(
        presenter->present(request(sequence, sequence * 16'666'667)), sequence);
  }

  presenter->set_visible(false);
  const auto submitted_frames = frames_before_resize + frames_after_resize;
  const auto hidden_result = presenter->present(
      request(submitted_frames, submitted_frames * 16'666'667));
  require(hidden_result.status == FrameStatus::skipped);
  require(hidden_result.failure == FrameFailureKind::unavailable);
  const auto telemetry = presenter->telemetry();
  require(telemetry.renderer_submissions == submitted_frames);
  require(telemetry.present_submissions == submitted_frames);
  require(telemetry.skipped_frames == 1);
  require(telemetry.native_wait_timeouts == 0);
  require(telemetry.zero_cpu_pixel_transfer());
  require(telemetry.presentation_profile.valid());
  require(telemetry.presentation_profile.bytes_per_pixel() == 4 ||
          telemetry.presentation_profile.bytes_per_pixel() == 8);
  presenter->detach();
  return 0;
}

int main() {
  try {
    return run_test();
  } catch (const std::exception& error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
