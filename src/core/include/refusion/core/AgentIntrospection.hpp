#pragma once

#include "refusion/core/ProjectDocument.hpp"
#include "refusion/core/VisualPropertyRegistry.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace refusion::core {

enum class AgentVisualKind : std::uint8_t {
  shape,
  text,
  group,
};

// Immutable semantic address used by CLI/MCP projections. Timeline row is the
// row inside the node's current parent scope, matching Studio's reversed visual
// stacking projection. Property ownership never creates additional visual
// nodes in this outline.
struct AgentVisualNode final {
  VisualNodeRef node;
  AgentVisualKind kind{AgentVisualKind::shape};
  std::string display_name;
  std::vector<LayerGroupId> parent_path;
  std::optional<LayerGroupId> parent_group;
  std::size_t sibling_index{0};
  std::size_t timeline_row{0};
  TimeRangeNs active_range;
  Transform2D transform;
  std::vector<MaskId> owned_masks;
  std::vector<EffectId> owned_effects;
  std::vector<AnimatedProperty> animated_properties;
  std::vector<VisualPropertyRecord> properties;

  friend bool operator==(const AgentVisualNode&,
                         const AgentVisualNode&) = default;
};

struct AgentProjectOutline final {
  ProjectId project_id;
  RevisionId revision_id;
  CompositionId composition_id;
  CanvasExtent canvas;
  RationalRate frame_rate;
  ProjectTimeNs duration{0};
  std::string registry_digest;
  std::string contribution_registry_digest;
  std::string snapshot_digest;
  std::vector<VisualNodeRef> roots;
  std::vector<AgentVisualNode> nodes;
  // Shared read-only media projections. Timeline, Inspector, CLI and future
  // MCP clients consume these accepted records; they never rediscover media
  // identity or timing from a host path or container independently.
  std::vector<AssetRecord> assets;
  std::vector<MediaSource> media_sources;
  std::vector<LinkedImport> linked_imports;
  std::vector<VideoClipSnapshot> video_clips;
  std::vector<AudioClipSnapshot> audio_clips;
};

[[nodiscard]] AgentProjectOutline agent_project_outline(
    const ProjectSnapshot& project);

[[nodiscard]] const AgentVisualNode* find_agent_visual_node(
    const AgentProjectOutline& outline,
    const VisualNodeRef& node) noexcept;

struct AgentProjectDiff final {
  bool same_project_id{false};
  bool next_revision{false};
  bool project_metadata_changed{false};
  bool composition_metadata_changed{false};
  bool topology_changed{false};
  std::string before_digest;
  std::string after_digest;
  std::vector<VisualNodeRef> added_nodes;
  std::vector<VisualNodeRef> removed_nodes;
  std::vector<VisualNodeRef> changed_nodes;
};

[[nodiscard]] AgentProjectDiff agent_project_diff(
    const ProjectSnapshot& before,
    const ProjectSnapshot& after);

[[nodiscard]] std::string project_snapshot_digest(
    const ProjectSnapshot& project);

}  // namespace refusion::core
