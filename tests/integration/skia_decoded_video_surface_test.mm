#include "TestComposition.hpp"
#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformMediaCapability.hpp"

#import <Metal/Metal.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("Skia decoded video surface requirement failed");
  }
}

}  // namespace

int main() {
  using namespace refusion::runtime::media;
  using namespace refusion::runtime::presentation;

  auto device_service = refusion::platform::create_platform_gpu_device_service();
  require(device_service != nullptr);
  auto decoder = refusion::platform::create_platform_hardware_video_decoder(*device_service);
  require(decoder != nullptr);

  auto decoded = decoder->decode(HardwareDecodeRequest{
      .source_path = REFUSION_TEST_H264_FIXTURE_PATH,
      .expected_profile =
          {
              .coded_width = 320,
              .coded_height = 180,
          },
      .source_frame_index = 5,
      .packet_timing =
          {
              .presentation_time = {.value = 5, .timescale = 30},
              .duration = {.value = 1, .timescale = 30},
          },
  });
  require(decoded.admitted());

  auto renderer = refusion::adapters::skia::SkiaGpuContexts::create(
      device_service->borrow(), test_composition(), decoded.surface);
  require(renderer != nullptr);
  require(renderer->ganesh_ready());
  require(renderer->device_identity().adapter_id == decoded.surface->info().device.adapter_id);

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

  const auto result = renderer->render(
      NativeFrameTarget{
          .backend = refusion::runtime::gpu::Backend::metal,
          .pixel_format = PixelFormat::bgra8_unorm,
          .texture = reinterpret_cast<std::uintptr_t>((__bridge void *)target_texture),
          .width_pixels = 640,
          .height_pixels = 360,
          .device_generation = device_service->identity().generation,
      },
      FixtureFrame{
          .frame_index = 5,
          .presentation_time_ns = 166'666'666,
          .duration_ns = 30'000'000'000,
      });
  require(result.succeeded());

  const auto counters = decoder->counters();
  require(counters.hardware_frames_decoded == 1);
  require(counters.native_surface_plane_bindings == 2);
  require(counters.strict_path_clean());
  std::cout << "{\"source_frame_index\":5,"
            << "\"skia_yuva_composite\":true,"
            << "\"plane_bindings\":" << counters.native_surface_plane_bindings << ','
            << "\"cpu_pixel_maps\":" << counters.cpu_pixel_maps << ','
            << "\"cpu_pixel_conversions\":" << counters.cpu_pixel_conversions << ','
            << "\"cpu_pixel_uploads\":" << counters.cpu_pixel_uploads << ','
            << "\"gpu_readbacks\":" << counters.gpu_readbacks << "}\n";
}
