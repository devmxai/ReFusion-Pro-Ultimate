#include "XplatFontFixture.hpp"
#include "refusion/adapters/skia/SkiaGpuComposition.hpp"
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

void write_reference_ppm(const std::string& path,
                         const std::vector<std::uint8_t>& bgra,
                         const std::size_t width,
                         const std::size_t height) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  require(static_cast<bool>(output),
          "could not open the requested qualification capture");
  output << "P6\n" << width << ' ' << height << "\n255\n";
  for (std::size_t index = 0; index + 3 < bgra.size(); index += 4) {
    const char rgb[] = {
        static_cast<char>(bgra[index + 2]),
        static_cast<char>(bgra[index + 1]),
        static_cast<char>(bgra[index]),
    };
    output.write(rgb, sizeof(rgb));
  }
  require(static_cast<bool>(output),
          "could not write the requested qualification capture");
}

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

void qualify_actual_pixel_crop(
    const std::vector<std::uint8_t>& actual,
    const std::vector<std::uint8_t>& reference,
    const std::size_t reference_width, const std::size_t crop_left,
    const std::size_t crop_top, const std::size_t crop_width,
    const std::size_t crop_height) {
  require(actual.size() == crop_width * crop_height * 4U,
          "Metal actual-pixel Canvas has an unexpected extent");
  std::uint8_t maximum_delta = 0;
  for (std::size_t y = 0; y < crop_height; ++y) {
    for (std::size_t x = 0; x < crop_width; ++x) {
      const auto actual_index = (y * crop_width + x) * 4U;
      const auto reference_index =
          ((y + crop_top) * reference_width + x + crop_left) * 4U;
      for (std::size_t channel = 0; channel < 4; ++channel) {
        maximum_delta = std::max(
            maximum_delta,
            static_cast<std::uint8_t>(std::abs(
                static_cast<int>(actual[actual_index + channel]) -
                static_cast<int>(reference[reference_index + channel]))));
      }
    }
  }
  require(maximum_delta <= 2,
          "Metal 100% Canvas is not a pixel-true centered crop; max delta=" +
              std::to_string(maximum_delta));
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
  auto contexts = refusion::adapters::skia::create_skia_gpu_composition(
      device_service->borrow(), nullptr, nullptr,
      refusion::tests::make_xplat_font_fixture_resolver());
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
      .presentation_profile = kFallbackSdrPresentationProfile,
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
          .output_consumer = refusion::runtime::render::VisualOutputConsumer::interactive_preview,
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

  MTLTextureDescriptor* fit_descriptor = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                width:320
                               height:180
                            mipmapped:NO];
  fit_descriptor.usage = MTLTextureUsageRenderTarget;
  fit_descriptor.storageMode = MTLStorageModeShared;
  id<MTLTexture> fit_texture = [device newTextureWithDescriptor:fit_descriptor];
  require(fit_texture != nil);
  const BackendFrameTargetLease fit_target{
      .device = device_service->identity(),
      .presentation_profile = kFallbackSdrPresentationProfile,
      .target_id = 3,
      .width_pixels = 320,
      .height_pixels = 180,
      .backend_private_state = std::shared_ptr<const void>(
          CFBridgingRetain(fit_texture),
          [](const void* value) { CFRelease(value); }),
  };
  const auto fit_rendered = contexts->render(
      fit_target,
      PresentationFrameRequest{
          .request_sequence = 61,
          .project_time_ns = 1'000'000'000,
          .transport_epoch_id = 77,
          .device = device_service->identity(),
          .output_consumer = refusion::runtime::render::VisualOutputConsumer::interactive_preview,
          .render_program = render_program,
      });
  require(fit_rendered.succeeded(), fit_rendered.diagnostic);

  id<MTLCommandBuffer> fit_barrier = [command_queue commandBuffer];
  require(fit_barrier != nil);
  [fit_barrier commit];
  [fit_barrier waitUntilCompleted];
  require(fit_barrier.status == MTLCommandBufferStatusCompleted);
  std::vector<std::uint8_t> fit_pixels(320U * 180U * bytes_per_pixel);
  [fit_texture getBytes:fit_pixels.data()
             bytesPerRow:320U * bytes_per_pixel
              fromRegion:MTLRegionMake2D(0, 0, 320, 180)
             mipmapLevel:0];
  const auto fit_qualification = qualify_pixels(fit_pixels);
  require(fit_qualification.opaque_pixels == 320U * 180U,
          "downsampled Canvas is not fully opaque");
  require(fit_qualification.changed_from_background > 3'000,
          "full-resolution Canvas downsample lost foreground detail");

  id<MTLTexture> actual_pixel_texture =
      [device newTextureWithDescriptor:fit_descriptor];
  require(actual_pixel_texture != nil);
  const BackendFrameTargetLease actual_pixel_target{
      .device = device_service->identity(),
      .presentation_profile = kFallbackSdrPresentationProfile,
      .target_id = 5,
      .width_pixels = 320,
      .height_pixels = 180,
      .backend_private_state = std::shared_ptr<const void>(
          CFBridgingRetain(actual_pixel_texture),
          [](const void* value) { CFRelease(value); }),
  };
  const auto actual_pixel_rendered = contexts->render(
      actual_pixel_target,
      PresentationFrameRequest{
          .request_sequence = 60,
          .project_time_ns = 1'000'000'000,
          .transport_epoch_id = 77,
          .device = device_service->identity(),
          .output_consumer = refusion::runtime::render::
              VisualOutputConsumer::interactive_preview,
          .canvas_view = {
              .mode = refusion::runtime::render::
                  CanvasViewportMode::custom_zoom,
              .raster_quality = refusion::runtime::render::
                  CanvasRasterQuality::full_resolution,
              .zoom = 1.0,
          },
          .render_program = render_program,
      });
  require(actual_pixel_rendered.succeeded(),
          actual_pixel_rendered.diagnostic);
  id<MTLCommandBuffer> actual_pixel_barrier = [command_queue commandBuffer];
  require(actual_pixel_barrier != nil);
  [actual_pixel_barrier commit];
  [actual_pixel_barrier waitUntilCompleted];
  require(actual_pixel_barrier.status == MTLCommandBufferStatusCompleted);
  std::vector<std::uint8_t> actual_pixel_pixels(
      320U * 180U * bytes_per_pixel);
  [actual_pixel_texture getBytes:actual_pixel_pixels.data()
                     bytesPerRow:320U * bytes_per_pixel
                      fromRegion:MTLRegionMake2D(0, 0, 320, 180)
                     mipmapLevel:0];
  qualify_actual_pixel_crop(actual_pixel_pixels, pixels, width, 160, 90, 320,
                            180);

  MTLTextureDescriptor* high_precision_descriptor = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                width:320
                               height:180
                            mipmapped:NO];
  high_precision_descriptor.usage = MTLTextureUsageRenderTarget;
  high_precision_descriptor.storageMode = MTLStorageModePrivate;
  id<MTLTexture> high_precision_texture =
      [device newTextureWithDescriptor:high_precision_descriptor];
  require(high_precision_texture != nil,
          "Metal device rejected the RGBA16F presentation fixture");
  const BackendFrameTargetLease high_precision_target{
      .device = device_service->identity(),
      .presentation_profile = kHighPrecisionSdrPresentationProfile,
      .target_id = 4,
      .width_pixels = 320,
      .height_pixels = 180,
      .backend_private_state = std::shared_ptr<const void>(
          CFBridgingRetain(high_precision_texture),
          [](const void* value) { CFRelease(value); }),
  };
  const auto high_precision_rendered = contexts->render(
      high_precision_target,
      PresentationFrameRequest{
          .request_sequence = 62,
          .project_time_ns = 1'000'000'000,
          .transport_epoch_id = 77,
          .device = device_service->identity(),
          .output_consumer = refusion::runtime::render::
              VisualOutputConsumer::interactive_preview,
          .render_program = render_program,
      });
  require(high_precision_rendered.succeeded(),
          high_precision_rendered.diagnostic);
  id<MTLCommandBuffer> high_precision_barrier =
      [command_queue commandBuffer];
  require(high_precision_barrier != nil);
  [high_precision_barrier commit];
  [high_precision_barrier waitUntilCompleted];
  require(high_precision_barrier.status == MTLCommandBufferStatusCompleted);

  // Offline qualification is a distinct output consumer but must execute the
  // same immutable program, exact ProjectTime, lowering and Skia compositor.
  // A second offscreen GPU target proves that the consumer identity does not
  // introduce a hidden export renderer or alter project pixels.
  id<MTLTexture> offline_texture =
      [device newTextureWithDescriptor:descriptor];
  require(offline_texture != nil);
  const BackendFrameTargetLease offline_target{
      .device = device_service->identity(),
      .presentation_profile = kFallbackSdrPresentationProfile,
      .target_id = 2,
      .width_pixels = 640,
      .height_pixels = 360,
      .backend_private_state = std::shared_ptr<const void>(
          CFBridgingRetain(offline_texture),
          [](const void* value) { CFRelease(value); }),
  };
  const auto offline_rendered = contexts->render(
      offline_target,
      PresentationFrameRequest{
          .request_sequence = 60,
          .project_time_ns = 1'000'000'000,
          .transport_epoch_id = 0,
          .device = device_service->identity(),
          .output_consumer = refusion::runtime::render::VisualOutputConsumer::offline_export,
          .render_program = render_program,
      });
  require(offline_rendered.succeeded());

  id<MTLCommandBuffer> offline_barrier = [command_queue commandBuffer];
  require(offline_barrier != nil);
  [offline_barrier commit];
  [offline_barrier waitUntilCompleted];
  require(offline_barrier.status == MTLCommandBufferStatusCompleted);

  std::vector<std::uint8_t> offline_pixels(
      width * height * bytes_per_pixel);
  [offline_texture getBytes:offline_pixels.data()
                 bytesPerRow:width * bytes_per_pixel
                  fromRegion:MTLRegionMake2D(0, 0, width, height)
                 mipmapLevel:0];
  require(offline_pixels == pixels,
          "Preview and Offline qualification produced different pixels");
  if (const char* capture_path =
          std::getenv("REFUSION_XPLAT_CAPTURE_PPM");
      capture_path != nullptr && capture_path[0] != '\0') {
    write_reference_ppm(capture_path, pixels, width, height);
  }

  auto unknown_consumer_request = PresentationFrameRequest{
      .device = device_service->identity(),
      .output_consumer = static_cast<
          refusion::runtime::render::VisualOutputConsumer>(255),
      .render_program = render_program,
  };
  require(!unknown_consumer_request.valid(),
          "unknown output consumers must fail request admission");
  require(contexts->render(target, unknown_consumer_request).status ==
              FrameStatus::rejected,
          "the native renderer admitted an unknown output consumer");

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
