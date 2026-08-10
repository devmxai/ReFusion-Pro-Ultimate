#include "SkiaVisualProgramExecutor.hpp"

#include "SkiaSceneCompositor.hpp"
#include "SkiaSurfacePolicy.hpp"
#include "SkiaTextLayoutInternal.hpp"

#include "refusion/runtime/render/VisualOutputContract.hpp"
#include "refusion/runtime/render/ViewportMapping.hpp"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImage.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkString.h"
#include "include/core/SkSurface.h"
#include "include/effects/SkRuntimeEffect.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace refusion::adapters::skia {
namespace {

[[nodiscard]] const sk_sp<SkRuntimeEffect>& presentation_dither_effect() {
  static const sk_sp<SkRuntimeEffect> effect = [] {
    const auto result = SkRuntimeEffect::MakeForShader(SkString(R"(
      uniform shader source;
      uniform float seed;
      uniform float ditherRange;
      uniform float linearTarget;

      half4 main(float2 position) {
        half4 color = source.eval(position);
        if (color.a <= 0.0) {
          return color;
        }
        half3 straight = color.rgb / color.a;
        half3 encoded = linearTarget > 0.5
                            ? fromLinearSrgb(straight)
                            : straight;
        float2 pixel = floor(max(position, float2(0.0)));
        float3 hash = fract(float3(pixel, seed) *
                            float3(0.1031, 0.1030, 0.0973));
        hash += dot(hash, hash.yzx + 33.33);
        float noise = fract((hash.x + hash.y) * hash.z) - 0.5;
        encoded = clamp(encoded + half(noise * ditherRange), 0.0, 1.0);
        half3 resultColor = linearTarget > 0.5
                                ? toLinearSrgb(encoded)
                                : encoded;
        return half4(resultColor * color.a, color.a);
      }
    )"));
    if (!result.effect) {
      throw std::runtime_error(
          "RFX-CANVAS-DITHER-001: Skia rejected the common presentation "
          "shader: " +
          std::string(result.errorText.c_str()));
    }
    return result.effect;
  }();
  return effect;
}

void draw_presented_image(
    SkCanvas& canvas, const sk_sp<SkImage>& image, const SkRect& source,
    const SkRect& destination, const SkSamplingOptions& sampling,
    const std::uint64_t presentation_sequence, const bool linear_target) {
  const auto image_matrix = SkMatrix::RectToRect(source, destination);
  auto image_shader = image->makeShader(
      SkTileMode::kClamp, SkTileMode::kClamp, sampling, image_matrix);
  if (!image_shader) {
    throw std::runtime_error(
        "RFX-CANVAS-DITHER-002: Skia could not create the presentation "
        "image shader");
  }
  SkRuntimeShaderBuilder builder(presentation_dither_effect());
  builder.child("source") = std::move(image_shader);
  builder.uniform("seed") = static_cast<float>(presentation_sequence % 4096U);
  builder.uniform("ditherRange") = 0.999F / 255.0F;
  builder.uniform("linearTarget") = linear_target ? 1.0F : 0.0F;
  auto shader = builder.makeShader();
  if (!shader) {
    throw std::runtime_error(
        "RFX-CANVAS-DITHER-003: Skia could not instantiate the common "
        "presentation shader");
  }
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setShader(std::move(shader));
  canvas.drawRect(destination, paint);
}

}  // namespace

struct SkiaVisualProgramExecutor::Implementation final {
  struct DownsampleSurface final {
    sk_sp<SkSurface> surface;
    std::uint32_t width_pixels{0};
    std::uint32_t height_pixels{0};
  };

  sk_sp<SkSurface> composition_surface;
  std::uint32_t width_pixels{0};
  std::uint32_t height_pixels{0};
  std::vector<DownsampleSurface> downsample_surfaces;
};

SkiaVisualProgramExecutor::SkiaVisualProgramExecutor()
    : implementation_(std::make_unique<Implementation>()) {}

SkiaVisualProgramExecutor::~SkiaVisualProgramExecutor() = default;

SkiaVisualProgramExecutor::SkiaVisualProgramExecutor(
    SkiaVisualProgramExecutor&&) noexcept = default;

SkiaVisualProgramExecutor& SkiaVisualProgramExecutor::operator=(
    SkiaVisualProgramExecutor&&) noexcept = default;

void SkiaVisualProgramExecutor::execute(
    SkSurface& target_surface,
    SkiaTextLayoutEngine& text_layout_engine,
    const runtime::render::VisualRenderProgram& program,
    const std::uint64_t presentation_sequence,
    const runtime::render::ProjectTimeNs project_time_ns,
    const std::uint64_t transport_epoch_id,
    const runtime::render::VisualOutputConsumer output_consumer,
    const runtime::render::CanvasViewportState& canvas_view,
    const std::uint32_t target_width_pixels,
    const std::uint32_t target_height_pixels,
    SkiaVideoFrameResolver* video_frames) {
  if (target_surface.width() <= 0 || target_surface.height() <= 0 ||
      static_cast<std::uint32_t>(target_surface.width()) !=
          target_width_pixels ||
      static_cast<std::uint32_t>(target_surface.height()) !=
          target_height_pixels) {
    throw std::invalid_argument(
        "RFX-CANVAS-TARGET-001: target surface extent does not match the "
        "viewport contract");
  }
  const auto frame = runtime::render::prepare_visual_output_frame(
      output_consumer, program, project_time_ns, transport_epoch_id,
      text_layout_engine);
  const auto mapping = runtime::render::resolve_viewport_mapping({
      .canvas_width_pixels = frame.plan.canvas_width_pixels,
      .canvas_height_pixels = frame.plan.canvas_height_pixels,
      .target_width_pixels = target_width_pixels,
      .target_height_pixels = target_height_pixels,
      .viewport = canvas_view,
  });

  auto& target_canvas = *target_surface.getCanvas();
  target_canvas.clear(SK_ColorBLACK);
  const auto destination =
      SkRect::MakeXYWH(static_cast<float>(mapping.destination_left),
                       static_cast<float>(mapping.destination_top),
                       static_cast<float>(mapping.destination_width),
                       static_cast<float>(mapping.destination_height));

  {
    if (!implementation_->composition_surface ||
        implementation_->width_pixels != frame.plan.canvas_width_pixels ||
        implementation_->height_pixels != frame.plan.canvas_height_pixels) {
      implementation_->composition_surface =
          target_surface.makeSurface(composition_surface_info(
              frame.plan.canvas_width_pixels, frame.plan.canvas_height_pixels));
      implementation_->width_pixels = frame.plan.canvas_width_pixels;
      implementation_->height_pixels = frame.plan.canvas_height_pixels;
    }
    if (!implementation_->composition_surface) {
      throw std::runtime_error(
          "RFX-CANVAS-SURFACE-001: GPU cannot allocate the required "
          "full-resolution Composition surface");
    }

    auto& composition_canvas =
        *implementation_->composition_surface->getCanvas();
    composition_canvas.resetMatrix();
    draw_visual_render_plan(composition_canvas, text_layout_engine, frame.plan,
                            video_frames);
    auto composition_image =
        implementation_->composition_surface->makeTemporaryImage();
    if (!composition_image) {
      throw std::runtime_error(
          "RFX-CANVAS-SNAPSHOT-001: Skia could not snapshot the GPU "
          "Composition surface");
    }

    sk_sp<SkImage> downsampled_image = std::move(composition_image);
    std::uint32_t downsampled_width = frame.plan.canvas_width_pixels;
    std::uint32_t downsampled_height = frame.plan.canvas_height_pixels;
    const auto final_width = static_cast<std::uint32_t>(
        std::max(1.0, std::ceil(mapping.destination_width)));
    const auto final_height = static_cast<std::uint32_t>(
        std::max(1.0, std::ceil(mapping.destination_height)));
    std::size_t stage_index = 0;
    while (static_cast<std::uint64_t>(downsampled_width) >
               static_cast<std::uint64_t>(final_width) * 2U ||
           static_cast<std::uint64_t>(downsampled_height) >
               static_cast<std::uint64_t>(final_height) * 2U) {
      const auto next_width =
          std::max(final_width, (downsampled_width + 1U) / 2U);
      const auto next_height =
          std::max(final_height, (downsampled_height + 1U) / 2U);
      if (implementation_->downsample_surfaces.size() <= stage_index) {
        implementation_->downsample_surfaces.emplace_back();
      }
      auto& stage = implementation_->downsample_surfaces[stage_index];
      if (!stage.surface || stage.width_pixels != next_width ||
          stage.height_pixels != next_height) {
        stage.surface = target_surface.makeSurface(
            composition_surface_info(next_width, next_height));
        stage.width_pixels = next_width;
        stage.height_pixels = next_height;
      }
      if (!stage.surface) {
        throw std::runtime_error(
            "RFX-CANVAS-DOWNSAMPLE-001: GPU could not allocate a staged "
            "Canvas downsample surface");
      }

      auto& stage_canvas = *stage.surface->getCanvas();
      stage_canvas.resetMatrix();
      stage_canvas.clear(SK_ColorTRANSPARENT);
      SkPaint stage_paint;
      stage_paint.setBlendMode(SkBlendMode::kSrc);
      stage_canvas.drawImageRect(
          downsampled_image,
          SkRect::MakeWH(static_cast<float>(downsampled_width),
                         static_cast<float>(downsampled_height)),
          SkRect::MakeWH(static_cast<float>(next_width),
                         static_cast<float>(next_height)),
          SkSamplingOptions(SkFilterMode::kLinear), &stage_paint,
          SkCanvas::kStrict_SrcRectConstraint);
      downsampled_image = stage.surface->makeTemporaryImage();
      if (!downsampled_image) {
        throw std::runtime_error(
            "RFX-CANVAS-DOWNSAMPLE-002: Skia could not snapshot a staged "
            "Canvas downsample surface");
      }
      downsampled_width = next_width;
      downsampled_height = next_height;
      ++stage_index;
    }
    implementation_->downsample_surfaces.resize(stage_index);
    const auto presentation_sampling =
        mapping.canvas_to_target_scale == 1.0
            ? SkSamplingOptions(SkFilterMode::kNearest)
            : SkSamplingOptions(SkCubicResampler::Mitchell());
    draw_presented_image(
        target_canvas, downsampled_image,
        SkRect::MakeWH(static_cast<float>(downsampled_width),
                       static_cast<float>(downsampled_height)),
        destination, presentation_sampling, presentation_sequence,
        target_surface.imageInfo().colorType() == kRGBA_F16_SkColorType);
  }
}

}  // namespace refusion::adapters::skia
