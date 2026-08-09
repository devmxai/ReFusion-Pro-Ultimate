#include "TestComposition.hpp"
#include "LongGopMediaFixture.hpp"
#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformMediaCapability.hpp"
#include "refusion/runtime/media/HardwareVideoDecodeScheduler.hpp"

#import <Metal/Metal.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("Skia decoded video surface requirement failed");
  }
}

class RendererPresenter final : public refusion::runtime::presentation::ViewportPresenter {
 public:
  RendererPresenter(refusion::runtime::presentation::ViewportFrameRenderer &renderer,
                    refusion::runtime::presentation::BackendFrameTargetLease target)
      : renderer_(renderer), target_(target) {}

  [[nodiscard]] refusion::runtime::gpu::DeviceIdentity device_identity()
      const noexcept override {
    return renderer_.device_identity();
  }

  [[nodiscard]] refusion::runtime::presentation::FrameResult attach(
      refusion::runtime::presentation::NativeViewportHostLease) override {
    return {
        .status = refusion::runtime::presentation::FrameStatus::accepted,
    };
  }
  void detach() noexcept override {}
  [[nodiscard]] refusion::runtime::presentation::FrameResult resize(
      refusion::runtime::presentation::ViewportExtent) override {
    return {
        .status = refusion::runtime::presentation::FrameStatus::accepted,
    };
  }
  void set_visible(bool) noexcept override {}
  [[nodiscard]] refusion::runtime::presentation::FrameResult present(
      const refusion::runtime::presentation::PresentationFrameRequest &frame)
      override {
    ++telemetry_.frame_requests;
    auto result = renderer_.render(target_, frame);
    if (result.succeeded()) {
      ++telemetry_.renderer_submissions;
      ++telemetry_.present_submissions;
    } else {
      ++telemetry_.rejected_frames;
    }
    return result;
  }
  [[nodiscard]] refusion::runtime::presentation::PresentationTelemetry telemetry()
      const noexcept override {
    return telemetry_;
  }

 private:
  refusion::runtime::presentation::ViewportFrameRenderer &renderer_;
  refusion::runtime::presentation::BackendFrameTargetLease target_;
  refusion::runtime::presentation::PresentationTelemetry telemetry_;
};

}  // namespace

int main() {
  using namespace refusion::runtime::media;
  using namespace refusion::runtime::presentation;

  auto device_service = refusion::platform::create_platform_gpu_device_service();
  require(device_service != nullptr);
  auto decoder = refusion::platform::create_platform_hardware_video_decoder(*device_service);
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
              .device = device_service->identity(),
          },
      .residency = {.maximum_surface_count = 3},
  };
  auto decoded = scheduler.decode_seek(request);
  require(decoded.admitted());
  require(decoded.target_source_frame_index == 9);
  require(decoded.telemetry.dependency_samples_submitted == 11);
  require(decoded.telemetry.peak_surface_residency == 11);
  require(decoded.telemetry.surfaces_retained == 2);
  auto publication = scheduler.publish_if_current(
      std::move(decoded), request.stamp);
  require(publication.accepted);

  auto render_program = std::make_shared<const
      refusion::runtime::render::VisualRenderProgram>(test_render_program());
  auto renderer = refusion::adapters::skia::SkiaGpuContexts::create(
      device_service->borrow(), publication.queue);
  require(renderer != nullptr);
  require(renderer->ganesh_ready());
  require(renderer->device_identity().adapter_id ==
          publication.queue->device_identity().adapter_id);

  auto native_lease = device_service->borrow();
  id<MTLDevice> device = (__bridge id<MTLDevice>)(const_cast<void *>(
      native_lease.backend_private_device()));
  require(device != nil);
  MTLTextureDescriptor *descriptor =
      [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                         width:640
                                                        height:360
                                                     mipmapped:NO];
  descriptor.usage = MTLTextureUsageRenderTarget;
  descriptor.storageMode = MTLStorageModePrivate;
  id<MTLTexture> target_texture = [device newTextureWithDescriptor:descriptor];
  require(target_texture != nil);

  RendererPresenter presenter(
      *renderer, BackendFrameTargetLease{
                     .device = device_service->identity(),
                     .presentation_profile =
                         kFallbackSdrPresentationProfile,
                     .target_id = 1,
                     .width_pixels = 640,
                     .height_pixels = 360,
                     .backend_private_state = std::shared_ptr<const void>(
                         CFBridgingRetain(target_texture),
                         [](const void* value) { CFRelease(value); }),
                 });
  auto fake_now = std::chrono::steady_clock::time_point{};
  ViewportRenderSession transport(presenter,
                                  PlaybackSpec{
                                      .duration_ns = 30'000'000'000,
                                      .frame_rate_numerator = 30,
                                      .frame_rate_denominator = 1,
                                      .loop = true,
                                  },
                                  [&fake_now] { return fake_now; },
                                  render_program);
  require(transport
              .attach({
                  .window_system = NativeWindowSystem::cocoa_view,
                  .host_id = 1,
                  .backend_private_state = std::make_shared<const int>(1),
              })
              .succeeded());
  require(transport.seek_to_frame(99).succeeded());
  require(renderer->selected_video_source_frame_index() == 9);
  require(transport.playback_state().position_ns == 3'300'000'000);
  require(transport.playback_state().clock_epoch_id == 1);
  require(transport.seek_to_frame(100).succeeded());
  require(renderer->selected_video_source_frame_index() == 10);
  require(transport.playback_state().clock_epoch_id == 2);
  require(transport.seek_to_frame(99).succeeded());
  require(renderer->selected_video_source_frame_index() == 9);
  require(transport.playback_state().position_ns == 3'300'000'000);
  require(transport.playback_state().clock_epoch_id == 3);

  const auto counters = decoder->counters();
  require(counters.hardware_decoder_sessions == 1);
  require(counters.compressed_samples_submitted == 11);
  require(counters.hardware_frames_decoded == 11);
  require(counters.native_surface_plane_bindings == 22);
  require(counters.native_surface_leases_released == 9);
  require(counters.surface_queues_published == 1);
  require(counters.strict_path_clean());
  require(presenter.telemetry().present_submissions == 3);
  require(presenter.telemetry().zero_cpu_pixel_transfer());
  std::cout << "{\"transport_clock_owner\":\"core::ProjectClock\","
            << "\"selected_source_frames\":[9,10,9],"
            << "\"dependency_samples\":11,"
            << "\"bounded_surface_residency\":2,"
            << "\"skia_yuva_composite\":true,"
            << "\"plane_bindings\":" << counters.native_surface_plane_bindings << ','
            << "\"cpu_pixel_maps\":" << counters.cpu_pixel_maps << ','
            << "\"cpu_pixel_conversions\":" << counters.cpu_pixel_conversions << ','
            << "\"cpu_pixel_uploads\":" << counters.cpu_pixel_uploads << ','
            << "\"gpu_readbacks\":" << counters.gpu_readbacks << "}\n";
}
