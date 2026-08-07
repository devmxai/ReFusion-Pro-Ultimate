#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformViewportPresenter.hpp"
#include "TestComposition.hpp"

#import <AppKit/AppKit.h>

#include <charconv>
#include <cstdlib>
#include <cstdint>
#include <iostream>
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
  auto renderer = refusion::adapters::skia::SkiaGpuContexts::create(
      device_service->borrow(), test_composition());
  auto presenter = refusion::platform::create_platform_viewport_presenter(
      *device_service, *renderer);

  const auto invalid_host = presenter->attach(NativeViewportHost{
      .window_system = NativeWindowSystem::win32_hwnd,
      .handle = reinterpret_cast<std::uintptr_t>((__bridge void*)host),
  });
  require(invalid_host.status == FrameStatus::rejected);

  require(presenter->attach(NativeViewportHost{
      .window_system = NativeWindowSystem::cocoa_view,
      .handle = reinterpret_cast<std::uintptr_t>((__bridge void*)host),
  }).succeeded());
  require(presenter->resize(ViewportExtent{
      .width_points = 640,
      .height_points = 360,
      .pixels_per_point = 1.0F,
  }).succeeded());
  presenter->set_visible(true);

  const std::uint64_t frame_count = requested_frame_count();
  for (std::uint64_t frame_index = 0; frame_index < frame_count; ++frame_index) {
    if (frame_index == frame_count / 2) {
      host.frame = NSMakeRect(0.0, 0.0, 800.0, 450.0);
      require(presenter->resize(ViewportExtent{
          .width_points = 800,
          .height_points = 450,
          .pixels_per_point = 1.0F,
      }).succeeded());
    }
    const auto result = presenter->present(FixtureFrame{
        .frame_index = frame_index,
        .presentation_time_ns = frame_index * 16'666'667,
        .duration_ns = 30'000'000'000,
    });
    require(result.succeeded());
  }

  presenter->set_visible(false);
  require(presenter->present(FixtureFrame{}).status == FrameStatus::skipped);
  presenter->set_visible(true);

  const auto telemetry = presenter->telemetry();
  require(telemetry.frame_requests == frame_count + 1);
  require(telemetry.drawable_acquisitions == frame_count);
  require(telemetry.renderer_submissions == frame_count);
  require(telemetry.present_submissions == frame_count);
  require(telemetry.skipped_frames == 1);
  require(telemetry.rejected_frames == 1);
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
