#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace refusion::core {

enum class VisualContributionCategory : std::uint8_t {
  mask,
  effect,
};

enum class VisualContributionOwner : std::uint8_t {
  shape,
  text,
};

enum class VisualParameterValueKind : std::uint8_t {
  number,
  color_rgba8,
  boolean,
};

using VisualParameterValue = std::variant<double, ColorRgba8, bool>;

struct VisualParameterDescriptor final {
  std::string id;
  std::string display_name;
  VisualParameterValueKind value_kind{VisualParameterValueKind::number};
  std::string unit;
  std::optional<double> minimum;
  std::optional<double> maximum;
  bool minimum_inclusive{true};
  bool maximum_inclusive{true};
  bool animatable{false};

  friend bool operator==(const VisualParameterDescriptor&,
                         const VisualParameterDescriptor&) = default;
};

struct VisualContributionDescriptor final {
  std::string id;
  std::string capability_id;
  std::string display_name;
  VisualContributionCategory category{VisualContributionCategory::effect};
  std::uint32_t schema_version{1};
  std::vector<VisualContributionOwner> owners;
  std::vector<VisualParameterDescriptor> parameters;

  friend bool operator==(const VisualContributionDescriptor&,
                         const VisualContributionDescriptor&) = default;
};

struct VisualParameterRecord final {
  VisualParameterDescriptor descriptor;
  VisualParameterValue value;

  friend bool operator==(const VisualParameterRecord&,
                         const VisualParameterRecord&) = default;
};

// One portable descriptor table owns IDs, defaults, parameter types/ranges,
// Inspector/Agent metadata and validation for every admitted Mask and FX.
[[nodiscard]] const std::vector<VisualContributionDescriptor>&
visual_contribution_descriptors();
[[nodiscard]] const VisualContributionDescriptor*
find_visual_contribution_descriptor(const std::string& contribution_id);
[[nodiscard]] const std::string& visual_contribution_registry_digest();
[[nodiscard]] std::string visual_contribution_registry_markdown();

[[nodiscard]] std::string visual_effect_kind(const LayerEffect& effect);
[[nodiscard]] std::string visual_mask_kind(const LayerMask& mask);

[[nodiscard]] std::optional<LayerEffect> make_default_visual_effect(
    const std::string& contribution_id,
    EffectId effect_id);
[[nodiscard]] std::optional<LayerMask> make_default_visual_mask(
    const std::string& contribution_id,
    MaskId mask_id,
    double width,
    double height,
    double corner_radius);

[[nodiscard]] std::vector<VisualParameterRecord>
inspect_visual_effect_parameters(const LayerEffect& effect);
[[nodiscard]] std::vector<VisualParameterRecord>
inspect_visual_mask_parameters(const LayerMask& mask);

[[nodiscard]] CompositionValidation set_visual_effect_parameter(
    LayerEffect& effect,
    const std::string& parameter_id,
    const VisualParameterValue& value);
[[nodiscard]] CompositionValidation set_visual_mask_parameter(
    LayerMask& mask,
    const std::string& parameter_id,
    const VisualParameterValue& value);

[[nodiscard]] CompositionValidation validate_visual_effect(
    const LayerEffect& effect);
[[nodiscard]] CompositionValidation validate_visual_mask(
    const LayerMask& mask);

}  // namespace refusion::core
