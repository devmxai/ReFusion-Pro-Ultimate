#include "refusion/core/VisualContributionRegistry.hpp"

#include "refusion/core/CanonicalText.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <locale>
#include <sstream>
#include <string_view>
#include <utility>

namespace refusion::core {
namespace {

using EffectGetter =
    std::function<std::vector<VisualParameterRecord>(const LayerEffect&)>;
using EffectSetter = std::function<bool(
    LayerEffect&, const std::string&, const VisualParameterValue&)>;
using EffectFactory = std::function<LayerEffect(EffectId)>;
using MaskGetter =
    std::function<std::vector<VisualParameterRecord>(const LayerMask&)>;
using MaskSetter = std::function<bool(
    LayerMask&, const std::string&, const VisualParameterValue&)>;
using MaskFactory =
    std::function<LayerMask(MaskId, double, double, double)>;

struct RegistryEntry final {
  VisualContributionDescriptor descriptor;
  std::function<bool(const LayerEffect&)> matches_effect;
  EffectGetter effect_getter;
  EffectSetter effect_setter;
  EffectFactory effect_factory;
  std::function<bool(const LayerMask&)> matches_mask;
  MaskGetter mask_getter;
  MaskSetter mask_setter;
  MaskFactory mask_factory;
};

[[nodiscard]] VisualParameterDescriptor number_parameter(
    std::string id,
    std::string display_name,
    std::string unit,
    std::optional<double> minimum = std::nullopt,
    std::optional<double> maximum = std::nullopt,
    const bool minimum_inclusive = true,
    const bool maximum_inclusive = true,
    const bool animatable = false) {
  return VisualParameterDescriptor{
      .id = std::move(id),
      .display_name = std::move(display_name),
      .value_kind = VisualParameterValueKind::number,
      .unit = std::move(unit),
      .minimum = minimum,
      .maximum = maximum,
      .minimum_inclusive = minimum_inclusive,
      .maximum_inclusive = maximum_inclusive,
      .animatable = animatable,
  };
}

[[nodiscard]] VisualParameterDescriptor color_parameter(
    std::string id,
    std::string display_name) {
  return VisualParameterDescriptor{
      .id = std::move(id),
      .display_name = std::move(display_name),
      .value_kind = VisualParameterValueKind::color_rgba8,
      .unit = "rgba8",
  };
}

[[nodiscard]] const std::vector<VisualContributionOwner>& layer_owners() {
  static const std::vector<VisualContributionOwner> owners{
      VisualContributionOwner::shape,
      VisualContributionOwner::text,
  };
  return owners;
}

[[nodiscard]] VisualParameterRecord record(
    const VisualContributionDescriptor& descriptor,
    const std::string_view parameter_id,
    VisualParameterValue value) {
  const auto found = std::find_if(
      descriptor.parameters.begin(), descriptor.parameters.end(),
      [parameter_id](const VisualParameterDescriptor& parameter) {
        return parameter.id == parameter_id;
      });
  return VisualParameterRecord{
      .descriptor = *found,
      .value = std::move(value),
  };
}

[[nodiscard]] const std::vector<RegistryEntry>& entries() {
  static const std::vector<RegistryEntry> registry = [] {
    std::vector<RegistryEntry> result;

    VisualContributionDescriptor rounded_rect{
        .id = "rounded_rect",
        .capability_id = "visual.mask.rounded_rect.v1",
        .display_name = "Rounded Rectangle Mask",
        .category = VisualContributionCategory::mask,
        .owners = layer_owners(),
        .parameters = {
            number_parameter("position_x", "Position X", "local_px"),
            number_parameter("position_y", "Position Y", "local_px"),
            number_parameter("width", "Width", "local_px", 0.0,
                             std::nullopt, false),
            number_parameter("height", "Height", "local_px", 0.0,
                             std::nullopt, false),
            number_parameter("corner_radius", "Corner Radius", "local_px",
                             0.0),
        },
    };
    result.push_back(RegistryEntry{
        .descriptor = rounded_rect,
        .matches_mask = [](const LayerMask&) { return true; },
        .mask_getter = [rounded_rect](const LayerMask& mask) {
          return std::vector<VisualParameterRecord>{
              record(rounded_rect, "position_x", mask.geometry.position_x),
              record(rounded_rect, "position_y", mask.geometry.position_y),
              record(rounded_rect, "width", mask.geometry.width),
              record(rounded_rect, "height", mask.geometry.height),
              record(rounded_rect, "corner_radius",
                     mask.geometry.corner_radius),
          };
        },
        .mask_setter = [](LayerMask& mask, const std::string& id,
                          const VisualParameterValue& value) {
          const auto* number = std::get_if<double>(&value);
          if (number == nullptr) {
            return false;
          }
          if (id == "position_x") mask.geometry.position_x = *number;
          else if (id == "position_y") mask.geometry.position_y = *number;
          else if (id == "width") mask.geometry.width = *number;
          else if (id == "height") mask.geometry.height = *number;
          else if (id == "corner_radius") mask.geometry.corner_radius = *number;
          else return false;
          return true;
        },
        .mask_factory = [](MaskId id, const double width,
                           const double height, const double corner_radius) {
          return LayerMask{
              .mask_id = std::move(id),
              .enabled = true,
              .inverted = false,
              .geometry = RoundedRectMask{
                  .width = width,
                  .height = height,
                  .corner_radius = corner_radius,
              },
          };
        },
    });

    VisualContributionDescriptor blur{
        .id = "gaussian_blur",
        .capability_id = "visual.fx.gaussian_blur.v1",
        .display_name = "Gaussian Blur",
        .category = VisualContributionCategory::effect,
        .owners = layer_owners(),
        .parameters = {
            number_parameter("sigma_x", "Sigma X", "local_px", 0.0,
                             256.0),
            number_parameter("sigma_y", "Sigma Y", "local_px", 0.0,
                             256.0),
        },
    };
    result.push_back(RegistryEntry{
        .descriptor = blur,
        .matches_effect = [](const LayerEffect& effect) {
          return std::holds_alternative<GaussianBlurEffect>(effect.parameters);
        },
        .effect_getter = [blur](const LayerEffect& effect) {
          const auto& parameters =
              std::get<GaussianBlurEffect>(effect.parameters);
          return std::vector<VisualParameterRecord>{
              record(blur, "sigma_x", parameters.sigma_x),
              record(blur, "sigma_y", parameters.sigma_y),
          };
        },
        .effect_setter = [](LayerEffect& effect, const std::string& id,
                            const VisualParameterValue& value) {
          auto* parameters = std::get_if<GaussianBlurEffect>(&effect.parameters);
          const auto* number = std::get_if<double>(&value);
          if (parameters == nullptr || number == nullptr) return false;
          if (id == "sigma_x") parameters->sigma_x = *number;
          else if (id == "sigma_y") parameters->sigma_y = *number;
          else return false;
          return true;
        },
        .effect_factory = [](EffectId id) {
          return LayerEffect{
              .effect_id = std::move(id),
              .enabled = true,
              .parameters = GaussianBlurEffect{.sigma_x = 8.0, .sigma_y = 8.0},
          };
        },
    });

    VisualContributionDescriptor shadow{
        .id = "drop_shadow",
        .capability_id = "visual.fx.drop_shadow.v1",
        .display_name = "Drop Shadow",
        .category = VisualContributionCategory::effect,
        .owners = layer_owners(),
        .parameters = {
            number_parameter("offset_x", "Offset X", "local_px"),
            number_parameter("offset_y", "Offset Y", "local_px"),
            number_parameter("sigma_x", "Sigma X", "local_px", 0.0,
                             256.0),
            number_parameter("sigma_y", "Sigma Y", "local_px", 0.0,
                             256.0),
            color_parameter("color", "Color"),
        },
    };
    result.push_back(RegistryEntry{
        .descriptor = shadow,
        .matches_effect = [](const LayerEffect& effect) {
          return std::holds_alternative<DropShadowEffect>(effect.parameters);
        },
        .effect_getter = [shadow](const LayerEffect& effect) {
          const auto& parameters =
              std::get<DropShadowEffect>(effect.parameters);
          return std::vector<VisualParameterRecord>{
              record(shadow, "offset_x", parameters.offset_x),
              record(shadow, "offset_y", parameters.offset_y),
              record(shadow, "sigma_x", parameters.sigma_x),
              record(shadow, "sigma_y", parameters.sigma_y),
              record(shadow, "color", parameters.color),
          };
        },
        .effect_setter = [](LayerEffect& effect, const std::string& id,
                            const VisualParameterValue& value) {
          auto* parameters = std::get_if<DropShadowEffect>(&effect.parameters);
          if (parameters == nullptr) return false;
          if (id == "color") {
            const auto* color = std::get_if<ColorRgba8>(&value);
            if (color == nullptr) return false;
            parameters->color = *color;
            return true;
          }
          const auto* number = std::get_if<double>(&value);
          if (number == nullptr) return false;
          if (id == "offset_x") parameters->offset_x = *number;
          else if (id == "offset_y") parameters->offset_y = *number;
          else if (id == "sigma_x") parameters->sigma_x = *number;
          else if (id == "sigma_y") parameters->sigma_y = *number;
          else return false;
          return true;
        },
        .effect_factory = [](EffectId id) {
          return LayerEffect{
              .effect_id = std::move(id),
              .enabled = true,
              .parameters = DropShadowEffect{
                  .offset_x = 12.0,
                  .offset_y = 16.0,
                  .sigma_x = 14.0,
                  .sigma_y = 14.0,
                  .color = ColorRgba8{.alpha = 128},
              },
          };
        },
    });

    VisualContributionDescriptor glow{
        .id = "glow",
        .capability_id = "visual.fx.glow.v1",
        .display_name = "Glow",
        .category = VisualContributionCategory::effect,
        .owners = layer_owners(),
        .parameters = {
            number_parameter("sigma", "Sigma", "local_px", 0.0, 256.0),
            color_parameter("color", "Color"),
        },
    };
    result.push_back(RegistryEntry{
        .descriptor = glow,
        .matches_effect = [](const LayerEffect& effect) {
          return std::holds_alternative<GlowEffect>(effect.parameters);
        },
        .effect_getter = [glow](const LayerEffect& effect) {
          const auto& parameters = std::get<GlowEffect>(effect.parameters);
          return std::vector<VisualParameterRecord>{
              record(glow, "sigma", parameters.sigma),
              record(glow, "color", parameters.color),
          };
        },
        .effect_setter = [](LayerEffect& effect, const std::string& id,
                            const VisualParameterValue& value) {
          auto* parameters = std::get_if<GlowEffect>(&effect.parameters);
          if (parameters == nullptr) return false;
          if (id == "sigma") {
            const auto* number = std::get_if<double>(&value);
            if (number == nullptr) return false;
            parameters->sigma = *number;
            return true;
          }
          if (id == "color") {
            const auto* color = std::get_if<ColorRgba8>(&value);
            if (color == nullptr) return false;
            parameters->color = *color;
            return true;
          }
          return false;
        },
        .effect_factory = [](EffectId id) {
          return LayerEffect{
              .effect_id = std::move(id),
              .enabled = true,
              .parameters = GlowEffect{
                  .sigma = 18.0,
                  .color = ColorRgba8{
                      .red = 124,
                      .green = 92,
                      .blue = 255,
                      .alpha = 192,
                  },
              },
          };
        },
    });
    return result;
  }();
  return registry;
}

[[nodiscard]] CompositionValidation rejected(std::string code,
                                             std::string message) {
  return CompositionValidation{
      .valid = false,
      .code = std::move(code),
      .message = std::move(message),
  };
}

[[nodiscard]] std::string_view parameter_kind_name(
    const VisualParameterValueKind kind) {
  switch (kind) {
    case VisualParameterValueKind::number:
      return "number";
    case VisualParameterValueKind::color_rgba8:
      return "color_rgba8";
    case VisualParameterValueKind::boolean:
      return "boolean";
  }
  return "unsupported";
}

[[nodiscard]] const RegistryEntry* effect_entry(const LayerEffect& effect) {
  const auto found = std::find_if(entries().begin(), entries().end(),
                                  [&effect](const RegistryEntry& entry) {
    return entry.matches_effect && entry.matches_effect(effect);
  });
  return found == entries().end() ? nullptr : &*found;
}

[[nodiscard]] const RegistryEntry* mask_entry(const LayerMask& mask) {
  const auto found = std::find_if(entries().begin(), entries().end(),
                                  [&mask](const RegistryEntry& entry) {
    return entry.matches_mask && entry.matches_mask(mask);
  });
  return found == entries().end() ? nullptr : &*found;
}

[[nodiscard]] CompositionValidation validate_records(
    const std::vector<VisualParameterRecord>& records,
    const std::string& contribution_id) {
  for (const auto& record_value : records) {
    const auto& descriptor = record_value.descriptor;
    switch (descriptor.value_kind) {
      case VisualParameterValueKind::number: {
        const auto* number = std::get_if<double>(&record_value.value);
        if (number == nullptr || !std::isfinite(*number)) {
          return rejected("RFX-CONTRIBUTION-PARAMETER-400",
                          contribution_id + "." + descriptor.id +
                              " must be a finite number");
        }
        if (descriptor.minimum &&
            (descriptor.minimum_inclusive
                 ? *number < *descriptor.minimum
                 : *number <= *descriptor.minimum)) {
          return rejected("RFX-CONTRIBUTION-RANGE-400",
                          contribution_id + "." + descriptor.id +
                              " is below its admitted range");
        }
        if (descriptor.maximum &&
            (descriptor.maximum_inclusive
                 ? *number > *descriptor.maximum
                 : *number >= *descriptor.maximum)) {
          return rejected("RFX-CONTRIBUTION-RANGE-400",
                          contribution_id + "." + descriptor.id +
                              " exceeds its admitted range");
        }
        break;
      }
      case VisualParameterValueKind::color_rgba8:
        if (!std::holds_alternative<ColorRgba8>(record_value.value)) {
          return rejected("RFX-CONTRIBUTION-PARAMETER-400",
                          contribution_id + "." + descriptor.id +
                              " must be rgba8");
        }
        break;
      case VisualParameterValueKind::boolean:
        if (!std::holds_alternative<bool>(record_value.value)) {
          return rejected("RFX-CONTRIBUTION-PARAMETER-400",
                          contribution_id + "." + descriptor.id +
                              " must be boolean");
        }
        break;
    }
  }
  return CompositionValidation{.valid = true};
}

}  // namespace

const std::vector<VisualContributionDescriptor>&
visual_contribution_descriptors() {
  static const std::vector<VisualContributionDescriptor> descriptors = [] {
    std::vector<VisualContributionDescriptor> result;
    result.reserve(entries().size());
    for (const auto& entry : entries()) result.push_back(entry.descriptor);
    return result;
  }();
  return descriptors;
}

const VisualContributionDescriptor* find_visual_contribution_descriptor(
    const std::string& contribution_id) {
  const auto& descriptors = visual_contribution_descriptors();
  const auto found = std::find_if(
      descriptors.begin(), descriptors.end(),
      [&contribution_id](const VisualContributionDescriptor& descriptor) {
        return descriptor.id == contribution_id;
      });
  return found == descriptors.end() ? nullptr : &*found;
}

const std::string& visual_contribution_registry_digest() {
  static const std::string digest = [] {
    std::uint64_t hash = 14695981039346656037ULL;
    const auto append = [&hash](const std::string_view text) {
      for (const unsigned char byte : text) {
        hash ^= byte;
        hash *= 1099511628211ULL;
      }
      hash ^= 0xffU;
      hash *= 1099511628211ULL;
    };
    for (const auto& descriptor : visual_contribution_descriptors()) {
      append(descriptor.id);
      append(descriptor.capability_id);
      append(descriptor.display_name);
      append(canonical_uint64(static_cast<unsigned>(descriptor.category)));
      append(canonical_uint64(descriptor.schema_version));
      for (const auto owner : descriptor.owners) {
        append(canonical_uint64(static_cast<unsigned>(owner)));
      }
      for (const auto& parameter : descriptor.parameters) {
        append(parameter.id);
        append(parameter.display_name);
        append(canonical_uint64(static_cast<unsigned>(parameter.value_kind)));
        append(parameter.unit);
        append(parameter.minimum
                   ? canonical_fixed6_float64(*parameter.minimum)
                   : "-");
        append(parameter.maximum
                   ? canonical_fixed6_float64(*parameter.maximum)
                   : "-");
        append(parameter.minimum_inclusive ? "min-inclusive" : "min-exclusive");
        append(parameter.maximum_inclusive ? "max-inclusive" : "max-exclusive");
        append(parameter.animatable ? "animated" : "static");
      }
    }
    return "rfx-vc-fnv1a64:" + canonical_hex64(hash);
  }();
  return digest;
}

std::string visual_contribution_registry_markdown() {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << "# Generated Mask and FX contribution registry\n\n"
         << "Digest: `" << visual_contribution_registry_digest() << "`\n\n"
         << "This file is generated from the same Core descriptors used by "
            "validation, Inspector and Agent commands. Do not hand-edit it.\n\n";
  for (const auto& descriptor : visual_contribution_descriptors()) {
    output << "## `" << descriptor.id << "` — " << descriptor.display_name
           << "\n\nCapability: `" << descriptor.capability_id << "`\n\n"
           << "| Parameter | Type | Unit | Range | Animatable |\n"
           << "|---|---|---|---|---:|\n";
    for (const auto& parameter : descriptor.parameters) {
      output << "| `" << parameter.id << "` | `"
             << parameter_kind_name(parameter.value_kind) << "` | `"
             << parameter.unit << "` | ";
      if (parameter.minimum) output << *parameter.minimum;
      else output << "-inf";
      output << " .. ";
      if (parameter.maximum) output << *parameter.maximum;
      else output << "+inf";
      output << " | " << (parameter.animatable ? "yes" : "no") << " |\n";
    }
    output << '\n';
  }
  return output.str();
}

std::string visual_effect_kind(const LayerEffect& effect) {
  const auto* entry = effect_entry(effect);
  return entry == nullptr ? std::string{} : entry->descriptor.id;
}

std::string visual_mask_kind(const LayerMask& mask) {
  const auto* entry = mask_entry(mask);
  return entry == nullptr ? std::string{} : entry->descriptor.id;
}

std::optional<LayerEffect> make_default_visual_effect(
    const std::string& contribution_id,
    EffectId effect_id) {
  const auto found = std::find_if(entries().begin(), entries().end(),
      [&contribution_id](const RegistryEntry& entry) {
        return entry.descriptor.id == contribution_id &&
               entry.descriptor.category == VisualContributionCategory::effect;
      });
  if (found == entries().end() || !found->effect_factory) return std::nullopt;
  return found->effect_factory(std::move(effect_id));
}

std::optional<LayerMask> make_default_visual_mask(
    const std::string& contribution_id,
    MaskId mask_id,
    const double width,
    const double height,
    const double corner_radius) {
  const auto found = std::find_if(entries().begin(), entries().end(),
      [&contribution_id](const RegistryEntry& entry) {
        return entry.descriptor.id == contribution_id &&
               entry.descriptor.category == VisualContributionCategory::mask;
      });
  if (found == entries().end() || !found->mask_factory) return std::nullopt;
  auto result = found->mask_factory(std::move(mask_id), width, height,
                                    corner_radius);
  if (!validate_visual_mask(result).valid) return std::nullopt;
  return result;
}

std::vector<VisualParameterRecord> inspect_visual_effect_parameters(
    const LayerEffect& effect) {
  const auto* entry = effect_entry(effect);
  return entry == nullptr ? std::vector<VisualParameterRecord>{}
                          : entry->effect_getter(effect);
}

std::vector<VisualParameterRecord> inspect_visual_mask_parameters(
    const LayerMask& mask) {
  const auto* entry = mask_entry(mask);
  return entry == nullptr ? std::vector<VisualParameterRecord>{}
                          : entry->mask_getter(mask);
}

CompositionValidation set_visual_effect_parameter(
    LayerEffect& effect,
    const std::string& parameter_id,
    const VisualParameterValue& value) {
  const auto* entry = effect_entry(effect);
  if (entry == nullptr) {
    return rejected("RFX-EFFECT-KIND-400", "effect kind is not registered");
  }
  auto candidate = effect;
  if (!entry->effect_setter(candidate, parameter_id, value)) {
    return rejected("RFX-EFFECT-PARAMETER-400",
                    "effect parameter ID or value type is invalid");
  }
  const auto validation = validate_visual_effect(candidate);
  if (validation.valid) effect = std::move(candidate);
  return validation;
}

CompositionValidation set_visual_mask_parameter(
    LayerMask& mask,
    const std::string& parameter_id,
    const VisualParameterValue& value) {
  const auto* entry = mask_entry(mask);
  if (entry == nullptr) {
    return rejected("RFX-MASK-KIND-400", "mask kind is not registered");
  }
  auto candidate = mask;
  if (!entry->mask_setter(candidate, parameter_id, value)) {
    return rejected("RFX-MASK-PARAMETER-400",
                    "mask parameter ID or value type is invalid");
  }
  const auto validation = validate_visual_mask(candidate);
  if (validation.valid) mask = std::move(candidate);
  return validation;
}

CompositionValidation validate_visual_effect(const LayerEffect& effect) {
  const auto* entry = effect_entry(effect);
  if (entry == nullptr) {
    return rejected("RFX-EFFECT-KIND-400", "effect kind is not registered");
  }
  const auto validation =
      validate_records(entry->effect_getter(effect), entry->descriptor.id);
  if (validation.valid) return validation;
  if (entry->descriptor.id == "gaussian_blur") {
    return rejected("RFX-PROJECT-132",
                    "Gaussian Blur sigma must be within [0, 256] px");
  }
  if (entry->descriptor.id == "drop_shadow") {
    return rejected("RFX-PROJECT-133",
                    "Drop Shadow parameters are invalid");
  }
  if (entry->descriptor.id == "glow") {
    return rejected("RFX-PROJECT-134",
                    "Glow sigma must be within [0, 256] px");
  }
  return validation;
}

CompositionValidation validate_visual_mask(const LayerMask& mask) {
  const auto* entry = mask_entry(mask);
  if (entry == nullptr) {
    return rejected("RFX-MASK-KIND-400", "mask kind is not registered");
  }
  const auto validation =
      validate_records(entry->mask_getter(mask), entry->descriptor.id);
  if (!validation.valid && entry->descriptor.id == "rounded_rect") {
    return rejected("RFX-PROJECT-140",
                    "rounded rectangle mask geometry is invalid");
  }
  return validation;
}

}  // namespace refusion::core
