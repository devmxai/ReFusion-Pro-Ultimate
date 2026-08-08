#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace refusion::core {

struct VisualPropertyId final {
  std::string value;

  friend bool operator==(const VisualPropertyId&,
                         const VisualPropertyId&) = default;
};

enum class VisualPropertyOwner : std::uint8_t {
  group,
  shape,
  text,
};

enum class VisualPropertyValueKind : std::uint8_t {
  number,
  color_rgba8,
  text,
  boolean,
  shape_fill,
};

using VisualPropertyValue =
    std::variant<double, ColorRgba8, std::string, bool, ShapeFill>;

struct VisualPropertyDescriptor final {
  VisualPropertyId id;
  std::string display_name;
  VisualPropertyValueKind value_kind{VisualPropertyValueKind::number};
  std::string unit;
  std::vector<VisualPropertyOwner> owners;
  std::optional<double> minimum;
  std::optional<double> maximum;
  bool animatable{false};
  bool writable{true};

  friend bool operator==(const VisualPropertyDescriptor&,
                         const VisualPropertyDescriptor&) = default;
};

struct VisualPropertyRecord final {
  VisualPropertyDescriptor descriptor;
  VisualPropertyValue value;

  friend bool operator==(const VisualPropertyRecord&,
                         const VisualPropertyRecord&) = default;
};

// The registry is the one portable source for property identity, value type,
// units, validation metadata and read/write behavior. Studio and Agent surfaces
// project this data; they do not define independent property semantics.
[[nodiscard]] const std::vector<VisualPropertyDescriptor>&
visual_property_descriptors();

// Stable digest and Agent projection are derived from the exact descriptor
// collection used by Inspector/property commands. RFX4+ records this digest and
// rejects a mismatched vocabulary before parsing project declarations.
[[nodiscard]] const std::string& visual_property_registry_digest();
[[nodiscard]] std::string visual_property_registry_markdown();

[[nodiscard]] std::vector<VisualPropertyRecord> inspect_visual_properties(
    const CompositionSnapshot& composition,
    const VisualNodeRef& node);

// Mutates only the caller-owned candidate. ProjectAuthority publishes it only
// after this function and full Composition validation succeed.
[[nodiscard]] CompositionValidation set_visual_property(
    CompositionSnapshot& candidate,
    const VisualNodeRef& node,
    const VisualPropertyId& property_id,
    const VisualPropertyValue& value);

}  // namespace refusion::core
