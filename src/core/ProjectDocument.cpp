#include "refusion/core/ProjectDocument.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
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

CompositionValidation validate_composition(
    const CompositionSnapshot& composition) {
  if (blank(composition.composition_id.value)) {
    return rejected("RFX-PROJECT-101", "composition ID is required");
  }
  if (blank(composition.display_name)) {
    return rejected("RFX-PROJECT-102", "composition name is required");
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
  if (composition.layers.empty()) {
    return rejected("RFX-PROJECT-106", "composition must contain at least one layer");
  }

  std::unordered_set<std::string> layer_ids;
  for (const auto& layer : composition.layers) {
    if (blank(layer.layer_id.value) || !layer_ids.emplace(layer.layer_id.value).second) {
      return rejected("RFX-PROJECT-107", "layer IDs must be non-empty and unique");
    }
    if (blank(layer.display_name)) {
      return rejected("RFX-PROJECT-108", "layer name is required");
    }
    if (!layer.active_range.valid() ||
        layer.active_range.end() > composition.duration) {
      return rejected("RFX-PROJECT-109", "layer range exceeds composition duration");
    }
    const auto& transform = layer.transform;
    if (!finite(transform.position_x) || !finite(transform.position_y) ||
        !finite(transform.scale_x) || !finite(transform.scale_y) ||
        !finite(transform.rotation_degrees) || !finite(transform.opacity) ||
        transform.scale_x <= 0.0 || transform.scale_y <= 0.0 ||
        transform.opacity < 0.0 || transform.opacity > 1.0) {
      return rejected("RFX-PROJECT-110", "layer transform contains invalid values");
    }

    std::unordered_set<std::uint8_t> animated_properties;
    for (const auto& animation : layer.animations) {
      const auto key = static_cast<std::uint8_t>(animation.property);
      if (!animated_properties.emplace(key).second ||
          animation.keyframes.empty()) {
        return rejected("RFX-PROJECT-111",
                        "animated properties must be unique and have keyframes");
      }
      ProjectTimeNs previous_time = 0;
      bool first = true;
      for (const auto& keyframe : animation.keyframes) {
        if (!finite(keyframe.value) || keyframe.time < layer.active_range.start ||
            keyframe.time > layer.active_range.end() ||
            (!first && keyframe.time <= previous_time)) {
          return rejected("RFX-PROJECT-112",
                          "keyframes must be finite, ordered and inside the layer range");
        }
        previous_time = keyframe.time;
        first = false;
      }
    }

    if (const auto* shape = std::get_if<ShapeLayerContent>(&layer.content)) {
      if (!finite(shape->width) || !finite(shape->height) ||
          !finite(shape->corner_radius) || shape->width <= 0.0 ||
          shape->height <= 0.0 || shape->corner_radius < 0.0) {
        return rejected("RFX-PROJECT-113", "shape geometry is invalid");
      }
    } else if (const auto* text = std::get_if<TextLayerContent>(&layer.content)) {
      if (blank(text->text) || blank(text->font_family) ||
          !finite(text->font_size) || !finite(text->layout_width) ||
          text->font_size <= 0.0 || text->layout_width <= 0.0) {
        return rejected("RFX-PROJECT-114", "text content is invalid");
      }
    }
  }

  return CompositionValidation{.valid = true};
}

double evaluate_animated_property(const LayerSnapshot& layer,
                                  const AnimatedProperty property,
                                  const ProjectTimeNs composition_time) noexcept {
  const auto default_value = default_property_value(layer.transform, property);
  const auto animation = std::find_if(
      layer.animations.begin(), layer.animations.end(),
      [property](const ScalarAnimation& candidate) {
        return candidate.property == property;
      });
  if (animation == layer.animations.end() || animation->keyframes.empty()) {
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

}  // namespace refusion::core
