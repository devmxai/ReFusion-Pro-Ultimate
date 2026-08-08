#include "LongGopMediaFixture.hpp"
#include "TestComposition.hpp"
#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformMediaCapability.hpp"
#include "refusion/platform/PlatformViewportPresenter.hpp"
#include "refusion/runtime/gpu/GpuObservability.hpp"
#include "refusion/runtime/media/HardwareVideoDecodeScheduler.hpp"

#import <AppKit/AppKit.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

void require(const bool condition, const std::source_location location =
                                       std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error(
        "GPU observability integration requirement failed at line " +
        std::to_string(location.line()));
  }
}

[[nodiscard]] const char* thermal_name(
    const refusion::runtime::gpu::GpuThermalState state) noexcept {
  using refusion::runtime::gpu::GpuThermalState;
  switch (state) {
    case GpuThermalState::nominal:
      return "nominal";
    case GpuThermalState::fair:
      return "fair";
    case GpuThermalState::serious:
      return "serious";
    case GpuThermalState::critical:
      return "critical";
    case GpuThermalState::unknown:
      return "unknown";
  }
  return "unknown";
}

}  // namespace

int main() {
  using namespace refusion::runtime::gpu;
  using namespace refusion::runtime::media;
  using namespace refusion::runtime::presentation;

  [NSApplication sharedApplication];
  NSView* host =
      [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 640.0, 360.0)];
  host.wantsLayer = YES;

  auto device_service = refusion::platform::create_platform_gpu_device_service();
  require(device_service != nullptr);
  const auto initial_device = device_service->identity();
  const std::string tier_id =
      "MACOS-METAL-INTERACTIVE-640X360/" + initial_device.adapter_name;
  auto observations = std::make_shared<GpuObservabilityService>(
      initial_device, 4'096,
      GpuQualificationBudget{
          .device_tier_id = tier_id,
          .maximum_peak_resident_bytes = 64ULL * 1'024ULL * 1'024ULL,
          .maximum_fence_latency_ns = 2'000'000'000ULL,
          .maximum_thermal_state = GpuThermalState::serious,
      });

  {
    auto decoder = refusion::platform::create_platform_hardware_video_decoder(
        *device_service, observations);
    require(decoder != nullptr);
    HardwareVideoDecodeScheduler scheduler(*decoder);
    HardwareSeekDecodeRequest request{
        .source_path = REFUSION_TEST_H264_FIXTURE_PATH,
        .expected_profile =
            {
                .coded_width = 320,
                .coded_height = 180,
            },
        .samples = refusion_test::long_gop_samples(),
        .target_presentation_time = {.value = 99'500, .timescale = 30'000},
        .stamp =
            {
                .transport_epoch_id = 1,
                .device = initial_device,
            },
        .residency = {.maximum_surface_count = 3},
    };
    auto decoded = scheduler.decode_seek(request);
    require(decoded.admitted());
    require(decoded.target_source_frame_index == 9);
    require(decoded.telemetry.dependency_samples_submitted == 11);
    require(decoded.telemetry.surfaces_retained == 2);
    auto publication =
        scheduler.publish_if_current(std::move(decoded), request.stamp);
    require(publication.accepted);
    require(publication.counters.strict_path_clean());

    auto render_program = std::make_shared<const
        refusion::runtime::render::VisualRenderProgram>(test_render_program());
    auto renderer = refusion::adapters::skia::SkiaGpuContexts::create(
        device_service->borrow(), publication.queue, observations);
    require(renderer != nullptr);
    require(renderer->ganesh_ready());
    auto presenter = refusion::platform::create_platform_viewport_presenter(
        *device_service, *renderer, observations);
    require(presenter != nullptr);
    require(presenter
                ->attach(refusion::platform::acquire_platform_viewport_host(
                    reinterpret_cast<std::uintptr_t>((__bridge void*)host)))
                .succeeded());
    require(presenter
                ->resize(ViewportExtent{
                    .width_points = 640,
                    .height_points = 360,
                    .pixels_per_point = 1.0F,
                })
                .succeeded());
    presenter->set_visible(true);

    require(presenter
                ->present(PresentationFrameRequest{
                    .request_sequence = 99,
                    .project_time_ns = 3'300'000'000ULL,
                    .device = initial_device,
                    .render_program = render_program,
                })
                .succeeded());
    require(renderer->selected_video_source_frame_index() == 9);
    require(presenter
                ->present(PresentationFrameRequest{
                    .request_sequence = 100,
                    .project_time_ns = 3'333'333'334ULL,
                    .device = initial_device,
                    .render_program = render_program,
                })
                .succeeded());
    require(renderer->selected_video_source_frame_index() == 10);
    require(presenter->telemetry().present_submissions == 2);
    require(presenter->telemetry().zero_cpu_pixel_transfer());

    const auto lost = device_service->report_device_loss(
        "injected G1-WP05 joint observability proof");
    require(lost.identity.generation == initial_device.generation + 1);
    require(presenter
                ->present(PresentationFrameRequest{
                    .device = initial_device,
                    .render_program = render_program,
                })
                .status == FrameStatus::rejected);

    presenter->detach();
    presenter.reset();
    renderer.reset();
    publication.selected_surface.reset();
    publication.queue.reset();
  }

  require(observations->wait_until_quiescent(std::chrono::seconds(5)));
  const auto snapshot = observations->snapshot();
  require(snapshot.strict_path_clean());
  require(snapshot.quiescent());
  require(snapshot.within_qualification_budget());
  require(snapshot.device_loss_events == 1);
  require(snapshot.stale_generation_rejections >= 1);
  require(snapshot.stale_generation_resources_accepted == 0);
  require(snapshot.attributed_submissions == 15);
  require(snapshot.attributed_copies == 0);
  require(snapshot.attributed_conversions == 0);
  require(snapshot.unattributed_submissions == 0);
  require(snapshot.unattributed_copies == 0);
  require(snapshot.unattributed_conversions == 0);
  require(snapshot.fences_issued == 3);
  require(snapshot.fences_completed == 3);
  require(snapshot.fence_latency_samples == 3);
  require(snapshot.thermal_samples == 3);
  require(snapshot.peak_resident_bytes != 0);
  require(snapshot.trace.size() == snapshot.event_sequence);

  std::cout << "{\"device_tier\":\"" << tier_id
            << "\",\"adapter_id\":" << initial_device.adapter_id
            << ",\"resource_leases\":"
            << snapshot.resource_leases_acquired
            << ",\"attributed_submissions\":"
            << snapshot.attributed_submissions
            << ",\"attributed_copies\":" << snapshot.attributed_copies
            << ",\"attributed_conversions\":"
            << snapshot.attributed_conversions
            << ",\"peak_resident_bytes\":"
            << snapshot.peak_resident_bytes
            << ",\"fence_latency_max_ns\":"
            << snapshot.fence_latency_max_ns << ",\"thermal_state\":\""
            << thermal_name(snapshot.maximum_observed_thermal_state)
            << "\",\"device_loss_events\":" << snapshot.device_loss_events
            << ",\"stale_generation_resources_accepted\":"
            << snapshot.stale_generation_resources_accepted
            << ",\"strict_path_clean\":true}\n";
}
