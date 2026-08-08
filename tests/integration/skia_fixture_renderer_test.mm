#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/runtime/render/RenderPlanCompiler.hpp"

#import <Metal/Metal.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(const bool condition,
             const std::string& diagnostic =
                 "Skia fixture renderer test requirement failed") {
  if (!condition) {
    throw std::runtime_error(diagnostic);
  }
}

[[nodiscard]] refusion::runtime::render::VisualRenderProgram
conformance_render_program() {
  std::ifstream input(REFUSION_XPLAT_VISUAL_FIXTURE_PATH, std::ios::binary);
  require(static_cast<bool>(input));
  const std::string source{std::istreambuf_iterator<char>(input),
                           std::istreambuf_iterator<char>()};
  const auto compiled = refusion::core::compile_project_rfx(source);
  require(compiled.succeeded());
  return refusion::runtime::render::compile_visual_render_program(
      *compiled.project);
}

struct PixelQualification final {
  std::size_t opaque_pixels{0};
  std::size_t colorful_pixels{0};
  std::size_t bright_pixels{0};
  std::size_t changed_from_background{0};
};

[[nodiscard]] PixelQualification qualify_pixels(
    const std::vector<std::uint8_t>& bgra) {
  PixelQualification result;
  for (std::size_t index = 0; index + 3 < bgra.size(); index += 4) {
    const auto blue = bgra[index];
    const auto green = bgra[index + 1];
    const auto red = bgra[index + 2];
    const auto alpha = bgra[index + 3];
    result.opaque_pixels += alpha == 255 ? 1U : 0U;
    const auto maximum = std::max({red, green, blue});
    const auto minimum = std::min({red, green, blue});
    result.colorful_pixels +=
        maximum > 60 && maximum - minimum > 35 ? 1U : 0U;
    result.bright_pixels += maximum > 150 ? 1U : 0U;
    const auto base_delta =
        std::abs(static_cast<int>(red) - 10) +
        std::abs(static_cast<int>(green) - 16) +
        std::abs(static_cast<int>(blue) - 32);
    result.changed_from_background += base_delta > 24 ? 1U : 0U;
  }
  return result;
}

}  // namespace

int main() {
  using namespace refusion::runtime::presentation;

  auto device_service = refusion::platform::create_platform_gpu_device_service();
  auto native_lease = device_service->borrow();
  id<MTLDevice> device = (__bridge id<MTLDevice>)(
      const_cast<void *>(native_lease.backend_private_device()));
  require(device != nil);
  id<MTLCommandQueue> command_queue = (__bridge id<MTLCommandQueue>)(
      const_cast<void *>(native_lease.backend_private_submission_queue()));
  require(command_queue != nil);

  auto render_program = std::make_shared<const
      refusion::runtime::render::VisualRenderProgram>(
          conformance_render_program());
  auto contexts = refusion::adapters::skia::SkiaGpuContexts::create(
      device_service->borrow());
  MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                width:640
                               height:360
                            mipmapped:NO];
  descriptor.usage = MTLTextureUsageRenderTarget;
  // CPU visibility is admitted only in this qualification executable. The
  // production renderer/presenter remains GPU-only and never maps pixels.
  descriptor.storageMode = MTLStorageModeShared;
  id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
  require(texture != nil);

  const BackendFrameTargetLease target{
      .device = device_service->identity(),
      .pixel_format = PixelFormat::bgra8_unorm,
      .target_id = 1,
      .width_pixels = 640,
      .height_pixels = 360,
      .backend_private_state = std::shared_ptr<const void>(
          CFBridgingRetain(texture),
          [](const void* value) { CFRelease(value); }),
  };
  const auto rendered = contexts->render(
      target,
      PresentationFrameRequest{
          .request_sequence = 60,
          .project_time_ns = 1'000'000'000,
          .transport_epoch_id = 77,
          .device = device_service->identity(),
          .render_program = render_program,
      });
  require(rendered.succeeded());

  // A same-queue barrier proves the asynchronous Skia submission completed
  // before the test-only read. It does not add synchronization to production.
  id<MTLCommandBuffer> barrier = [command_queue commandBuffer];
  require(barrier != nil);
  [barrier commit];
  [barrier waitUntilCompleted];
  require(barrier.status == MTLCommandBufferStatusCompleted);

  constexpr std::size_t width = 640;
  constexpr std::size_t height = 360;
  constexpr std::size_t bytes_per_pixel = 4;
  std::vector<std::uint8_t> pixels(width * height * bytes_per_pixel);
  [texture getBytes:pixels.data()
          bytesPerRow:width * bytes_per_pixel
           fromRegion:MTLRegionMake2D(0, 0, width, height)
          mipmapLevel:0];
  const auto qualification = qualify_pixels(pixels);
  const auto pixel_count = width * height;
  require(qualification.opaque_pixels == pixel_count,
          "pixel qualification: output is not fully opaque");
  require(qualification.changed_from_background > 18'000,
          "pixel qualification: foreground coverage is too small");
  require(qualification.colorful_pixels > 8'000,
          "pixel qualification: color/gradient coverage is too small");
  require(qualification.bright_pixels > 1'000,
          "pixel qualification: highlight coverage is too small: " +
              std::to_string(qualification.bright_pixels));

  // The opaque full-canvas base must survive the shared compositor unchanged
  // in a corner outside the bounded foreground operations.
  require(pixels[0] == 32 && pixels[1] == 16 && pixels[2] == 10 &&
              pixels[3] == 255,
          "pixel qualification: base corner color changed");

  auto wrong_generation = target;
  ++wrong_generation.device.generation;
  const auto rejected = contexts->render(
      wrong_generation,
      PresentationFrameRequest{
          .device = device_service->identity(),
          .render_program = render_program,
      });
  require(rejected.status == FrameStatus::rejected);

  auto stale_request = PresentationFrameRequest{
      .device = device_service->identity(),
      .render_program = render_program,
  };
  ++stale_request.device.generation;
  require(contexts->render(target, stale_request).status ==
          FrameStatus::rejected);
}
