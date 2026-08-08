#include "refusion/core/AgentIntrospection.hpp"

#include "refusion/core/CanonicalText.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string_view>
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

[[nodiscard]] const LayerSnapshot* layer_for(
    const CompositionSnapshot& composition,
    const VisualNodeRef& node) noexcept {
  const auto* layer = std::get_if<LayerId>(&node);
  return layer == nullptr ? nullptr : find_layer(composition, *layer);
}

[[nodiscard]] const LayerGroupSnapshot* group_for(
    const CompositionSnapshot& composition,
    const VisualNodeRef& node) noexcept {
  const auto* group = std::get_if<LayerGroupId>(&node);
  return group == nullptr ? nullptr : find_layer_group(composition, *group);
}

[[nodiscard]] bool same_topology(const CompositionSnapshot& lhs,
                                 const CompositionSnapshot& rhs) {
  if (composition_root_nodes(lhs) != composition_root_nodes(rhs) ||
      lhs.layers.size() != rhs.layers.size() ||
      lhs.groups.size() != rhs.groups.size()) {
    return false;
  }
  for (const auto& layer : lhs.layers) {
    if (find_layer(rhs, layer.layer_id) == nullptr) {
      return false;
    }
  }
  for (const auto& group : lhs.groups) {
    const auto* other = find_layer_group(rhs, group.group_id);
    if (other == nullptr || other->children != group.children) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::string project_snapshot_digest(const ProjectSnapshot& project) {
  const auto canonical = serialize_project_rfx(project);
  std::uint64_t hash = 14695981039346656037ULL;
  for (const unsigned char byte : canonical) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  return "rfx-project-fnv1a64:" + canonical_hex64(hash);
}

AgentProjectOutline agent_project_outline(const ProjectSnapshot& project) {
  if (!project.composition) {
    throw std::invalid_argument("RFX-SCHEMA-002: project composition is required");
  }
  const auto validation = validate_composition(*project.composition);
  if (!validation.valid) {
    throw std::invalid_argument(validation.code + ": " + validation.message);
  }
  const auto& composition = *project.composition;
  AgentProjectOutline outline{
      .project_id = project.project_id,
      .revision_id = project.revision_id,
      .composition_id = composition.composition_id,
      .canvas = composition.canvas,
      .frame_rate = composition.frame_rate,
      .duration = composition.duration,
      .registry_digest = visual_property_registry_digest(),
      .contribution_registry_digest =
          visual_contribution_registry_digest(),
      .snapshot_digest = project_snapshot_digest(project),
      .roots = composition_root_nodes(composition),
  };
  outline.nodes.reserve(composition.layers.size() + composition.groups.size());

  std::function<void(const VisualNodeRef&, const std::vector<LayerGroupId>&,
                     std::size_t, std::size_t)>
      visit;
  visit = [&](const VisualNodeRef& node,
              const std::vector<LayerGroupId>& parent_path,
              const std::size_t sibling_index,
              const std::size_t sibling_count) {
    AgentVisualNode record{
        .node = node,
        .parent_path = parent_path,
        .parent_group = parent_path.empty()
                            ? std::nullopt
                            : std::optional<LayerGroupId>{parent_path.back()},
        .sibling_index = sibling_index,
        .timeline_row = sibling_count - sibling_index - 1,
    };
    if (const auto* layer = layer_for(composition, node)) {
      record.kind = std::holds_alternative<ShapeLayerContent>(layer->content)
                        ? AgentVisualKind::shape
                        : AgentVisualKind::text;
      record.display_name = layer->display_name;
      record.active_range = layer->active_range;
      record.transform = layer->transform;
      record.properties = inspect_visual_properties(composition, node);
      record.owned_masks.reserve(layer->masks.size());
      for (const auto& mask : layer->masks) {
        record.owned_masks.push_back(mask.mask_id);
      }
      record.owned_effects.reserve(layer->effects.size());
      for (const auto& effect : layer->effects) {
        record.owned_effects.push_back(effect.effect_id);
      }
      record.animated_properties.reserve(layer->animations.size());
      for (const auto& animation : layer->animations) {
        record.animated_properties.push_back(animation.property);
      }
      outline.nodes.push_back(std::move(record));
      return;
    }

    const auto* group = group_for(composition, node);
    record.kind = AgentVisualKind::group;
    record.display_name = group->display_name;
    record.active_range = group->active_range;
    record.transform = group->transform;
    record.properties = inspect_visual_properties(composition, node);
    record.animated_properties.reserve(group->animations.size());
    for (const auto& animation : group->animations) {
      record.animated_properties.push_back(animation.property);
    }
    outline.nodes.push_back(std::move(record));

    auto child_path = parent_path;
    child_path.push_back(group->group_id);
    for (std::size_t index = 0; index < group->children.size(); ++index) {
      visit(group->children[index], child_path, index, group->children.size());
    }
  };

  for (std::size_t index = 0; index < outline.roots.size(); ++index) {
    visit(outline.roots[index], {}, index, outline.roots.size());
  }
  return outline;
}

const AgentVisualNode* find_agent_visual_node(
    const AgentProjectOutline& outline,
    const VisualNodeRef& node) noexcept {
  const auto found = std::find_if(
      outline.nodes.begin(), outline.nodes.end(),
      [&node](const AgentVisualNode& candidate) {
        return candidate.node == node;
      });
  return found == outline.nodes.end() ? nullptr : &*found;
}

AgentProjectDiff agent_project_diff(const ProjectSnapshot& before,
                                    const ProjectSnapshot& after) {
  AgentProjectDiff diff{
      .same_project_id = before.project_id == after.project_id,
      .next_revision =
          before.revision_id.value != std::numeric_limits<std::uint64_t>::max() &&
          after.revision_id.value == before.revision_id.value + 1,
      .project_metadata_changed = before.display_name != after.display_name,
      .before_digest = project_snapshot_digest(before),
      .after_digest = project_snapshot_digest(after),
  };
  if (!before.composition || !after.composition) {
    diff.composition_metadata_changed = before.composition != after.composition;
    diff.topology_changed = diff.composition_metadata_changed;
    return diff;
  }
  const auto& lhs = *before.composition;
  const auto& rhs = *after.composition;
  diff.composition_metadata_changed =
      lhs.composition_id != rhs.composition_id ||
      lhs.display_name != rhs.display_name || lhs.canvas != rhs.canvas ||
      lhs.frame_rate != rhs.frame_rate || lhs.duration != rhs.duration;
  diff.topology_changed = !same_topology(lhs, rhs);

  std::unordered_map<std::string, VisualNodeRef> before_nodes;
  std::unordered_map<std::string, VisualNodeRef> after_nodes;
  for (const auto& layer : lhs.layers) {
    before_nodes.emplace(node_key(layer.layer_id), layer.layer_id);
  }
  for (const auto& group : lhs.groups) {
    before_nodes.emplace(node_key(group.group_id), group.group_id);
  }
  for (const auto& layer : rhs.layers) {
    after_nodes.emplace(node_key(layer.layer_id), layer.layer_id);
  }
  for (const auto& group : rhs.groups) {
    after_nodes.emplace(node_key(group.group_id), group.group_id);
  }
  for (const auto& [key, node] : before_nodes) {
    if (!after_nodes.contains(key)) {
      diff.removed_nodes.push_back(node);
      continue;
    }
    bool changed = false;
    if (const auto* layer_id = std::get_if<LayerId>(&node)) {
      changed = *find_layer(lhs, *layer_id) != *find_layer(rhs, *layer_id);
    } else {
      const auto group_id = std::get<LayerGroupId>(node);
      changed = *find_layer_group(lhs, group_id) !=
                *find_layer_group(rhs, group_id);
    }
    if (changed) {
      diff.changed_nodes.push_back(node);
    }
  }
  for (const auto& [key, node] : after_nodes) {
    if (!before_nodes.contains(key)) {
      diff.added_nodes.push_back(node);
    }
  }
  const auto order = [](const VisualNodeRef& lhs_node,
                        const VisualNodeRef& rhs_node) {
    return node_key(lhs_node) < node_key(rhs_node);
  };
  std::sort(diff.added_nodes.begin(), diff.added_nodes.end(), order);
  std::sort(diff.removed_nodes.begin(), diff.removed_nodes.end(), order);
  std::sort(diff.changed_nodes.begin(), diff.changed_nodes.end(), order);
  return diff;
}

}  // namespace refusion::core
