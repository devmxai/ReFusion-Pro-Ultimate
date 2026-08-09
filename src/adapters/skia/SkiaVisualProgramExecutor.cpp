#include "SkiaVisualProgramExecutor.hpp"

#include "SkiaSceneCompositor.hpp"
#include "SkiaSurfacePolicy.hpp"
#include "SkiaTextLayoutInternal.hpp"

#include "refusion/runtime/render/VisualOutputContract.hpp"
#include "refusion/runtime/render/ViewportMapping.hpp"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImage.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkSurface.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace refusion::adapters::skia {

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
    const runtime::render::ProjectTimeNs project_time_ns,
    const std::uint64_t transport_epoch_id,
    const runtime::render::VisualOutputConsumer output_consumer,
    const runtime::render::CanvasViewportState& canvas_view,
    const std::uint32_t target_width_pixels,
    const std::uint32_t target_height_pixels) {
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

  if (mapping.requires_full_resolution_intermediate) {
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
    draw_visual_render_plan(composition_canvas, text_layout_engine, frame.plan);
    auto composition_image =
        implementation_->composition_surface->makeTemporaryImage();
    if (!composition_image) {
      throw std::runtime_error(
          "RFX-CANVAS-SNAPSHOT-001: Skia could not snapshot the GPU "
          "Composition surface");
    }

    SkPaint presentation_paint;
    presentation_paint.setAntiAlias(true);
    presentation_paint.setDither(true);
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
    target_canvas.drawImageRect(
        downsampled_image,
        SkRect::MakeWH(static_cast<float>(downsampled_width),
                       static_cast<float>(downsampled_height)),
        destination, SkSamplingOptions(SkCubicResampler::Mitchell()),
        &presentation_paint, SkCanvas::kStrict_SrcRectConstraint);
    return;
  }

  target_canvas.save();
  target_canvas.clipRect(destination, SkClipOp::kIntersect, true);
  target_canvas.translate(static_cast<float>(mapping.destination_left),
                          static_cast<float>(mapping.destination_top));
  target_canvas.scale(static_cast<float>(mapping.canvas_to_target_scale),
                      static_cast<float>(mapping.canvas_to_target_scale));
  draw_visual_render_plan(target_canvas, text_layout_engine, frame.plan);
  target_canvas.restore();
}

}  // namespace refusion::adapters::skia
