#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformViewportPresenter.hpp"
#include "TestComposition.hpp"

#import <AppKit/AppKit.h>

#include <charconv>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("Metal viewport presenter test requirement failed");
  }
}

[[nodiscard]] std::uint64_t requested_frame_count() {
  const char* value = std::getenv("REFUSION_PRESENTER_SOAK_FRAMES");
  if (value == nullptr) {
    return 3;
  }
  std::uint64_t result = 0;
  const std::string_view input(value);
  const auto [end, error] = std::from_chars(
      input.data(), input.data() + input.size(), result);
  require(error == std::errc{} && end == input.data() + input.size() && result != 0);
  return result;
}

}  // namespace

int main() {
  using namespace refusion::runtime::presentation;

  [NSApplication sharedApplication];
  NSView* host = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 640.0, 360.0)];
  host.wantsLayer = YES;

  auto device_service = refusion::platform::create_platform_gpu_device_service();
  auto render_program = std::make_shared<const
      refusion::runtime::render::VisualRenderProgram>(test_render_program());
  auto renderer = refusion::adapters::skia::SkiaGpuContexts::create(
      device_service->borrow());
  auto presenter = refusion::platform::create_platform_viewport_presenter(
      *device_service, *renderer);

  auto viewport_host = refusion::platform::acquire_platform_viewport_host(
      reinterpret_cast<std::uintptr_t>((__bridge void*)host));
  auto wrong_window_system = viewport_host;
  wrong_window_system.window_system = NativeWindowSystem::win32_hwnd;
  const auto invalid_host = presenter->attach(wrong_window_system);
  require(invalid_host.status == FrameStatus::rejected);

  require(presenter->attach(viewport_host).succeeded());
  require(presenter->resize(ViewportExtent{
      .width_points = 640,
      .height_points = 360,
      .pixels_per_point = 1.0F,
  }).succeeded());
  presenter->set_visible(true);

  const auto frame_request = [&](const std::uint64_t sequence,
                                 const std::uint64_t project_time_ns) {
    return PresentationFrameRequest{
        .request_sequence = sequence,
        .project_time_ns = project_time_ns,
        .device = renderer->device_identity(),
        .render_program = render_program,
    };
  };

  const std::uint64_t frame_count = requested_frame_count();
  const bool soak_mode = std::getenv("REFUSION_PRESENTER_SOAK_FRAMES") != nullptr;
  constexpr std::uint64_t project_duration_ns = 30'000'000'000ULL;
  constexpr std::uint64_t frame_duration_ns = 16'666'667ULL;
  for (std::uint64_t frame_index = 0; frame_index < frame_count; ++frame_index) {
    if (frame_index == frame_count / 2) {
      host.frame = NSMakeRect(0.0, 0.0, 800.0, 450.0);
      require(presenter->resize(ViewportExtent{
          .width_points = 800,
          .height_points = 450,
          .pixels_per_point = 1.0F,
      }).succeeded());
    }
    // The qualification composition is a 30-second looping project. Keep the
    // stress sample inside its legal ProjectTime domain instead of asking the
    // renderer to accept an out-of-range timestamp after the first loop.
    const auto project_time_ns =
        (frame_index * frame_duration_ns) % project_duration_ns;
    const auto result = presenter->present(
        frame_request(frame_index, project_time_ns));
    require(result.succeeded());
  }

  presenter->set_visible(false);
  require(presenter->present(frame_request(0, 0)).status == FrameStatus::skipped);
  presenter->set_visible(true);

  if (!soak_mode) {
    using refusion::runtime::gpu::DeviceLifecycleEvent;
    using refusion::runtime::gpu::DeviceStatus;
    const auto suspended = device_service->handle_lifecycle_event(
        DeviceLifecycleEvent::will_sleep);
    require(suspended.status == DeviceStatus::suspended);
    require(presenter->present(frame_request(0, 0)).status == FrameStatus::skipped);
    const auto resumed = device_service->handle_lifecycle_event(
        DeviceLifecycleEvent::did_wake);
    require(resumed.ready());
    require(presenter->present(frame_request(0, 0)).succeeded());

    const auto original_generation = renderer->device_identity().generation;
    const auto lost =
        device_service->report_device_loss("injected presenter-test loss");
    require(lost.identity.generation == original_generation + 1);
    require(presenter->present(frame_request(0, 0)).status == FrameStatus::rejected);
  }

  const auto telemetry = presenter->telemetry();
  const auto lifecycle_presentations = soak_mode ? 0ULL : 1ULL;
  const auto lifecycle_requests = soak_mode ? 0ULL : 3ULL;
  require(telemetry.frame_requests == frame_count + 1 + lifecycle_requests);
  require(telemetry.drawable_acquisitions == frame_count + lifecycle_presentations);
  require(telemetry.renderer_submissions == frame_count + lifecycle_presentations);
  require(telemetry.present_submissions == frame_count + lifecycle_presentations);
  require(telemetry.skipped_frames == 1 + (soak_mode ? 0 : 1));
  require(telemetry.rejected_frames == 1 + (soak_mode ? 0 : 1));
  require(telemetry.visibility_suspends == 1);
  require(telemetry.visibility_resumes == 2);
  if (!soak_mode) {
    require(telemetry.device_status == refusion::runtime::gpu::DeviceStatus::lost);
    require(telemetry.device_suspended_frames == 1);
    require(telemetry.device_loss_rejections == 1);
    require(telemetry.stale_generation_rejections == 1);
  }
  require(telemetry.zero_cpu_pixel_transfer());

  std::cout << "{\"requested_frames\":" << frame_count
            << ",\"present_submissions\":" << telemetry.present_submissions
            << ",\"cpu_pixel_maps\":" << telemetry.cpu_pixel_maps
            << ",\"cpu_pixel_uploads\":" << telemetry.cpu_pixel_uploads
            << ",\"gpu_readbacks\":" << telemetry.gpu_readbacks
            << ",\"unattributed_gpu_copies\":"
            << telemetry.unattributed_gpu_copies << "}\n";

  presenter->detach();
}
