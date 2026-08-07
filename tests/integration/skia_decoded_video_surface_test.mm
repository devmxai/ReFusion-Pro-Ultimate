#include "TestComposition.hpp"
#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformMediaCapability.hpp"

#import <Metal/Metal.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("Skia decoded video surface requirement failed");
  }
}

class RendererPresenter final : public refusion::runtime::presentation::ViewportPresenter {
 public:
  RendererPresenter(refusion::runtime::presentation::ViewportFrameRenderer &renderer,
                    refusion::runtime::presentation::NativeFrameTarget target)
      : renderer_(renderer), target_(target) {}

  [[nodiscard]] refusion::runtime::presentation::FrameResult attach(
      refusion::runtime::presentation::NativeViewportHost) override {
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
      const refusion::runtime::presentation::FixtureFrame &frame) override {
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
  refusion::runtime::presentation::NativeFrameTarget target_;
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

  HardwareDecodeSequenceRequest request{
      .source_path = REFUSION_TEST_H264_FIXTURE_PATH,
      .expected_profile =
          {
              .coded_width = 320,
              .coded_height = 180,
          },
  };
  for (std::uint64_t index = 0; index < 8; ++index) {
    request.samples.push_back({
        .access_unit_index = index,
        .source_frame_index = index,
        .timing =
            {
                .presentation_time =
                    {
                        .value = static_cast<std::int64_t>(index),
                        .timescale = 30,
                    },
                .duration = {.value = 1, .timescale = 30},
            },
        .decode_time =
            {
                .value = static_cast<std::int64_t>(index),
                .timescale = 30,
            },
        .sync_sample = true,
    });
  }
  auto decoded = decoder->decode_sequence(request);
  require(decoded.admitted());

  auto renderer = refusion::adapters::skia::SkiaGpuContexts::create(
      device_service->borrow(), test_composition(), decoded.queue);
  require(renderer != nullptr);
  require(renderer->ganesh_ready());
  require(renderer->device_identity().adapter_id == decoded.queue->device_identity().adapter_id);

  auto native_lease = device_service->borrow();
  const auto handles = native_lease.native_handles();
  id<MTLDevice> device = (__bridge id<MTLDevice>)(reinterpret_cast<void *>(handles.device));
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
      *renderer, NativeFrameTarget{
                     .backend = refusion::runtime::gpu::Backend::metal,
                     .pixel_format = PixelFormat::bgra8_unorm,
                     .texture = reinterpret_cast<std::uintptr_t>((__bridge void *)target_texture),
                     .width_pixels = 640,
                     .height_pixels = 360,
                     .device_generation = device_service->identity().generation,
                 });
  auto fake_now = std::chrono::steady_clock::time_point{};
  ViewportRenderSession transport(presenter,
                                  PlaybackSpec{
                                      .duration_ns = 30'000'000'000,
                                      .frame_rate_numerator = 30,
                                      .frame_rate_denominator = 1,
                                      .loop = true,
                                  },
                                  [&fake_now] { return fake_now; });
  require(transport
              .attach({
                  .window_system = NativeWindowSystem::cocoa_view,
                  .handle = 1,
              })
              .succeeded());
  require(transport.seek_to_frame(3).succeeded());
  require(renderer->selected_video_source_frame_index() == 3);
  require(transport.playback_state().position_ns == 100'000'000);
  require(transport.playback_state().clock_epoch_id == 1);
  require(transport.seek_to_frame(7).succeeded());
  require(renderer->selected_video_source_frame_index() == 7);
  require(transport.playback_state().clock_epoch_id == 2);
  require(transport.seek_to_frame(1).succeeded());
  require(renderer->selected_video_source_frame_index() == 1);
  require(transport.playback_state().position_ns == 33'333'334);
  require(transport.playback_state().clock_epoch_id == 3);

  const auto counters = decoder->counters();
  require(counters.hardware_decoder_sessions == 1);
  require(counters.compressed_samples_submitted == 8);
  require(counters.hardware_frames_decoded == 8);
  require(counters.native_surface_plane_bindings == 16);
  require(counters.surface_queues_published == 1);
  require(counters.strict_path_clean());
  require(presenter.telemetry().present_submissions == 3);
  require(presenter.telemetry().zero_cpu_pixel_transfer());
  std::cout << "{\"transport_clock_owner\":\"core::ProjectClock\","
            << "\"selected_source_frames\":[3,7,1],"
            << "\"skia_yuva_composite\":true,"
            << "\"plane_bindings\":" << counters.native_surface_plane_bindings << ','
            << "\"cpu_pixel_maps\":" << counters.cpu_pixel_maps << ','
            << "\"cpu_pixel_conversions\":" << counters.cpu_pixel_conversions << ','
            << "\"cpu_pixel_uploads\":" << counters.cpu_pixel_uploads << ','
            << "\"gpu_readbacks\":" << counters.gpu_readbacks << "}\n";
}
