#include "SkiaSceneCompositor.hpp"

#include "SkiaTextLayoutInternal.hpp"

#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/effects/SkGradient.h"
#include "include/effects/SkImageFilters.h"

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace refusion::adapters::skia {
namespace {

using runtime::render::BlendMode;
using runtime::render::ColorRgba8;
using runtime::render::DrawLayer;
using runtime::render::GradientStop;

[[nodiscard]] SkColor to_sk_color(const ColorRgba8& color,
                                  const double opacity) {
  const auto alpha = static_cast<std::uint8_t>(
      std::clamp(static_cast<double>(color.alpha) * opacity, 0.0, 255.0));
  return SkColorSetARGB(alpha, color.red, color.green, color.blue);
}

[[nodiscard]] sk_sp<SkImageFilter> make_effect_stack(
    const std::vector<runtime::render::Effect>& effects) {
  sk_sp<SkImageFilter> input;
  for (const auto& effect : effects) {
    std::visit(
        [&input](const auto& parameters) {
          using Parameters = std::decay_t<decltype(parameters)>;
          if constexpr (std::is_same_v<Parameters,
                                       runtime::render::GaussianBlur>) {
            input = SkImageFilters::Blur(
                static_cast<SkScalar>(parameters.sigma_x),
                static_cast<SkScalar>(parameters.sigma_y), std::move(input));
          } else if constexpr (std::is_same_v<
                                   Parameters, runtime::render::DropShadow>) {
            input = SkImageFilters::DropShadow(
                static_cast<SkScalar>(parameters.offset_x),
                static_cast<SkScalar>(parameters.offset_y),
                static_cast<SkScalar>(parameters.sigma_x),
                static_cast<SkScalar>(parameters.sigma_y),
                to_sk_color(parameters.color, 1.0), std::move(input));
          } else {
            input = SkImageFilters::DropShadow(
                0.0F, 0.0F, static_cast<SkScalar>(parameters.sigma),
                static_cast<SkScalar>(parameters.sigma),
                to_sk_color(parameters.color, 1.0), std::move(input));
          }
        },
        effect.parameters);
  }
  return input;
}

[[nodiscard]] SkBlendMode to_sk_blend_mode(const BlendMode mode) {
  switch (mode) {
    case BlendMode::normal:
      return SkBlendMode::kSrcOver;
    case BlendMode::multiply:
      return SkBlendMode::kMultiply;
    case BlendMode::screen:
      return SkBlendMode::kScreen;
    case BlendMode::overlay:
      return SkBlendMode::kOverlay;
  }
  return SkBlendMode::kSrcOver;
}

template <typename Factory>
[[nodiscard]] sk_sp<SkShader> make_gradient_shader(
    const std::vector<GradientStop>& stops, Factory&& factory) {
  std::vector<SkColor4f> colors;
  std::vector<float> positions;
  colors.reserve(stops.size());
  positions.reserve(stops.size());
  for (const auto& stop : stops) {
    colors.push_back(SkColor4f::FromColor(to_sk_color(stop.color, 1.0)));
    positions.push_back(static_cast<float>(stop.offset));
  }
  const SkGradient gradient{
      SkGradient::Colors{SkSpan<const SkColor4f>(colors),
                         SkSpan<const float>(positions), SkTileMode::kClamp,
                         SkColorSpace::MakeSRGB()},
      SkGradient::Interpolation{
          .fInPremul = SkGradient::Interpolation::InPremul::kNo,
          .fColorSpace = SkGradient::Interpolation::ColorSpace::kSRGB,
          .fHueMethod = SkGradient::Interpolation::HueMethod::kShorter}};
  return factory(gradient);
}

void apply_masks(SkCanvas& canvas, const DrawLayer& layer) {
  for (const auto& mask : layer.masks) {
    const auto rect = SkRect::MakeXYWH(
        static_cast<float>(mask.position_x - mask.width * 0.5),
        static_cast<float>(mask.position_y - mask.height * 0.5),
        static_cast<float>(mask.width), static_cast<float>(mask.height));
    canvas.clipRRect(
        SkRRect::MakeRectXY(rect, static_cast<float>(mask.corner_radius),
                           static_cast<float>(mask.corner_radius)),
        mask.inverted ? SkClipOp::kDifference : SkClipOp::kIntersect, true);
  }
}

void draw_shape(SkCanvas& canvas, const runtime::render::DrawShape& shape,
                const double opacity) {
  SkPaint paint;
  paint.setAntiAlias(true);
  if (const auto* solid = std::get_if<ColorRgba8>(&shape.fill)) {
    paint.setColor(to_sk_color(*solid, opacity));
  } else if (const auto* linear =
                 std::get_if<runtime::render::LinearGradient>(&shape.fill)) {
    const SkPoint points[2] = {
        SkPoint::Make(static_cast<float>(linear->start_x),
                      static_cast<float>(linear->start_y)),
        SkPoint::Make(static_cast<float>(linear->end_x),
                      static_cast<float>(linear->end_y)),
    };
    paint.setShader(make_gradient_shader(
        linear->stops, [&points](const SkGradient& gradient) {
          return SkShaders::LinearGradient(points, gradient);
        }));
    paint.setAlphaf(static_cast<float>(opacity));
  } else {
    const auto& radial = std::get<runtime::render::RadialGradient>(shape.fill);
    paint.setShader(make_gradient_shader(
        radial.stops, [&radial](const SkGradient& gradient) {
          return SkShaders::RadialGradient(
              SkPoint::Make(static_cast<float>(radial.center_x),
                            static_cast<float>(radial.center_y)),
              static_cast<float>(radial.radius), gradient);
        }));
    paint.setAlphaf(static_cast<float>(opacity));
  }

  const auto rect = SkRect::MakeXYWH(
      static_cast<float>(-shape.width * 0.5),
      static_cast<float>(-shape.height * 0.5), static_cast<float>(shape.width),
      static_cast<float>(shape.height));
  const auto rounded =
      SkRRect::MakeRectXY(rect, static_cast<float>(shape.corner_radius),
                          static_cast<float>(shape.corner_radius));
  canvas.drawRRect(rounded, paint);
  if (shape.stroke_width > 0.0) {
    SkPaint stroke;
    stroke.setAntiAlias(true);
    stroke.setStyle(SkPaint::kStroke_Style);
    stroke.setStrokeWidth(static_cast<float>(shape.stroke_width));
    stroke.setColor(to_sk_color(shape.stroke_color, opacity));
    canvas.drawRRect(rounded, stroke);
  }
}

void draw_text(SkCanvas& canvas, SkiaTextLayoutEngine& text_layout_engine,
               const runtime::render::DrawText& text, const double opacity) {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(to_sk_color(text.fill, opacity));
  canvas.save();
  if (text.clip) {
    canvas.clipRect(
        SkRect::MakeLTRB(static_cast<float>(text.clip->left),
                         static_cast<float>(text.clip->top),
                         static_cast<float>(text.clip->right),
                         static_cast<float>(text.clip->bottom)),
        SkClipOp::kIntersect, true);
  }
  if (!text_layout_engine.draw_cached(canvas, text.cache_key, paint)) {
    throw std::runtime_error(
        "RFX-TEXT-LAYOUT-PREVIEW-002: accepted layout cache entry is missing");
  }
  canvas.restore();
}

}  // namespace

void draw_visual_render_plan(
    SkCanvas& canvas, SkiaTextLayoutEngine& text_layout_engine,
    const runtime::render::VisualRenderPlan& plan) {
  if (!plan.valid()) {
    throw std::invalid_argument(
        "RFX-RENDER-PLAN-003: Skia received an invalid VisualRenderPlan");
  }
  // The common compositor implements exactly this portable profile. A future
  // profile requires its own admitted common policy; a native backend may not
  // silently reinterpret this plan using API defaults.
  if (!core::is_desktop_v1_sdr_color_contract(plan.color_contract) ||
      plan.color_contract_digest !=
          core::desktop_v1_sdr_color_contract_digest()) {
    throw std::invalid_argument(
        "RFX-COLOR-CONTRACT-001: unsupported visual color contract");
  }
  canvas.drawColor(to_sk_color(plan.clear_color, 1.0), SkBlendMode::kSrc);
  canvas.save();

  for (const auto& layer : plan.layers) {
    canvas.save();
    const auto& transform = layer.world_transform;
    canvas.concat(SkMatrix::MakeAll(
        static_cast<SkScalar>(transform.m00),
        static_cast<SkScalar>(transform.m01),
        static_cast<SkScalar>(transform.m02),
        static_cast<SkScalar>(transform.m10),
        static_cast<SkScalar>(transform.m11),
        static_cast<SkScalar>(transform.m12), 0.0F, 0.0F, 1.0F));

    const auto effect_filter = make_effect_stack(layer.effects);
    const bool isolated = effect_filter || layer.blend_mode != BlendMode::normal;
    if (isolated) {
      SkPaint effect_paint;
      effect_paint.setImageFilter(effect_filter);
      effect_paint.setBlendMode(to_sk_blend_mode(layer.blend_mode));
      const auto& bounds = layer.isolation_bounds;
      const auto sk_bounds = SkRect::MakeLTRB(
          static_cast<float>(bounds.left), static_cast<float>(bounds.top),
          static_cast<float>(bounds.right), static_cast<float>(bounds.bottom));
      canvas.saveLayer(bounds.valid() ? &sk_bounds : nullptr, &effect_paint);
    }

    apply_masks(canvas, layer);
    std::visit(
        [&canvas, &text_layout_engine, &layer](const auto& content) {
          using Content = std::decay_t<decltype(content)>;
          if constexpr (std::is_same_v<Content,
                                       runtime::render::DrawShape>) {
            draw_shape(canvas, content, layer.effective_opacity);
          } else {
            draw_text(canvas, text_layout_engine, content,
                      layer.effective_opacity);
          }
        },
        layer.content);
    if (isolated) {
      canvas.restore();
    }
    canvas.restore();
  }
  canvas.restore();
}

}  // namespace refusion::adapters::skia
