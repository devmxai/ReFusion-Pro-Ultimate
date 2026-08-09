#include "refusion/runtime/render/RenderPlanCompiler.hpp"

#include "refusion/core/CanonicalText.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"

#include <bit>
#include <cmath>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace refusion::runtime::render {

struct VisualRenderProgram::State final {
  core::ProjectSnapshot project;
  core::VisualColorContract color_contract;
  std::string color_contract_digest;
};

struct VisualRenderProgramAccess final {
  [[nodiscard]] static const VisualRenderProgram::State& state(
      const VisualRenderProgram& program) {
    if (!program.state_) {
      throw std::invalid_argument("RFX-RENDER-PROGRAM-001: render program is empty");
    }
    return *program.state_;
  }

  [[nodiscard]] static VisualRenderProgram make(
      const core::ProjectSnapshot& project) {
    return VisualRenderProgram(
        std::make_shared<const VisualRenderProgram::State>(
            VisualRenderProgram::State{
                .project = project,
                .color_contract = core::desktop_v1_sdr_color_contract(),
                .color_contract_digest =
                    core::desktop_v1_sdr_color_contract_digest()}));
  }
};

namespace {

[[nodiscard]] ColorRgba8 lower_color(const core::ColorRgba8& value) noexcept {
  return {.red = value.red,
          .green = value.green,
          .blue = value.blue,
          .alpha = value.alpha};
}

[[nodiscard]] LocalRect lower_rect(const core::LocalRect& value) noexcept {
  return {.left = value.left,
          .top = value.top,
          .right = value.right,
          .bottom = value.bottom};
}

[[nodiscard]] AffineTransform2D lower_transform(
    const core::AffineTransform2D& value) noexcept {
  return {.m00 = value.m00,
          .m01 = value.m01,
          .m02 = value.m02,
          .m10 = value.m10,
          .m11 = value.m11,
          .m12 = value.m12};
}

[[nodiscard]] GradientStop lower_stop(const core::GradientStop& value) {
  return {.offset = value.offset, .color = lower_color(value.color)};
}

[[nodiscard]] std::vector<GradientStop> lower_stops(
    const std::vector<core::GradientStop>& values) {
  std::vector<GradientStop> result;
  result.reserve(values.size());
  for (const auto& value : values) {
    result.push_back(lower_stop(value));
  }
  return result;
}

[[nodiscard]] ShapeFill lower_fill(const core::ShapeFill& value) {
  return std::visit(
      [](const auto& fill) -> ShapeFill {
        using Fill = std::decay_t<decltype(fill)>;
        if constexpr (std::is_same_v<Fill, core::ColorRgba8>) {
          return lower_color(fill);
        } else if constexpr (std::is_same_v<Fill,
                                            core::LinearGradientFill>) {
          return LinearGradient{.start_x = fill.start_x,
                                .start_y = fill.start_y,
                                .end_x = fill.end_x,
                                .end_y = fill.end_y,
                                .stops = lower_stops(fill.stops)};
        } else {
          return RadialGradient{.center_x = fill.center_x,
                                .center_y = fill.center_y,
                                .radius = fill.radius,
                                .stops = lower_stops(fill.stops)};
        }
      },
      value);
}

[[nodiscard]] BlendMode lower_blend(const core::BlendMode value) noexcept {
  switch (value) {
    case core::BlendMode::normal:
      return BlendMode::normal;
    case core::BlendMode::multiply:
      return BlendMode::multiply;
    case core::BlendMode::screen:
      return BlendMode::screen;
    case core::BlendMode::overlay:
      return BlendMode::overlay;
  }
  return BlendMode::normal;
}

[[nodiscard]] Effect lower_effect(const core::LayerEffect& value) {
  const auto kind = core::visual_effect_kind(value);
  const auto* descriptor =
      core::find_visual_contribution_descriptor(kind);
  if (descriptor == nullptr) {
    throw std::invalid_argument(
        "RFX-RENDER-CONTRIBUTION-001: effect descriptor is not registered");
  }
  auto parameters = std::visit(
      [](const auto& effect) -> EffectParameters {
        using Parameters = std::decay_t<decltype(effect)>;
        if constexpr (std::is_same_v<Parameters,
                                     core::GaussianBlurEffect>) {
          return GaussianBlur{.sigma_x = effect.sigma_x,
                              .sigma_y = effect.sigma_y};
        } else if constexpr (std::is_same_v<Parameters,
                                            core::DropShadowEffect>) {
          return DropShadow{.offset_x = effect.offset_x,
                            .offset_y = effect.offset_y,
                            .sigma_x = effect.sigma_x,
                            .sigma_y = effect.sigma_y,
                            .color = lower_color(effect.color)};
        } else {
          return Glow{.sigma = effect.sigma,
                      .color = lower_color(effect.color)};
        }
      },
      value.parameters);
  return Effect{
      .descriptor_id = descriptor->id,
      .capability_id = descriptor->capability_id,
      .schema_version = descriptor->schema_version,
      .parameters = std::move(parameters),
  };
}

[[nodiscard]] DrawContent lower_content(
    const core::EvaluatedVisualLayer& layer) {
  return std::visit(
      [&layer](const auto& content) -> DrawContent {
        using Content = std::decay_t<decltype(content)>;
        if constexpr (std::is_same_v<Content, core::ShapeLayerContent>) {
          return DrawShape{.width = content.width,
                           .height = content.height,
                           .corner_radius = content.corner_radius,
                           .fill = lower_fill(content.fill),
                           .stroke_width = content.stroke_width,
                           .stroke_color = lower_color(content.stroke_color)};
        } else {
          if (!layer.text_layout) {
            throw std::runtime_error(
                "RFX-TEXT-LAYOUT-PLAN-001: evaluated Text Layer has no layout result");
          }
          std::optional<LocalRect> clip;
          if (content.overflow == core::TextOverflowMode::clip) {
            clip = lower_rect(layer.text_layout->content_box);
          }
          return DrawText{.cache_key = layer.text_layout->cache_key,
                          .fill = lower_color(content.fill),
                          .clip = clip};
        }
      },
      layer.content);
}

void hash_bytes(std::uint64_t& hash, const void* bytes,
                const std::size_t size) noexcept {
  const auto* current = static_cast<const std::uint8_t*>(bytes);
  for (std::size_t index = 0; index < size; ++index) {
    hash ^= current[index];
    hash *= 1099511628211ULL;
  }
}

template <typename Value>
  requires(std::is_integral_v<Value> || std::is_enum_v<Value>)
void hash_value(std::uint64_t& hash, const Value value) noexcept {
  const auto append_little_endian = [&hash](auto bits) {
    using Unsigned = decltype(bits);
    for (std::size_t index = 0; index < sizeof(Unsigned); ++index) {
      const auto byte = static_cast<std::uint8_t>(bits & 0xffU);
      hash_bytes(hash, &byte, sizeof(byte));
      if constexpr (sizeof(Unsigned) > 1U) {
        if (index + 1U < sizeof(Unsigned)) {
          bits >>= 8U;
        }
      }
    }
  };
  if constexpr (std::is_enum_v<Value>) {
    using Raw = std::underlying_type_t<Value>;
    append_little_endian(
        static_cast<std::make_unsigned_t<Raw>>(value));
  } else if constexpr (std::is_same_v<Value, bool>) {
    append_little_endian(static_cast<std::uint8_t>(value ? 1U : 0U));
  } else {
    append_little_endian(
        static_cast<std::make_unsigned_t<Value>>(value));
  }
}

void hash_value(std::uint64_t& hash, const double value) noexcept {
  constexpr double kSemanticScale = 1'000'000.0;
  const double scaled = value * kSemanticScale;
  const double canonical = std::isfinite(scaled)
                               ? std::round(scaled) / kSemanticScale
                               : value;
  hash_value(hash, std::bit_cast<std::uint64_t>(
                       canonical == 0.0 ? 0.0 : canonical));
}

void hash_color(std::uint64_t& hash, const ColorRgba8& color) noexcept {
  hash_value(hash, color.red);
  hash_value(hash, color.green);
  hash_value(hash, color.blue);
  hash_value(hash, color.alpha);
}

void hash_rect(std::uint64_t& hash, const LocalRect& rect) noexcept {
  hash_value(hash, rect.left);
  hash_value(hash, rect.top);
  hash_value(hash, rect.right);
  hash_value(hash, rect.bottom);
}

void hash_transform(std::uint64_t& hash,
                    const AffineTransform2D& transform) noexcept {
  hash_value(hash, transform.m00);
  hash_value(hash, transform.m01);
  hash_value(hash, transform.m02);
  hash_value(hash, transform.m10);
  hash_value(hash, transform.m11);
  hash_value(hash, transform.m12);
}

void hash_string(std::uint64_t& hash, const std::string_view value) noexcept {
  hash_bytes(hash, value.data(), value.size());
  constexpr std::uint8_t separator = 0xff;
  hash_value(hash, separator);
}

[[nodiscard]] std::string plan_digest(const VisualRenderPlan& plan) {
  std::uint64_t hash = 14695981039346656037ULL;
  hash_string(hash, plan.stamp.project_id);
  hash_value(hash, plan.stamp.revision);
  hash_string(hash, plan.stamp.composition_id);
  hash_value(hash, plan.stamp.project_time_ns);
  hash_value(hash, plan.canvas_width_pixels);
  hash_value(hash, plan.canvas_height_pixels);
  hash_string(hash, plan.color_contract.profile_id);
  hash_value(hash, plan.color_contract.schema_version);
  hash_value(hash, plan.color_contract.authored_encoding);
  hash_value(hash, plan.color_contract.primaries);
  hash_value(hash, plan.color_contract.transfer);
  hash_value(hash, plan.color_contract.project_alpha);
  hash_value(hash, plan.color_contract.compositing_alpha);
  hash_value(hash, plan.color_contract.blend_filter_working_space);
  hash_value(hash, plan.color_contract.gradient_interpolation);
  hash_value(hash, plan.color_contract.filter_edge);
  hash_value(hash, plan.color_contract.target_format);
  hash_value(hash, plan.color_contract.output_transfer);
  hash_string(hash, plan.color_contract_digest);
  hash_color(hash, plan.clear_color);
  for (const auto& layer : plan.layers) {
    hash_string(hash, layer.layer_id);
    hash_transform(hash, layer.world_transform);
    hash_value(hash, layer.effective_opacity);
    hash_value(hash, layer.blend_mode);
    hash_rect(hash, layer.isolation_bounds);
    for (const auto& mask : layer.masks) {
      hash_string(hash, mask.descriptor_id);
      hash_string(hash, mask.capability_id);
      hash_value(hash, mask.schema_version);
      hash_value(hash, mask.inverted);
      hash_value(hash, mask.position_x);
      hash_value(hash, mask.position_y);
      hash_value(hash, mask.width);
      hash_value(hash, mask.height);
      hash_value(hash, mask.corner_radius);
    }
    for (const auto& effect : layer.effects) {
      hash_string(hash, effect.descriptor_id);
      hash_string(hash, effect.capability_id);
      hash_value(hash, effect.schema_version);
      hash_value(hash, effect.parameters.index());
      std::visit(
          [&hash](const auto& value) {
            using EffectType = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<EffectType, GaussianBlur>) {
              hash_value(hash, value.sigma_x);
              hash_value(hash, value.sigma_y);
            } else if constexpr (std::is_same_v<EffectType, DropShadow>) {
              hash_value(hash, value.offset_x);
              hash_value(hash, value.offset_y);
              hash_value(hash, value.sigma_x);
              hash_value(hash, value.sigma_y);
              hash_color(hash, value.color);
            } else {
              hash_value(hash, value.sigma);
              hash_color(hash, value.color);
            }
          },
          effect.parameters);
    }
    hash_value(hash, layer.content.index());
    std::visit(
        [&hash](const auto& content) {
          using Content = std::decay_t<decltype(content)>;
          if constexpr (std::is_same_v<Content, DrawShape>) {
            hash_value(hash, content.width);
            hash_value(hash, content.height);
            hash_value(hash, content.corner_radius);
            hash_value(hash, content.stroke_width);
            hash_color(hash, content.stroke_color);
            hash_value(hash, content.fill.index());
            std::visit(
                [&hash](const auto& fill) {
                  using Fill = std::decay_t<decltype(fill)>;
                  if constexpr (std::is_same_v<Fill, ColorRgba8>) {
                    hash_color(hash, fill);
                  } else {
                    if constexpr (std::is_same_v<Fill, LinearGradient>) {
                      hash_value(hash, fill.start_x);
                      hash_value(hash, fill.start_y);
                      hash_value(hash, fill.end_x);
                      hash_value(hash, fill.end_y);
                    } else {
                      hash_value(hash, fill.center_x);
                      hash_value(hash, fill.center_y);
                      hash_value(hash, fill.radius);
                    }
                    for (const auto& stop : fill.stops) {
                      hash_value(hash, stop.offset);
                      hash_color(hash, stop.color);
                    }
                  }
                },
                content.fill);
          } else {
            hash_string(hash, content.cache_key);
            hash_color(hash, content.fill);
            const bool clipped = content.clip.has_value();
            hash_value(hash, clipped);
            if (content.clip) {
              hash_rect(hash, *content.clip);
            }
          }
        },
        layer.content);
  }
  return "rfx-render-plan-v3-fnv1a64:" + core::canonical_hex64(hash);
}

}  // namespace

VisualRenderProgram::VisualRenderProgram(std::shared_ptr<const State> state)
    : state_(std::move(state)) {}

bool VisualRenderProgram::valid() const noexcept {
  return state_ && state_->project.composition.has_value();
}

const std::string& VisualRenderProgram::project_id() const noexcept {
  static const std::string empty;
  return state_ ? state_->project.project_id.value : empty;
}

std::uint64_t VisualRenderProgram::revision() const noexcept {
  return state_ ? state_->project.revision_id.value : 0;
}

const std::string& VisualRenderProgram::composition_id() const noexcept {
  static const std::string empty;
  return state_ && state_->project.composition
             ? state_->project.composition->composition_id.value
             : empty;
}

std::uint32_t VisualRenderProgram::canvas_width_pixels() const noexcept {
  return state_ && state_->project.composition
             ? state_->project.composition->canvas.width_pixels
             : 0;
}

std::uint32_t VisualRenderProgram::canvas_height_pixels() const noexcept {
  return state_ && state_->project.composition
             ? state_->project.composition->canvas.height_pixels
             : 0;
}

const core::VisualColorContract& VisualRenderProgram::color_contract() const
    noexcept {
  static const core::VisualColorContract empty;
  return state_ ? state_->color_contract : empty;
}

const std::string& VisualRenderProgram::color_contract_digest() const noexcept {
  static const std::string empty;
  return state_ ? state_->color_contract_digest : empty;
}

VisualRenderProgram compile_visual_render_program(
    const core::ProjectSnapshot& project) {
  if (project.project_id.value.empty() || !project.composition) {
    throw std::invalid_argument(
        "RFX-RENDER-PROGRAM-002: project and composition identities are required");
  }
  const auto validation = core::validate_composition(*project.composition);
  if (!validation.valid) {
    throw std::invalid_argument(validation.code + ": " + validation.message);
  }
  return VisualRenderProgramAccess::make(project);
}

VisualRenderPlan evaluate_visual_render_plan(
    const VisualRenderProgram& program,
    const ProjectTimeNs project_time_ns,
    const std::uint64_t clock_epoch,
    core::TextLayoutPort& text_layout_port) {
  const auto& project = VisualRenderProgramAccess::state(program).project;
  const auto& composition = *project.composition;
  if (project_time_ns > composition.duration) {
    throw std::out_of_range(
        "RFX-RENDER-PLAN-002: ProjectTime is outside Composition duration");
  }

  VisualRenderPlan result{
      .stamp = {.project_id = project.project_id.value,
                .revision = project.revision_id.value,
                .composition_id = composition.composition_id.value,
                .project_time_ns = project_time_ns,
                .clock_epoch = clock_epoch},
      .canvas_width_pixels = composition.canvas.width_pixels,
      .canvas_height_pixels = composition.canvas.height_pixels,
      .color_contract = program.color_contract(),
      .color_contract_digest = program.color_contract_digest(),
  };
  const auto scene = core::evaluate_visual_scene(
      composition, project_time_ns, text_layout_port);
  result.layers.reserve(scene.layers.size());
  for (const auto& layer : scene.layers) {
    DrawLayer lowered{
        .layer_id = layer.layer_id.value,
        .world_transform = lower_transform(layer.world_transform),
        .effective_opacity = layer.effective_opacity,
        .blend_mode = lower_blend(layer.blend_mode),
        .isolation_bounds = lower_rect(layer.bounds.effect_local),
        .content = lower_content(layer),
    };
    lowered.masks.reserve(layer.masks.size());
    for (const auto& mask : layer.masks) {
      if (!mask.enabled) {
        continue;
      }
      const auto kind = core::visual_mask_kind(mask);
      const auto* descriptor =
          core::find_visual_contribution_descriptor(kind);
      if (descriptor == nullptr) {
        throw std::invalid_argument(
            "RFX-RENDER-CONTRIBUTION-002: mask descriptor is not registered");
      }
      lowered.masks.push_back({.descriptor_id = descriptor->id,
                               .capability_id = descriptor->capability_id,
                               .schema_version = descriptor->schema_version,
                               .inverted = mask.inverted,
                               .position_x = mask.geometry.position_x,
                               .position_y = mask.geometry.position_y,
                               .width = mask.geometry.width,
                               .height = mask.geometry.height,
                               .corner_radius = mask.geometry.corner_radius});
    }
    lowered.effects.reserve(layer.effects.size());
    for (const auto& effect : layer.effects) {
      if (effect.enabled) {
        lowered.effects.push_back(lower_effect(effect));
      }
    }
    result.layers.push_back(std::move(lowered));
  }
  result.semantic_digest = plan_digest(result);
  return result;
}

}  // namespace refusion::runtime::render
