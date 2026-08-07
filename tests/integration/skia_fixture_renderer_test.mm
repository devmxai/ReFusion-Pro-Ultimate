#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "TestComposition.hpp"

#import <Metal/Metal.h>

#include <cstdint>
#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("Skia fixture renderer test requirement failed");
  }
}

}  // namespace

int main() {
  using namespace refusion::runtime::presentation;

  auto device_service = refusion::platform::create_platform_gpu_device_service();
  auto native_lease = device_service->borrow();
  const auto handles = native_lease.native_handles();
  id<MTLDevice> device = (__bridge id<MTLDevice>)(
      reinterpret_cast<void*>(handles.device));
  require(device != nil);

  auto contexts = refusion::adapters::skia::SkiaGpuContexts::create(
      device_service->borrow(), test_composition());
  MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                width:640
                               height:360
                            mipmapped:NO];
  descriptor.usage = MTLTextureUsageRenderTarget;
  descriptor.storageMode = MTLStorageModePrivate;
  id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
  require(texture != nil);

  const NativeFrameTarget target{
      .backend = refusion::runtime::gpu::Backend::metal,
      .pixel_format = PixelFormat::bgra8_unorm,
      .texture = reinterpret_cast<std::uintptr_t>((__bridge void*)texture),
      .width_pixels = 640,
      .height_pixels = 360,
      .device_generation = device_service->identity().generation,
  };
  const auto rendered = contexts->render(
      target,
      FixtureFrame{
          .frame_index = 1,
          .presentation_time_ns = 500'000'000,
          .duration_ns = 30'000'000'000,
      });
  require(rendered.succeeded());

  auto wrong_generation = target;
  ++wrong_generation.device_generation;
  const auto rejected = contexts->render(wrong_generation, FixtureFrame{});
  require(rejected.status == FrameStatus::rejected);
}
