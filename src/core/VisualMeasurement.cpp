#include "refusion/core/VisualMeasurement.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace refusion::core {
namespace {

[[nodiscard]] std::string node_key(const VisualNodeRef& node) {
  if (const auto* layer = std::get_if<LayerId>(&node)) {
    return "layer:" + layer->value;
  }
  return "group:" + std::get<LayerGroupId>(node).value;
}

[[nodiscard]] bool empty(const LocalRect& rect) noexcept {
  return rect.right <= rect.left || rect.bottom <= rect.top;
}

[[nodiscard]] bool valid_bounds(const LocalRect& rect) noexcept {
  return std::isfinite(rect.left) && std::isfinite(rect.top) &&
         std::isfinite(rect.right) && std::isfinite(rect.bottom) &&
         rect.left <= rect.right && rect.top <= rect.bottom;
}

[[nodiscard]] LocalRect united(const LocalRect& lhs,
                               const LocalRect& rhs) noexcept {
  if (empty(lhs)) {
    return rhs;
  }
  if (empty(rhs)) {
    return lhs;
  }
  return LocalRect{
      .left = std::min(lhs.left, rhs.left),
      .top = std::min(lhs.top, rhs.top),
      .right = std::max(lhs.right, rhs.right),
      .bottom = std::max(lhs.bottom, rhs.bottom),
  };
}

[[nodiscard]] VisualMeasurementOutcome failed(std::string code,
                                              std::string message) {
  return VisualMeasurementOutcome{
      .diagnostic = VisualMeasurementDiagnostic{
          .code = std::move(code),
          .message = std::move(message),
      },
  };
}

[[nodiscard]] const EvaluatedVisualNodeTransform* find_transform(
    const std::vector<EvaluatedVisualNodeTransform>& transforms,
    const VisualNodeRef& node) {
  const auto found = std::find_if(
      transforms.begin(), transforms.end(), [&node](const auto& candidate) {
        return candidate.node == node;
      });
  return found == transforms.end() ? nullptr : &*found;
}

}  // namespace

VisualMeasurementOutcome measure_visual_nodes(
    const CompositionSnapshot& composition,
    const ProjectTimeNs composition_time,
    TextLayoutPort* text_layout_port) {
  if (composition_time >= composition.duration) {
    return failed("RFX-MEASURE-TIME-001",
                  "measurement time is outside the Composition half-open range");
  }

  EvaluatedVisualScene scene;
  try {
    scene = text_layout_port == nullptr
                ? evaluate_visual_scene(composition, composition_time)
                : evaluate_visual_scene(composition, composition_time,
                                        *text_layout_port);
  } catch (const std::exception& error) {
    const std::string message{error.what()};
    const auto separator = message.find(": ");
    return failed(separator == std::string::npos
                      ? "RFX-MEASURE-EVALUATION-001"
                      : message.substr(0, separator),
                  separator == std::string::npos
                      ? message
                      : message.substr(separator + 2));
  }

  std::unordered_map<std::string, VisualNodeMeasurement> measured;
  for (const auto& layer : scene.layers) {
    const VisualNodeRef node{layer.layer_id};
    const auto* transform = find_transform(scene.transforms, node);
    if (transform == nullptr) {
      return failed("RFX-MEASURE-TRANSFORM-001",
                    "evaluated Layer has no matching transform record");
    }

    LocalRect geometry_local = layer.bounds.geometry_local;
    std::optional<LocalRect> logical_local;
    std::optional<LocalRect> ink_local;
    if (const auto* text = std::get_if<TextLayerContent>(&layer.content)) {
      // Geometry is authored project truth. A layout adapter may only supply
      // derived logical/ink metrics; it cannot redefine the TextBox.
      geometry_local = text_box_bounds(text->box);
      if (layer.text_layout) {
        logical_local = layer.text_layout->logical_bounds;
        ink_local = layer.text_layout->ink_bounds;
      }
    } else {
      logical_local = geometry_local;
      ink_local = geometry_local;
    }

    if (!valid_bounds(geometry_local) ||
        (logical_local && !valid_bounds(*logical_local)) ||
        (ink_local && !valid_bounds(*ink_local))) {
      return failed("RFX-MEASURE-BOUNDS-001",
                    "layout adapter returned non-finite or inverted bounds");
    }

    measured.emplace(
        node_key(node),
        VisualNodeMeasurement{
            .node = node,
            .geometry_world = transform_local_rect(
                geometry_local, transform->world_transform),
            .logical_world =
                logical_local
                    ? std::optional<LocalRect>{transform_local_rect(
                          *logical_local, transform->world_transform)}
                    : std::nullopt,
            .ink_world =
                ink_local
                    ? std::optional<LocalRect>{transform_local_rect(
                          *ink_local, transform->world_transform)}
                    : std::nullopt,
            .parent_world_transform = transform->parent_world_transform,
            .world_transform = transform->world_transform,
        });
  }

  std::function<std::optional<VisualNodeMeasurement>(const VisualNodeRef&)>
      aggregate;
  aggregate = [&](const VisualNodeRef& node)
      -> std::optional<VisualNodeMeasurement> {
    if (const auto found = measured.find(node_key(node));
        found != measured.end()) {
      return found->second;
    }
    const auto* group_id = std::get_if<LayerGroupId>(&node);
    if (group_id == nullptr) {
      return std::nullopt;
    }
    const auto* group = find_layer_group(composition, *group_id);
    const auto* transform = find_transform(scene.transforms, node);
    if (group == nullptr || transform == nullptr) {
      return std::nullopt;
    }

    bool have_child = false;
    bool logical_available = true;
    bool ink_available = true;
    LocalRect geometry;
    LocalRect logical;
    LocalRect ink;
    for (const auto& child : group->children) {
      const auto child_measurement = aggregate(child);
      if (!child_measurement) {
        continue;
      }
      geometry = have_child
                     ? united(geometry, child_measurement->geometry_world)
                     : child_measurement->geometry_world;
      if (child_measurement->logical_world) {
        logical = have_child && logical_available
                      ? united(logical, *child_measurement->logical_world)
                      : *child_measurement->logical_world;
      } else {
        logical_available = false;
      }
      if (child_measurement->ink_world) {
        ink = have_child && ink_available
                  ? united(ink, *child_measurement->ink_world)
                  : *child_measurement->ink_world;
      } else {
        ink_available = false;
      }
      have_child = true;
    }
    if (!have_child) {
      return std::nullopt;
    }
    VisualNodeMeasurement result{
        .node = node,
        .geometry_world = geometry,
        .logical_world = logical_available
                             ? std::optional<LocalRect>{logical}
                             : std::nullopt,
        .ink_world = ink_available ? std::optional<LocalRect>{ink}
                                    : std::nullopt,
        .parent_world_transform = transform->parent_world_transform,
        .world_transform = transform->world_transform,
    };
    measured.emplace(node_key(node), result);
    return result;
  };

  for (const auto& transform : scene.transforms) {
    static_cast<void>(aggregate(transform.node));
  }

  VisualMeasurementSnapshot snapshot{
      .composition_time = composition_time,
      .layout_engine_digest = text_layout_port == nullptr
                                  ? std::string{}
                                  : text_layout_port->layout_engine_digest(),
  };
  snapshot.nodes.reserve(scene.transforms.size());
  for (const auto& transform : scene.transforms) {
    if (const auto found = measured.find(node_key(transform.node));
        found != measured.end()) {
      snapshot.nodes.push_back(found->second);
    }
  }
  return VisualMeasurementOutcome{.snapshot = std::move(snapshot)};
}

const VisualNodeMeasurement* find_visual_measurement(
    const VisualMeasurementSnapshot& snapshot,
    const VisualNodeRef& node) noexcept {
  const auto found = std::find_if(
      snapshot.nodes.begin(), snapshot.nodes.end(),
      [&node](const VisualNodeMeasurement& candidate) {
        return candidate.node == node;
      });
  return found == snapshot.nodes.end() ? nullptr : &*found;
}

std::optional<LocalRect> measurement_bounds(
    const VisualNodeMeasurement& measurement,
    const AlignmentBoundsBasis basis) noexcept {
  switch (basis) {
    case AlignmentBoundsBasis::geometry:
      return measurement.geometry_world;
    case AlignmentBoundsBasis::logical:
      return measurement.logical_world;
    case AlignmentBoundsBasis::ink:
      return measurement.ink_world;
  }
  return std::nullopt;
}

}  // namespace refusion::core
