#include "refusion/core/ProjectDocument.hpp"
#include "refusion/core/CanonicalText.hpp"
#include "refusion/core/TextLayout.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace refusion::core {
namespace {

[[nodiscard]] bool finite(const double value) noexcept {
  return std::isfinite(value);
}

[[nodiscard]] bool blank(const std::string& value) {
  return value.empty() ||
         std::all_of(value.begin(), value.end(), [](const unsigned char character) {
           return character == ' ' || character == '\t' || character == '\n' ||
                  character == '\r';
         });
}

[[nodiscard]] bool sha256_digest(const std::string& value) noexcept {
  constexpr std::string_view prefix{"sha256:"};
  if (value.size() != prefix.size() + 64 ||
      value.compare(0, prefix.size(), prefix) != 0) {
    return false;
  }
  return std::all_of(value.begin() + static_cast<std::ptrdiff_t>(prefix.size()),
                     value.end(), [](const unsigned char character) {
                       return (character >= '0' && character <= '9') ||
                              (character >= 'a' && character <= 'f');
                     });
}

[[nodiscard]] CompositionValidation rejected(std::string code,
                                             std::string message) {
  return CompositionValidation{
      .valid = false,
      .code = std::move(code),
      .message = std::move(message),
  };
}

[[nodiscard]] double default_property_value(const Transform2D& transform,
                                            const AnimatedProperty property) noexcept {
  switch (property) {
    case AnimatedProperty::position_x:
      return transform.position_x;
    case AnimatedProperty::position_y:
      return transform.position_y;
    case AnimatedProperty::scale_x:
      return transform.scale_x;
    case AnimatedProperty::scale_y:
      return transform.scale_y;
    case AnimatedProperty::rotation_degrees:
      return transform.rotation_degrees;
    case AnimatedProperty::opacity:
      return transform.opacity;
  }
  return 0.0;
}

[[nodiscard]] std::string node_key(const VisualNodeRef& node) {
  if (const auto* layer = std::get_if<LayerId>(&node)) {
    return "layer:" + layer->value;
  }
  return "group:" + std::get<LayerGroupId>(node).value;
}

[[nodiscard]] bool value_is_admitted_for_animation(
    const AnimatedProperty property,
    const double value) noexcept {
  if (!finite(value)) return false;
  switch (property) {
    case AnimatedProperty::position_x:
    case AnimatedProperty::position_y:
    case AnimatedProperty::rotation_degrees:
      return true;
    case AnimatedProperty::scale_x:
    case AnimatedProperty::scale_y:
      return value > 0.0;
    case AnimatedProperty::opacity:
      return value >= 0.0 && value <= 1.0;
  }
  return false;
}

[[nodiscard]] std::optional<CompositionValidation> validate_node_properties(
    const Transform2D& transform,
    const std::vector<ScalarAnimation>& animations,
    const TimeRangeNs& active_range,
    const bool pass_through_group) {
  if (!finite(transform.position_x) || !finite(transform.position_y) ||
      !finite(transform.anchor_x) || !finite(transform.anchor_y) ||
      !finite(transform.scale_x) || !finite(transform.scale_y) ||
      !finite(transform.rotation_degrees) || !finite(transform.opacity) ||
      transform.scale_x <= 0.0 || transform.scale_y <= 0.0 ||
      transform.opacity < 0.0 || transform.opacity > 1.0) {
    return rejected("RFX-PROJECT-110",
                    "visual transform contains invalid values");
  }
  if (pass_through_group && transform.opacity != 1.0) {
    return rejected(
        "RFX-PROJECT-121",
        "EXP-002 pass-through groups require opacity 1 until isolated group compositing is admitted");
  }

  std::unordered_set<std::uint8_t> animated_properties;
  for (const auto& animation : animations) {
    const auto key = static_cast<std::uint8_t>(animation.property);
    if (!animated_properties.emplace(key).second ||
        animation.keyframes.empty()) {
      return rejected("RFX-PROJECT-111",
                      "animated properties must be unique and have keyframes");
    }
    if (pass_through_group &&
        animation.property == AnimatedProperty::opacity) {
      return rejected(
          "RFX-PROJECT-121",
          "EXP-002 pass-through groups cannot animate opacity before isolated compositing is admitted");
    }
    ProjectTimeNs previous_time = 0;
    bool first = true;
    for (const auto& keyframe : animation.keyframes) {
      if (!value_is_admitted_for_animation(animation.property,
                                           keyframe.value)) {
        return rejected(
            "RFX-PROJECT-113",
            "keyframe value is outside the admitted animated-property range");
      }
      if (keyframe.time < active_range.start ||
          keyframe.time > active_range.end() ||
          (!first && keyframe.time <= previous_time)) {
        return rejected(
            "RFX-PROJECT-112",
            "keyframes must be finite, ordered and inside the visual range");
      }
      previous_time = keyframe.time;
      first = false;
    }
  }
  return std::nullopt;
}

[[nodiscard]] double evaluate_property(
    const Transform2D& transform,
    const std::vector<ScalarAnimation>& animations,
    const AnimatedProperty property,
    const ProjectTimeNs composition_time) noexcept {
  const auto default_value = default_property_value(transform, property);
  const auto animation = std::find_if(
      animations.begin(), animations.end(),
      [property](const ScalarAnimation& candidate) {
        return candidate.property == property;
      });
  if (animation == animations.end() || animation->keyframes.empty()) {
    return default_value;
  }

  const auto& keyframes = animation->keyframes;
  if (composition_time <= keyframes.front().time) {
    return keyframes.front().value;
  }
  if (composition_time >= keyframes.back().time) {
    return keyframes.back().value;
  }

  const auto upper = std::upper_bound(
      keyframes.begin(), keyframes.end(), composition_time,
      [](const ProjectTimeNs time, const ScalarKeyframe& keyframe) {
        return time < keyframe.time;
      });
  const auto& right = *upper;
  const auto& left = *(upper - 1);
  const auto span = right.time - left.time;
  const auto offset = composition_time - left.time;
  const double progress = static_cast<double>(offset) / static_cast<double>(span);
  return left.value + (right.value - left.value) * progress;
}

[[nodiscard]] AffineTransform2D multiply(const AffineTransform2D& lhs,
                                         const AffineTransform2D& rhs) noexcept {
  return AffineTransform2D{
      .m00 = lhs.m00 * rhs.m00 + lhs.m01 * rhs.m10,
      .m01 = lhs.m00 * rhs.m01 + lhs.m01 * rhs.m11,
      .m02 = lhs.m00 * rhs.m02 + lhs.m01 * rhs.m12 + lhs.m02,
      .m10 = lhs.m10 * rhs.m00 + lhs.m11 * rhs.m10,
      .m11 = lhs.m10 * rhs.m01 + lhs.m11 * rhs.m11,
      .m12 = lhs.m10 * rhs.m02 + lhs.m11 * rhs.m12 + lhs.m12,
  };
}

[[nodiscard]] AffineTransform2D evaluated_matrix(
    const Transform2D& transform,
    const std::vector<ScalarAnimation>& animations,
    const ProjectTimeNs composition_time) noexcept {
  constexpr double kPi = 3.14159265358979323846;
  const double position_x = evaluate_property(
      transform, animations, AnimatedProperty::position_x, composition_time);
  const double position_y = evaluate_property(
      transform, animations, AnimatedProperty::position_y, composition_time);
  const double scale_x = evaluate_property(
      transform, animations, AnimatedProperty::scale_x, composition_time);
  const double scale_y = evaluate_property(
      transform, animations, AnimatedProperty::scale_y, composition_time);
  const double rotation = evaluate_property(
      transform, animations, AnimatedProperty::rotation_degrees,
      composition_time) *
                          kPi / 180.0;
  const double cosine = std::cos(rotation);
  const double sine = std::sin(rotation);
  const double m00 = cosine * scale_x;
  const double m01 = -sine * scale_y;
  const double m10 = sine * scale_x;
  const double m11 = cosine * scale_y;
  return AffineTransform2D{
      .m00 = m00,
      .m01 = m01,
      .m02 = position_x - m00 * transform.anchor_x -
             m01 * transform.anchor_y,
      .m10 = m10,
      .m11 = m11,
      .m12 = position_y - m10 * transform.anchor_x -
             m11 * transform.anchor_y,
  };
}

}  // namespace

bool RationalRate::valid() const noexcept {
  return numerator != 0 && denominator != 0;
}

bool TimeRangeNs::valid() const noexcept {
  return duration != 0 &&
         start <= std::numeric_limits<ProjectTimeNs>::max() - duration;
}

ProjectTimeNs TimeRangeNs::end() const noexcept {
  if (!valid()) {
    return 0;
  }
  return start + duration;
}

bool TimeRangeNs::contains(const ProjectTimeNs time) const noexcept {
  return valid() && time >= start && time < end();
}

bool CanvasExtent::valid() const noexcept {
  return width_pixels != 0 && height_pixels != 0;
}

bool FontIdentity::qualified() const noexcept {
  return source == FontSourceKind::packaged_asset &&
         !family_name.empty() && !asset_id.empty() &&
         sha256_digest(content_digest);
}

LocalRect text_box_bounds(const TextBox& box) noexcept {
  return LocalRect{
      .left = -box.width * 0.5,
      .top = -box.height * 0.5,
      .right = box.width * 0.5,
      .bottom = box.height * 0.5,
  };
}

LocalRect text_box_content_bounds(const TextBox& box) noexcept {
  auto bounds = text_box_bounds(box);
  bounds.left += box.padding_left;
  bounds.top += box.padding_top;
  bounds.right -= box.padding_right;
  bounds.bottom -= box.padding_bottom;
  return bounds;
}

CompositionValidation validate_composition(
    const CompositionSnapshot& composition) {
  if (!portable_ascii_identifier(composition.composition_id.value)) {
    return rejected("RFX-PROJECT-101",
                    "composition ID must be a portable ASCII identifier");
  }
  if (blank(composition.display_name) ||
      !validate_preserved_utf8(composition.display_name).valid()) {
    return rejected("RFX-PROJECT-102",
                    "composition name must be valid preserved UTF-8");
  }
  if (!composition.canvas.valid()) {
    return rejected("RFX-PROJECT-103", "composition canvas must be non-empty");
  }
  if (!composition.frame_rate.valid()) {
    return rejected("RFX-PROJECT-104", "composition frame rate is invalid");
  }
  if (composition.duration == 0) {
    return rejected("RFX-PROJECT-105", "composition duration must be non-zero");
  }
  std::unordered_set<std::string> layer_ids;
  std::unordered_set<std::string> all_visual_ids;
  std::unordered_set<std::string> mask_ids;
  std::unordered_set<std::string> effect_ids;
  for (const auto& layer : composition.layers) {
    if (!portable_ascii_identifier(layer.layer_id.value) ||
        !layer_ids.emplace(layer.layer_id.value).second) {
      return rejected(
          "RFX-PROJECT-107",
          "layer IDs must be portable ASCII identifiers and unique");
    }
    if (!all_visual_ids.emplace(layer.layer_id.value).second) {
      return rejected("RFX-PROJECT-107",
                      "visual IDs must be non-empty and globally unique");
    }
    if (blank(layer.display_name) ||
        !validate_preserved_utf8(layer.display_name).valid()) {
      return rejected("RFX-PROJECT-108",
                      "layer name must be valid preserved UTF-8");
    }
    if (!layer.active_range.valid() ||
        layer.active_range.end() > composition.duration) {
      return rejected("RFX-PROJECT-109", "layer range exceeds composition duration");
    }
    if (const auto property_failure = validate_node_properties(
            layer.transform, layer.animations, layer.active_range, false)) {
      return *property_failure;
    }
    switch (layer.blend_mode) {
      case BlendMode::normal:
      case BlendMode::multiply:
      case BlendMode::screen:
      case BlendMode::overlay:
        break;
      default:
        return rejected("RFX-PROJECT-137", "Layer blend mode is invalid");
    }

    if (const auto* shape = std::get_if<ShapeLayerContent>(&layer.content)) {
      if (!finite(shape->width) || !finite(shape->height) ||
          !finite(shape->corner_radius) || shape->width <= 0.0 ||
          shape->height <= 0.0 || shape->corner_radius < 0.0 ||
          !finite(shape->stroke_width) || shape->stroke_width < 0.0 ||
          shape->stroke_width > 1024.0) {
        return rejected("RFX-PROJECT-113", "shape geometry is invalid");
      }
      const auto valid_stops = [](const std::vector<GradientStop>& stops) {
        if (stops.size() < 2 || stops.size() > 32) {
          return false;
        }
        double previous = -1.0;
        for (const auto& stop : stops) {
          if (!finite(stop.offset) || stop.offset < 0.0 ||
              stop.offset > 1.0 || stop.offset <= previous) {
            return false;
          }
          previous = stop.offset;
        }
        return true;
      };
      if (const auto* linear =
              std::get_if<LinearGradientFill>(&shape->fill)) {
        if (!finite(linear->start_x) || !finite(linear->start_y) ||
            !finite(linear->end_x) || !finite(linear->end_y) ||
            (linear->start_x == linear->end_x &&
             linear->start_y == linear->end_y) ||
            !valid_stops(linear->stops)) {
          return rejected("RFX-PROJECT-135",
                          "linear gradient geometry or stops are invalid");
        }
      } else if (const auto* radial =
                     std::get_if<RadialGradientFill>(&shape->fill)) {
        if (!finite(radial->center_x) || !finite(radial->center_y) ||
            !finite(radial->radius) || radial->radius <= 0.0 ||
            !valid_stops(radial->stops)) {
          return rejected("RFX-PROJECT-136",
                          "radial gradient geometry or stops are invalid");
        }
      }
    } else if (const auto* text = std::get_if<TextLayerContent>(&layer.content)) {
      const auto& box = text->box;
      if (blank(text->text) || blank(text->font.family_name) ||
          !validate_preserved_utf8(text->text).valid() ||
          !validate_preserved_utf8(text->font.family_name).valid() ||
          !finite(text->font_size) || text->font_size <= 0.0 ||
          text->font_size > 4096.0 || !finite(box.width) ||
          !finite(box.height) || box.width <= 0.0 || box.height <= 0.0 ||
          !finite(box.padding_top) || !finite(box.padding_right) ||
          !finite(box.padding_bottom) || !finite(box.padding_left) ||
          box.padding_top < 0.0 || box.padding_right < 0.0 ||
          box.padding_bottom < 0.0 || box.padding_left < 0.0 ||
          box.padding_left + box.padding_right >= box.width ||
          box.padding_top + box.padding_bottom >= box.height ||
          !finite(text->line_height_ratio) ||
          text->line_height_ratio < 0.5 || text->line_height_ratio > 10.0 ||
          !finite(text->letter_spacing) || text->letter_spacing < -1024.0 ||
          text->letter_spacing > 1024.0) {
        return rejected("RFX-PROJECT-114", "text content is invalid");
      }
      switch (text->font.source) {
        case FontSourceKind::system_family:
          if (!text->font.asset_id.empty() ||
              !text->font.content_digest.empty()) {
            return rejected(
                "RFX-PROJECT-141",
                "system Font references cannot claim an asset ID or digest");
          }
          break;
        case FontSourceKind::packaged_asset:
          if (!text->font.qualified() ||
              !portable_ascii_identifier(text->font.asset_id)) {
            return rejected(
                "RFX-PROJECT-142",
                "packaged Font identity requires a portable asset ID and lowercase sha256 digest");
          }
          break;
        default:
          return rejected("RFX-PROJECT-143", "Font source kind is invalid");
      }
      switch (text->direction) {
        case ParagraphDirection::left_to_right:
        case ParagraphDirection::right_to_left:
          break;
        default:
          return rejected("RFX-PROJECT-144", "text direction is invalid");
      }
      switch (text->horizontal_alignment) {
        case TextHorizontalAlignment::start:
        case TextHorizontalAlignment::center:
        case TextHorizontalAlignment::end:
        case TextHorizontalAlignment::left:
        case TextHorizontalAlignment::right:
          break;
        default:
          return rejected("RFX-PROJECT-145",
                          "text horizontal alignment is invalid");
      }
      switch (text->vertical_alignment) {
        case TextVerticalAlignment::top:
        case TextVerticalAlignment::center:
        case TextVerticalAlignment::bottom:
          break;
        default:
          return rejected("RFX-PROJECT-146",
                          "text vertical alignment is invalid");
      }
      switch (text->wrap) {
        case TextWrapMode::no_wrap:
        case TextWrapMode::word:
          break;
        default:
          return rejected("RFX-PROJECT-147", "text wrap mode is invalid");
      }
      switch (text->overflow) {
        case TextOverflowMode::clip:
        case TextOverflowMode::visible:
          break;
        default:
          return rejected("RFX-PROJECT-148", "text overflow mode is invalid");
      }
    }
    if (layer.masks.size() > 32) {
      return rejected("RFX-PROJECT-138",
                      "a Layer may contain at most 32 ordered masks");
    }
    for (const auto& mask : layer.masks) {
      if (!portable_ascii_identifier(mask.mask_id.value) ||
          !mask_ids.emplace(mask.mask_id.value).second) {
        return rejected("RFX-PROJECT-139",
                        "mask IDs must be portable and globally unique");
      }
      const auto validation = validate_visual_mask(mask);
      if (!validation.valid) return validation;
    }
    if (layer.effects.size() > 64) {
      return rejected("RFX-PROJECT-130",
                      "a Layer may contain at most 64 ordered effects");
    }
    for (const auto& effect : layer.effects) {
      if (!portable_ascii_identifier(effect.effect_id.value) ||
          !effect_ids.emplace(effect.effect_id.value).second) {
        return rejected("RFX-PROJECT-131",
                        "effect IDs must be portable and globally unique");
      }
      const auto validation = validate_visual_effect(effect);
      if (!validation.valid) return validation;
    }
  }

  std::unordered_set<std::string> group_ids;
  for (const auto& group : composition.groups) {
    if (!portable_ascii_identifier(group.group_id.value) ||
        !group_ids.emplace(group.group_id.value).second ||
        !all_visual_ids.emplace(group.group_id.value).second) {
      return rejected("RFX-PROJECT-116",
                      "group IDs must be portable and globally unique");
    }
    if (blank(group.display_name) ||
        !validate_preserved_utf8(group.display_name).valid()) {
      return rejected("RFX-PROJECT-117",
                      "group name must be valid preserved UTF-8");
    }
    if (!group.active_range.valid() ||
        group.active_range.end() > composition.duration) {
      return rejected("RFX-PROJECT-118",
                      "group range exceeds composition duration");
    }
    if (group.children.empty()) {
      return rejected("RFX-PROJECT-119", "group must contain at least one child");
    }
    if (const auto property_failure = validate_node_properties(
            group.transform, group.animations, group.active_range, true)) {
      return *property_failure;
    }
  }

  std::unordered_set<std::string> cycle_visiting;
  std::unordered_set<std::string> cycle_visited;
  std::function<bool(const LayerGroupId&)> visit_group =
      [&](const LayerGroupId& group_id) {
        if (cycle_visited.contains(group_id.value)) {
          return true;
        }
        if (!cycle_visiting.emplace(group_id.value).second) {
          return false;
        }
        const auto* group = find_layer_group(composition, group_id);
        for (const auto& child : group->children) {
          if (const auto* child_group = std::get_if<LayerGroupId>(&child);
              child_group != nullptr && find_layer_group(composition, *child_group) != nullptr &&
              !visit_group(*child_group)) {
            return false;
          }
        }
        cycle_visiting.erase(group_id.value);
        cycle_visited.emplace(group_id.value);
        return true;
      };
  for (const auto& group : composition.groups) {
    if (!visit_group(group.group_id)) {
      return rejected("RFX-PROJECT-127",
                      "visual hierarchy must be acyclic");
    }
  }

  if (!composition.groups.empty() && composition.root_nodes.empty()) {
    return rejected("RFX-PROJECT-122",
                    "hierarchical compositions require explicit root order");
  }

  const auto roots = composition_root_nodes(composition);
  std::unordered_set<std::string> referenced;
  const auto validate_reference = [&](const VisualNodeRef& node)
      -> std::optional<CompositionValidation> {
    if (const auto* layer = std::get_if<LayerId>(&node)) {
      if (!layer_ids.contains(layer->value)) {
        return rejected("RFX-PROJECT-123",
                        "visual hierarchy references an unknown layer");
      }
    } else if (!group_ids.contains(std::get<LayerGroupId>(node).value)) {
      return rejected("RFX-PROJECT-123",
                      "visual hierarchy references an unknown group");
    }
    if (!referenced.emplace(node_key(node)).second) {
      return rejected("RFX-PROJECT-124",
                      "each visual node must have exactly one parent/root position");
    }
    return std::nullopt;
  };

  for (const auto& root : roots) {
    if (const auto failure = validate_reference(root)) {
      return *failure;
    }
  }
  for (const auto& group : composition.groups) {
    for (const auto& child : group.children) {
      if (const auto failure = validate_reference(child)) {
        return *failure;
      }
      const TimeRangeNs* child_range = nullptr;
      if (const auto* layer_ref = std::get_if<LayerId>(&child)) {
        child_range = &find_layer(composition, *layer_ref)->active_range;
      } else {
        child_range = &find_layer_group(
                           composition, std::get<LayerGroupId>(child))
                           ->active_range;
      }
      if (child_range->start < group.active_range.start ||
          child_range->end() > group.active_range.end()) {
        return rejected("RFX-PROJECT-125",
                        "group range must contain every child range");
      }
    }
  }
  if (referenced.size() != composition.layers.size() + composition.groups.size()) {
    return rejected("RFX-PROJECT-126",
                    "every visual node must be reachable from the root order");
  }

  std::unordered_set<std::string> visiting;
  std::unordered_set<std::string> visited;
  std::function<bool(const VisualNodeRef&)> visit = [&](const VisualNodeRef& node) {
    const auto key = node_key(node);
    if (visited.contains(key)) {
      return true;
    }
    if (!visiting.emplace(key).second) {
      return false;
    }
    if (const auto* group_ref = std::get_if<LayerGroupId>(&node)) {
      const auto* group = find_layer_group(composition, *group_ref);
      for (const auto& child : group->children) {
        if (!visit(child)) {
          return false;
        }
      }
    }
    visiting.erase(key);
    visited.emplace(key);
    return true;
  };
  for (const auto& root : roots) {
    if (!visit(root)) {
      return rejected("RFX-PROJECT-127",
                      "visual hierarchy must be acyclic");
    }
  }
  if (visited.size() != composition.layers.size() + composition.groups.size()) {
    return rejected("RFX-PROJECT-126",
                    "every visual node must be reachable from the root order");
  }

  return CompositionValidation{.valid = true};
}

double evaluate_animated_property(const LayerSnapshot& layer,
                                  const AnimatedProperty property,
                                  const ProjectTimeNs composition_time) noexcept {
  return evaluate_property(
      layer.transform, layer.animations, property, composition_time);
}

double evaluate_animated_property(const LayerGroupSnapshot& group,
                                  const AnimatedProperty property,
                                  const ProjectTimeNs composition_time) noexcept {
  return evaluate_property(
      group.transform, group.animations, property, composition_time);
}

const LayerSnapshot* find_layer(const CompositionSnapshot& composition,
                                const LayerId& layer_id) noexcept {
  const auto found = std::find_if(
      composition.layers.begin(), composition.layers.end(),
      [&layer_id](const LayerSnapshot& layer) {
        return layer.layer_id == layer_id;
      });
  return found == composition.layers.end() ? nullptr : &*found;
}

const LayerGroupSnapshot* find_layer_group(
    const CompositionSnapshot& composition,
    const LayerGroupId& group_id) noexcept {
  const auto found = std::find_if(
      composition.groups.begin(), composition.groups.end(),
      [&group_id](const LayerGroupSnapshot& group) {
        return group.group_id == group_id;
      });
  return found == composition.groups.end() ? nullptr : &*found;
}

bool visual_node_is_ancestor(const CompositionSnapshot& composition,
                             const VisualNodeRef& ancestor,
                             const VisualNodeRef& descendant) noexcept {
  const auto* ancestor_group = std::get_if<LayerGroupId>(&ancestor);
  if (ancestor_group == nullptr || ancestor == descendant) {
    return false;
  }
  std::unordered_set<std::string> visited;
  std::function<bool(const LayerGroupId&)> contains =
      [&](const LayerGroupId& group_id) {
        if (!visited.emplace(group_id.value).second) {
          return false;
        }
        const auto* group = find_layer_group(composition, group_id);
        if (group == nullptr) {
          return false;
        }
        for (const auto& child : group->children) {
          if (child == descendant) {
            return true;
          }
          if (const auto* child_group = std::get_if<LayerGroupId>(&child);
              child_group != nullptr && contains(*child_group)) {
            return true;
          }
        }
        return false;
      };
  return contains(*ancestor_group);
}

std::vector<VisualNodeRef> composition_root_nodes(
    const CompositionSnapshot& composition) {
  if (!composition.root_nodes.empty()) {
    return composition.root_nodes;
  }
  std::vector<VisualNodeRef> roots;
  roots.reserve(composition.layers.size());
  for (const auto& layer : composition.layers) {
    roots.emplace_back(layer.layer_id);
  }
  return roots;
}

namespace {

[[nodiscard]] bool empty_rect(const LocalRect& rect) noexcept {
  return rect.right <= rect.left || rect.bottom <= rect.top;
}

[[nodiscard]] LocalRect intersect_rect(const LocalRect& lhs,
                                       const LocalRect& rhs) noexcept {
  const LocalRect result{
      .left = std::max(lhs.left, rhs.left),
      .top = std::max(lhs.top, rhs.top),
      .right = std::min(lhs.right, rhs.right),
      .bottom = std::min(lhs.bottom, rhs.bottom),
  };
  if (empty_rect(result)) {
    return {};
  }
  return result;
}

[[nodiscard]] LocalRect union_rect(const LocalRect& lhs,
                                   const LocalRect& rhs) noexcept {
  if (empty_rect(lhs)) {
    return rhs;
  }
  if (empty_rect(rhs)) {
    return lhs;
  }
  return LocalRect{
      .left = std::min(lhs.left, rhs.left),
      .top = std::min(lhs.top, rhs.top),
      .right = std::max(lhs.right, rhs.right),
      .bottom = std::max(lhs.bottom, rhs.bottom),
  };
}

[[nodiscard]] LocalRect expanded_rect(const LocalRect& rect,
                                      const double x,
                                      const double y) noexcept {
  if (empty_rect(rect)) {
    return {};
  }
  return LocalRect{
      .left = rect.left - x,
      .top = rect.top - y,
      .right = rect.right + x,
      .bottom = rect.bottom + y,
  };
}

[[nodiscard]] LocalRect translated_rect(const LocalRect& rect,
                                        const double x,
                                        const double y) noexcept {
  if (empty_rect(rect)) {
    return {};
  }
  return LocalRect{
      .left = rect.left + x,
      .top = rect.top + y,
      .right = rect.right + x,
      .bottom = rect.bottom + y,
  };
}

[[nodiscard]] LocalRect transformed_rect(const LocalRect& rect,
                                         const AffineTransform2D& matrix) {
  if (empty_rect(rect)) {
    return {};
  }
  const std::array<std::pair<double, double>, 4> corners{{
      {rect.left, rect.top},
      {rect.right, rect.top},
      {rect.right, rect.bottom},
      {rect.left, rect.bottom},
  }};
  LocalRect result{
      .left = std::numeric_limits<double>::max(),
      .top = std::numeric_limits<double>::max(),
      .right = std::numeric_limits<double>::lowest(),
      .bottom = std::numeric_limits<double>::lowest(),
  };
  for (const auto& [x, y] : corners) {
    const double world_x = matrix.m00 * x + matrix.m01 * y + matrix.m02;
    const double world_y = matrix.m10 * x + matrix.m11 * y + matrix.m12;
    result.left = std::min(result.left, world_x);
    result.top = std::min(result.top, world_y);
    result.right = std::max(result.right, world_x);
    result.bottom = std::max(result.bottom, world_y);
  }
  return result;
}

[[nodiscard]] std::vector<EvaluatedVisualNodeTransform>
evaluate_visual_node_transforms_impl(
    const CompositionSnapshot& composition,
    const ProjectTimeNs composition_time) {
  std::vector<EvaluatedVisualNodeTransform> result;
  result.reserve(composition.layers.size() + composition.groups.size());
  const AffineTransform2D identity;
  std::function<void(const VisualNodeRef&, const AffineTransform2D&, double)>
      evaluate_node;
  evaluate_node = [&](const VisualNodeRef& node,
                      const AffineTransform2D& parent_transform,
                      const double parent_opacity) {
    if (const auto* layer_ref = std::get_if<LayerId>(&node)) {
      const auto* layer = find_layer(composition, *layer_ref);
      if (!layer->active_range.contains(composition_time)) {
        return;
      }
      const auto local = evaluated_matrix(
          layer->transform, layer->animations, composition_time);
      const double opacity = evaluate_property(
          layer->transform, layer->animations, AnimatedProperty::opacity,
          composition_time);
      result.push_back(EvaluatedVisualNodeTransform{
          .node = node,
          .parent_world_transform = parent_transform,
          .world_transform = multiply(parent_transform, local),
          .effective_opacity = parent_opacity * opacity,
      });
      return;
    }

    const auto* group = find_layer_group(
        composition, std::get<LayerGroupId>(node));
    if (!group->active_range.contains(composition_time)) {
      return;
    }
    const auto local = evaluated_matrix(
        group->transform, group->animations, composition_time);
    const auto world = multiply(parent_transform, local);
    const double opacity = evaluate_property(
        group->transform, group->animations, AnimatedProperty::opacity,
        composition_time);
    const double effective_opacity = parent_opacity * opacity;
    result.push_back(EvaluatedVisualNodeTransform{
        .node = node,
        .parent_world_transform = parent_transform,
        .world_transform = world,
        .effective_opacity = effective_opacity,
    });
    for (const auto& child : group->children) {
      evaluate_node(child, world, effective_opacity);
    }
  };

  for (const auto& root : composition_root_nodes(composition)) {
    evaluate_node(root, identity, 1.0);
  }
  return result;
}

[[nodiscard]] DerivedVisualBounds derive_visual_bounds(
    const LayerSnapshot& layer,
    const AffineTransform2D& world_transform,
    const std::optional<TextLayoutResult>& text_layout) {
  LocalRect geometry;
  if (const auto* shape = std::get_if<ShapeLayerContent>(&layer.content)) {
    const double half_stroke = shape->stroke_width * 0.5;
    geometry = LocalRect{
        .left = -shape->width * 0.5 - half_stroke,
        .top = -shape->height * 0.5 - half_stroke,
        .right = shape->width * 0.5 + half_stroke,
        .bottom = shape->height * 0.5 + half_stroke,
    };
  } else if (text_layout) {
    geometry = text_layout->clipped_bounds;
  } else {
    geometry = text_box_bounds(
        std::get<TextLayerContent>(layer.content).box);
  }

  LocalRect masked = geometry;
  for (const auto& mask : layer.masks) {
    if (!mask.enabled || mask.inverted) {
      continue;
    }
    const LocalRect mask_rect{
        .left = mask.geometry.position_x - mask.geometry.width * 0.5,
        .top = mask.geometry.position_y - mask.geometry.height * 0.5,
        .right = mask.geometry.position_x + mask.geometry.width * 0.5,
        .bottom = mask.geometry.position_y + mask.geometry.height * 0.5,
    };
    masked = intersect_rect(masked, mask_rect);
  }

  LocalRect effect = masked;
  for (const auto& layer_effect : layer.effects) {
    if (!layer_effect.enabled) {
      continue;
    }
    if (const auto* blur =
            std::get_if<GaussianBlurEffect>(&layer_effect.parameters)) {
      effect = expanded_rect(effect, blur->sigma_x * 3.0,
                             blur->sigma_y * 3.0);
    } else if (const auto* shadow =
                   std::get_if<DropShadowEffect>(&layer_effect.parameters)) {
      const auto shadow_bounds = translated_rect(
          expanded_rect(effect, shadow->sigma_x * 3.0,
                        shadow->sigma_y * 3.0),
          shadow->offset_x, shadow->offset_y);
      effect = union_rect(effect, shadow_bounds);
    } else {
      const auto& glow = std::get<GlowEffect>(layer_effect.parameters);
      effect = union_rect(effect,
                          expanded_rect(effect, glow.sigma * 3.0,
                                        glow.sigma * 3.0));
    }
  }
  return DerivedVisualBounds{
      .geometry_local = geometry,
      .masked_local = masked,
      .effect_local = effect,
      .world = transformed_rect(effect, world_transform),
  };
}

[[nodiscard]] EvaluatedVisualScene evaluate_visual_scene_impl(
    const CompositionSnapshot& composition,
    const ProjectTimeNs composition_time,
    TextLayoutPort* text_layout_port) {
  const auto validation = validate_composition(composition);
  if (!validation.valid) {
    throw std::invalid_argument(validation.code + ": " + validation.message);
  }

  EvaluatedVisualScene scene{
      .transforms =
          evaluate_visual_node_transforms_impl(composition, composition_time),
  };
  scene.layers.reserve(composition.layers.size());
  for (const auto& evaluated_node : scene.transforms) {
    if (const auto* layer_ref = std::get_if<LayerId>(&evaluated_node.node)) {
      const auto* layer = find_layer(composition, *layer_ref);
      std::optional<TextLayoutResult> text_layout;
      if (text_layout_port != nullptr) {
        if (const auto* text = std::get_if<TextLayerContent>(&layer->content)) {
          auto outcome = text_layout_port->layout(TextLayoutRequest{.text = *text});
          if (!outcome.succeeded()) {
            const auto diagnostic = outcome.diagnostic.value_or(
                TextLayoutDiagnostic{
                    .code = "RFX-TEXT-LAYOUT-UNKNOWN",
                    .message = "Text layout failed without a diagnostic",
                });
            throw std::runtime_error(diagnostic.code + ": " +
                                     diagnostic.message);
          }
          text_layout = std::move(*outcome.result);
        }
      }
      const auto bounds = derive_visual_bounds(
          *layer, evaluated_node.world_transform, text_layout);
      scene.layers.push_back(EvaluatedVisualLayer{
          .layer_id = layer->layer_id,
          .display_name = layer->display_name,
          .content = layer->content,
          .masks = layer->masks,
          .effects = layer->effects,
          .blend_mode = layer->blend_mode,
          .world_transform = evaluated_node.world_transform,
          .effective_opacity = evaluated_node.effective_opacity,
          .text_layout = std::move(text_layout),
          .bounds = bounds,
      });
    }
  }
  return scene;
}

}  // namespace

std::vector<EvaluatedVisualNodeTransform> evaluate_visual_node_transforms(
    const CompositionSnapshot& composition,
    const ProjectTimeNs composition_time) {
  const auto validation = validate_composition(composition);
  if (!validation.valid) {
    throw std::invalid_argument(validation.code + ": " + validation.message);
  }
  return evaluate_visual_node_transforms_impl(composition, composition_time);
}

LocalRect transform_local_rect(const LocalRect& rect,
                               const AffineTransform2D& transform) noexcept {
  return transformed_rect(rect, transform);
}

EvaluatedVisualScene evaluate_visual_scene(
    const CompositionSnapshot& composition,
    const ProjectTimeNs composition_time) {
  return evaluate_visual_scene_impl(composition, composition_time, nullptr);
}

EvaluatedVisualScene evaluate_visual_scene(
    const CompositionSnapshot& composition,
    const ProjectTimeNs composition_time,
    TextLayoutPort& text_layout_port) {
  return evaluate_visual_scene_impl(composition, composition_time,
                                    &text_layout_port);
}

std::vector<EvaluatedVisualLayer> evaluate_visual_layers(
    const CompositionSnapshot& composition,
    const ProjectTimeNs composition_time) {
  return evaluate_visual_scene(composition, composition_time).layers;
}

std::vector<EvaluatedVisualLayer> evaluate_visual_layers(
    const CompositionSnapshot& composition,
    const ProjectTimeNs composition_time,
    TextLayoutPort& text_layout_port) {
  return evaluate_visual_scene(composition, composition_time,
                               text_layout_port)
      .layers;
}

}  // namespace refusion::core
