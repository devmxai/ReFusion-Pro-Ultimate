#include "AgentJson.hpp"

#include "refusion/core/CanonicalText.hpp"
#include "refusion/core/AgentIntrospection.hpp"
#include "refusion/core/ProjectAuthority.hpp"
#include "refusion/core/ProjectClock.hpp"
#include "refusion/core/SemanticAuthoring.hpp"
#include "refusion/core/VisualMeasurement.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"
#include "refusion/core/VisualPropertyRegistry.hpp"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <variant>

namespace refusion::cli {
namespace {

using namespace refusion::core;

void quoted(std::ostream& output, const std::string_view value) {
  const auto utf8 = validate_preserved_utf8(value);
  if (!utf8.valid()) {
    throw std::invalid_argument("Agent JSON string violates the UTF-8 contract");
  }
  output << '"';
  for (const unsigned char character : value) {
    switch (character) {
      case '"': output << "\\\""; break;
      case '\\': output << "\\\\"; break;
      case '\b': output << "\\b"; break;
      case '\f': output << "\\f"; break;
      case '\n': output << "\\n"; break;
      case '\r': output << "\\r"; break;
      case '\t': output << "\\t"; break;
      default:
        if (character < 0x20U) {
          output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                 << static_cast<unsigned int>(character) << std::dec;
        } else {
          output << static_cast<char>(character);
        }
        break;
    }
  }
  output << '"';
}

void number(std::ostream& output, const double value) {
  output << canonical_float64(value);
}

[[nodiscard]] std::string node_ref(const VisualNodeRef& node) {
  if (const auto* layer = std::get_if<LayerId>(&node)) {
    return "layer:" + layer->value;
  }
  return "group:" + std::get<LayerGroupId>(node).value;
}

[[nodiscard]] std::string_view node_kind(const AgentVisualKind kind) {
  switch (kind) {
    case AgentVisualKind::shape: return "shape";
    case AgentVisualKind::text: return "text";
    case AgentVisualKind::group: return "group";
  }
  return "unknown";
}

[[nodiscard]] std::string_view animated_property(
    const AnimatedProperty property) {
  switch (property) {
    case AnimatedProperty::position_x: return "transform.position.x";
    case AnimatedProperty::position_y: return "transform.position.y";
    case AnimatedProperty::scale_x: return "transform.scale.x";
    case AnimatedProperty::scale_y: return "transform.scale.y";
    case AnimatedProperty::rotation_degrees: return "transform.rotation";
    case AnimatedProperty::opacity: return "transform.opacity";
  }
  return "unknown";
}

[[nodiscard]] std::string_view value_kind(
    const VisualPropertyValueKind kind) {
  switch (kind) {
    case VisualPropertyValueKind::number: return "number";
    case VisualPropertyValueKind::color_rgba8: return "color_rgba8";
    case VisualPropertyValueKind::text: return "text";
    case VisualPropertyValueKind::boolean: return "boolean";
    case VisualPropertyValueKind::shape_fill: return "shape_fill";
  }
  return "unknown";
}

void color(std::ostream& output, const ColorRgba8& value) {
  std::ostringstream encoded;
  encoded.imbue(std::locale::classic());
  encoded << '#' << std::uppercase << std::hex << std::setfill('0')
          << std::setw(2) << static_cast<unsigned int>(value.red)
          << std::setw(2) << static_cast<unsigned int>(value.green)
          << std::setw(2) << static_cast<unsigned int>(value.blue)
          << std::setw(2) << static_cast<unsigned int>(value.alpha);
  quoted(output, encoded.str());
}

void gradient_stops(std::ostream& output,
                    const std::vector<GradientStop>& stops) {
  output << '[';
  for (std::size_t index = 0; index < stops.size(); ++index) {
    if (index != 0) output << ',';
    output << "{\"offset\":";
    number(output, stops[index].offset);
    output << ",\"color\":";
    color(output, stops[index].color);
    output << '}';
  }
  output << ']';
}

void shape_fill(std::ostream& output, const ShapeFill& fill) {
  std::visit(
      [&output](const auto& value) {
        using Value = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<Value, ColorRgba8>) {
          output << "{\"kind\":\"solid\",\"color\":";
          color(output, value);
        } else if constexpr (std::is_same_v<Value, LinearGradientFill>) {
          output << "{\"kind\":\"linear_gradient\",\"start_local_px\":[";
          number(output, value.start_x);
          output << ',';
          number(output, value.start_y);
          output << "],\"end_local_px\":[";
          number(output, value.end_x);
          output << ',';
          number(output, value.end_y);
          output << "],\"stops\":";
          gradient_stops(output, value.stops);
        } else {
          output << "{\"kind\":\"radial_gradient\",\"center_local_px\":[";
          number(output, value.center_x);
          output << ',';
          number(output, value.center_y);
          output << "],\"radius_px\":";
          number(output, value.radius);
          output << ",\"stops\":";
          gradient_stops(output, value.stops);
        }
        output << '}';
      },
      fill);
}

void property_value(std::ostream& output, const VisualPropertyValue& value) {
  std::visit(
      [&output](const auto& typed) {
        using Value = std::decay_t<decltype(typed)>;
        if constexpr (std::is_same_v<Value, double>) {
          number(output, typed);
        } else if constexpr (std::is_same_v<Value, ColorRgba8>) {
          color(output, typed);
        } else if constexpr (std::is_same_v<Value, std::string>) {
          quoted(output, typed);
        } else if constexpr (std::is_same_v<Value, bool>) {
          output << (typed ? "true" : "false");
        } else {
          shape_fill(output, typed);
        }
      },
      value);
}

void rect(std::ostream& output, const LocalRect& value) {
  output << "{\"left\":";
  number(output, value.left);
  output << ",\"top\":";
  number(output, value.top);
  output << ",\"right\":";
  number(output, value.right);
  output << ",\"bottom\":";
  number(output, value.bottom);
  output << ",\"width\":";
  number(output, value.right - value.left);
  output << ",\"height\":";
  number(output, value.bottom - value.top);
  output << '}';
}

void optional_rect(std::ostream& output,
                   const std::optional<LocalRect>& value) {
  if (value) rect(output, *value);
  else output << "null";
}

void node_array(std::ostream& output,
                const std::vector<VisualNodeRef>& nodes) {
  output << '[';
  for (std::size_t index = 0; index < nodes.size(); ++index) {
    if (index != 0) output << ',';
    quoted(output, node_ref(nodes[index]));
  }
  output << ']';
}

void transform(std::ostream& output, const Transform2D& value) {
  output << "{\"position_parent_px\":[";
  number(output, value.position_x);
  output << ',';
  number(output, value.position_y);
  output << "],\"anchor_local_px\":[";
  number(output, value.anchor_x);
  output << ',';
  number(output, value.anchor_y);
  output << "],\"scale_ratio\":[";
  number(output, value.scale_x);
  output << ',';
  number(output, value.scale_y);
  output << "],\"rotation_degrees\":";
  number(output, value.rotation_degrees);
  output << ",\"opacity_ratio\":";
  number(output, value.opacity);
  output << '}';
}

void time_range(std::ostream& output, const TimeRangeNs& range,
                const ProjectClockSpec& clock) {
  output << "{\"start_ns\":" << range.start << ",\"end_ns\":"
         << range.end() << ",\"duration_ns\":" << range.duration
         << ",\"start_frame\":" << clock.frame_at_time(range.start)
         << ",\"end_frame\":" << clock.frame_at_time(range.end()) << '}';
}

void semantic_address(std::ostream& output, const AgentVisualNode& node,
                      const ProjectClockSpec& clock) {
  output << "\"ref\":";
  quoted(output, node_ref(node.node));
  output << ",\"kind\":";
  quoted(output, node_kind(node.kind));
  output << ",\"name\":";
  quoted(output, node.display_name);
  output << ",\"parent_path\":[";
  for (std::size_t index = 0; index < node.parent_path.size(); ++index) {
    if (index != 0) output << ',';
    quoted(output, "group:" + node.parent_path[index].value);
  }
  output << "],\"sibling_index\":" << node.sibling_index
         << ",\"timeline_row\":" << node.timeline_row << ",\"range\":";
  time_range(output, node.active_range, clock);
}

[[nodiscard]] const EvaluatedVisualLayer* evaluated_layer(
    const EvaluatedVisualScene& scene, const VisualNodeRef& node) {
  const auto* layer_id = std::get_if<LayerId>(&node);
  if (layer_id == nullptr) return nullptr;
  const auto found = std::find_if(
      scene.layers.begin(), scene.layers.end(),
      [layer_id](const EvaluatedVisualLayer& layer) {
        return layer.layer_id == *layer_id;
      });
  return found == scene.layers.end() ? nullptr : &*found;
}

}  // namespace

void write_error_json(std::ostream& output, const std::string_view schema,
                      const std::string_view code,
                      const std::string_view message) {
  output.imbue(std::locale::classic());
  output << "{\"schema\":";
  quoted(output, schema);
  output << ",\"ok\":false,\"diagnostic\":{\"code\":";
  quoted(output, code);
  output << ",\"message\":";
  quoted(output, message);
  output << "}}\n";
}

void write_validate_json(const ProjectSnapshot& project,
                         std::ostream& output) {
  output.imbue(std::locale::classic());
  const auto outline = agent_project_outline(project);
  output << "{\"schema\":\"refusion.agent.validate.v1\",\"ok\":true,"
            "\"project_id\":";
  quoted(output, project.project_id.value);
  output << ",\"revision\":" << project.revision_id.value
         << ",\"snapshot_digest\":";
  quoted(output, outline.snapshot_digest);
  output << ",\"registry_digest\":";
  quoted(output, outline.registry_digest);
  output << ",\"contribution_registry_digest\":";
  quoted(output, outline.contribution_registry_digest);
  output << ",\"layers\":" << project.composition->layers.size()
         << ",\"groups\":" << project.composition->groups.size()
         << ",\"roots\":" << outline.roots.size() << "}\n";
}

void write_outline_json(const ProjectSnapshot& project,
                        std::ostream& output) {
  output.imbue(std::locale::classic());
  const auto outline = agent_project_outline(project);
  const ProjectClockSpec clock{.duration_ns = outline.duration,
                               .frame_rate = outline.frame_rate,
                               .loop = false};
  output << "{\"schema\":\"refusion.agent.outline.v1\",\"project\":{"
            "\"id\":";
  quoted(output, outline.project_id.value);
  output << ",\"revision\":" << outline.revision_id.value
         << ",\"snapshot_digest\":";
  quoted(output, outline.snapshot_digest);
  output << ",\"registry_digest\":";
  quoted(output, outline.registry_digest);
  output << ",\"contribution_registry_digest\":";
  quoted(output, outline.contribution_registry_digest);
  output << "},\"composition\":{\"id\":";
  quoted(output, outline.composition_id.value);
  output << ",\"canvas_px\":[" << outline.canvas.width_pixels << ','
         << outline.canvas.height_pixels << "],\"frame_rate\":["
         << outline.frame_rate.numerator << ',' << outline.frame_rate.denominator
         << "],\"duration_ns\":" << outline.duration
         << ",\"duration_frames\":" << clock.frame_at_time(outline.duration)
         << "},\"roots\":";
  node_array(output, outline.roots);
  output << ",\"nodes\":[";
  for (std::size_t index = 0; index < outline.nodes.size(); ++index) {
    if (index != 0) output << ',';
    const auto& node = outline.nodes[index];
    output << '{';
    semantic_address(output, node, clock);
    output << ",\"transform\":";
    transform(output, node.transform);
    output << ",\"ownership\":{\"masks\":[";
    for (std::size_t mask = 0; mask < node.owned_masks.size(); ++mask) {
      if (mask != 0) output << ',';
      quoted(output, node.owned_masks[mask].value);
    }
    output << "],\"effects\":[";
    for (std::size_t effect = 0; effect < node.owned_effects.size(); ++effect) {
      if (effect != 0) output << ',';
      quoted(output, node.owned_effects[effect].value);
    }
    output << "],\"animated_properties\":[";
    for (std::size_t animation = 0;
         animation < node.animated_properties.size(); ++animation) {
      if (animation != 0) output << ',';
      quoted(output, animated_property(node.animated_properties[animation]));
    }
    output << "]}}";
  }
  output << "]}\n";
}

bool write_inspect_json(const ProjectSnapshot& project,
                        const VisualNodeRef& requested_node,
                        std::ostream& output,
                        std::string& error) {
  output.imbue(std::locale::classic());
  const auto outline = agent_project_outline(project);
  const auto* node = find_agent_visual_node(outline, requested_node);
  if (node == nullptr) {
    error = "RFX-AGENT-NODE-001: visual node does not exist";
    return false;
  }
  const ProjectClockSpec clock{.duration_ns = outline.duration,
                               .frame_rate = outline.frame_rate,
                               .loop = false};
  output << "{\"schema\":\"refusion.agent.inspect.v1\",\"project_id\":";
  quoted(output, outline.project_id.value);
  output << ",\"revision\":" << outline.revision_id.value
         << ",\"snapshot_digest\":";
  quoted(output, outline.snapshot_digest);
  output << ",\"registry_digest\":";
  quoted(output, outline.registry_digest);
  output << ",\"contribution_registry_digest\":";
  quoted(output, outline.contribution_registry_digest);
  output << ",\"node\":{";
  semantic_address(output, *node, clock);
  output << ",\"transform\":";
  transform(output, node->transform);
  output << ",\"properties\":[";
  for (std::size_t index = 0; index < node->properties.size(); ++index) {
    if (index != 0) output << ',';
    const auto& property = node->properties[index];
    output << "{\"id\":";
    quoted(output, property.descriptor.id.value);
    output << ",\"display_name\":";
    quoted(output, property.descriptor.display_name);
    output << ",\"value_kind\":";
    quoted(output, value_kind(property.descriptor.value_kind));
    output << ",\"unit\":";
    quoted(output, property.descriptor.unit);
    output << ",\"writable\":"
           << (property.descriptor.writable ? "true" : "false")
           << ",\"animatable\":"
           << (property.descriptor.animatable ? "true" : "false")
           << ",\"value\":";
    property_value(output, property.value);
    output << '}';
  }
  output << "]}}\n";
  return true;
}

void write_capabilities_json(std::ostream& output) {
  output.imbue(std::locale::classic());
  const auto capabilities = authoring_capabilities();
  output << "{\"schema\":\"refusion.agent.capabilities.v1\","
            "\"registry_digest\":";
  quoted(output, visual_property_registry_digest());
  output << ",\"contribution_registry_digest\":";
  quoted(output, visual_contribution_registry_digest());
  output << ",\"contributions\":[";
  const auto& contributions = visual_contribution_descriptors();
  for (std::size_t index = 0; index < contributions.size(); ++index) {
    if (index != 0) output << ',';
    output << "{\"descriptor_id\":";
    quoted(output, contributions[index].id);
    output << ",\"capability_id\":";
    quoted(output, contributions[index].capability_id);
    output << '}';
  }
  output << "],\"capabilities\":[";
  for (std::size_t index = 0; index < capabilities.size(); ++index) {
    if (index != 0) output << ',';
    const auto& capability = capabilities[index];
    output << "{\"id\":";
    quoted(output, capability.capability_id);
    output << ",\"supported\":"
           << (capability.supported ? "true" : "false")
           << ",\"unavailable_code\":";
    if (capability.unavailable_code.empty()) output << "null";
    else quoted(output, capability.unavailable_code);
    output << '}';
  }
  output << "]}\n";
}

void write_lint_json(const ProjectSnapshot& project, std::ostream& output) {
  output.imbue(std::locale::classic());
  const auto outline = agent_project_outline(project);
  const auto issues = semantic_authoring_lint(*project.composition);
  output << "{\"schema\":\"refusion.agent.lint.v1\",\"project_id\":";
  quoted(output, project.project_id.value);
  output << ",\"revision\":" << project.revision_id.value
         << ",\"snapshot_digest\":";
  quoted(output, outline.snapshot_digest);
  output << ",\"issues\":[";
  for (std::size_t index = 0; index < issues.size(); ++index) {
    if (index != 0) output << ',';
    output << "{\"severity\":\"warning\",\"code\":";
    quoted(output, issues[index].code);
    output << ",\"message\":";
    quoted(output, issues[index].message);
    output << ",\"nodes\":";
    node_array(output, issues[index].nodes);
    output << '}';
  }
  output << "]}\n";
}

void write_diff_json(const ProjectSnapshot& before,
                     const ProjectSnapshot& after,
                     std::ostream& output) {
  output.imbue(std::locale::classic());
  const auto diff = agent_project_diff(before, after);
  output << "{\"schema\":\"refusion.agent.diff.v1\",\"same_project_id\":"
         << (diff.same_project_id ? "true" : "false")
         << ",\"next_revision\":"
         << (diff.next_revision ? "true" : "false")
         << ",\"project_metadata_changed\":"
         << (diff.project_metadata_changed ? "true" : "false")
         << ",\"composition_metadata_changed\":"
         << (diff.composition_metadata_changed ? "true" : "false")
         << ",\"topology_changed\":"
         << (diff.topology_changed ? "true" : "false")
         << ",\"before_digest\":";
  quoted(output, diff.before_digest);
  output << ",\"after_digest\":";
  quoted(output, diff.after_digest);
  output << ",\"added_nodes\":";
  node_array(output, diff.added_nodes);
  output << ",\"removed_nodes\":";
  node_array(output, diff.removed_nodes);
  output << ",\"changed_nodes\":";
  node_array(output, diff.changed_nodes);
  output << "}\n";
}

bool write_measure_json(const ProjectSnapshot& project,
                        const ProjectTimeNs composition_time,
                        TextLayoutPort* text_layout,
                        std::ostream& output,
                        std::string& error) {
  output.imbue(std::locale::classic());
  const auto& composition = *project.composition;
  if (composition_time >= composition.duration) {
    error = "RFX-MEASURE-TIME-001: project time is outside the Composition "
            "half-open range";
    return false;
  }
  const auto measured =
      measure_visual_nodes(composition, composition_time, text_layout);
  if (!measured.succeeded()) {
    error = measured.diagnostic->code + ": " + measured.diagnostic->message;
    return false;
  }
  const auto scene = text_layout == nullptr
                         ? evaluate_visual_scene(composition, composition_time)
                         : evaluate_visual_scene(composition, composition_time,
                                                 *text_layout);
  const auto outline = agent_project_outline(project);
  const ProjectClockSpec clock{.duration_ns = composition.duration,
                               .frame_rate = composition.frame_rate,
                               .loop = false};
  output << "{\"schema\":\"refusion.agent.measure.v1\",\"project_id\":";
  quoted(output, project.project_id.value);
  output << ",\"revision\":" << project.revision_id.value
         << ",\"snapshot_digest\":";
  quoted(output, outline.snapshot_digest);
  output << ",\"time_ns\":" << composition_time << ",\"frame\":"
         << clock.frame_at_time(composition_time) << ",\"layout_engine_digest\":";
  quoted(output, measured.snapshot->layout_engine_digest);
  output << ",\"nodes\":[";
  for (std::size_t index = 0; index < measured.snapshot->nodes.size(); ++index) {
    if (index != 0) output << ',';
    const auto& measurement = measured.snapshot->nodes[index];
    const auto* address = find_agent_visual_node(outline, measurement.node);
    output << '{';
    semantic_address(output, *address, clock);
    output << ",\"geometry_world_px\":";
    rect(output, measurement.geometry_world);
    output << ",\"logical_world_px\":";
    optional_rect(output, measurement.logical_world);
    output << ",\"ink_world_px\":";
    optional_rect(output, measurement.ink_world);
    const auto* layer = evaluated_layer(scene, measurement.node);
    if (layer != nullptr) {
      output << ",\"geometry_local_px\":";
      rect(output, layer->bounds.geometry_local);
      output << ",\"masked_local_px\":";
      rect(output, layer->bounds.masked_local);
      output << ",\"effect_local_px\":";
      rect(output, layer->bounds.effect_local);
      output << ",\"effect_world_px\":";
      rect(output, layer->bounds.world);
      output << ",\"resolved_font_digest\":";
      if (layer->text_layout) quoted(output, layer->text_layout->resolved_font_digest);
      else output << "null";
    }
    output << '}';
  }
  output << "]}\n";
  return true;
}

}  // namespace refusion::cli
