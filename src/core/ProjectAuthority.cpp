#include "refusion/core/ProjectAuthority.hpp"

#include "refusion/core/CanonicalCoordinates.hpp"
#include "refusion/core/CanonicalText.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace refusion::core {
namespace {

[[nodiscard]] bool is_blank(const std::string& value) {
  return value.empty() || std::all_of(value.begin(), value.end(),
                                      [](const unsigned char character) {
                                        return ascii_space(character);
                                      });
}

[[nodiscard]] std::string visual_node_key(const VisualNodeRef& node) {
  if (const auto* layer_id = std::get_if<LayerId>(&node)) {
    return "layer:" + layer_id->value;
  }
  return "group:" + std::get<LayerGroupId>(node).value;
}

struct NodeLocation final {
  bool found{false};
  std::optional<std::size_t> parent_group_index;
  std::size_t sibling_index{0};
};

[[nodiscard]] NodeLocation locate_node(const CompositionSnapshot& composition,
                                       const VisualNodeRef& node) {
  const auto key = visual_node_key(node);
  for (std::size_t index = 0; index < composition.root_nodes.size(); ++index) {
    if (visual_node_key(composition.root_nodes[index]) == key) {
      return NodeLocation{
          .found = true,
          .parent_group_index = std::nullopt,
          .sibling_index = index,
      };
    }
  }
  for (std::size_t group_index = 0; group_index < composition.groups.size();
       ++group_index) {
    const auto& children = composition.groups[group_index].children;
    for (std::size_t child_index = 0; child_index < children.size();
         ++child_index) {
      if (visual_node_key(children[child_index]) == key) {
        return NodeLocation{
            .found = true,
            .parent_group_index = group_index,
            .sibling_index = child_index,
        };
      }
    }
  }
  return NodeLocation{};
}

[[nodiscard]] bool visual_node_exists(const CompositionSnapshot& composition,
                                      const VisualNodeRef& node) {
  if (const auto* layer_id = std::get_if<LayerId>(&node)) {
    return find_layer(composition, *layer_id) != nullptr;
  }
  return find_layer_group(composition, std::get<LayerGroupId>(node)) != nullptr;
}

[[nodiscard]] const TimeRangeNs&
visual_node_range(const CompositionSnapshot& composition,
                  const VisualNodeRef& node) {
  if (const auto* layer_id = std::get_if<LayerId>(&node)) {
    return find_layer(composition, *layer_id)->active_range;
  }
  return find_layer_group(composition, std::get<LayerGroupId>(node))
      ->active_range;
}

[[nodiscard]] std::vector<VisualNodeRef>&
siblings_for_parent(CompositionSnapshot& composition,
                    const std::optional<std::size_t> parent_group_index) {
  if (parent_group_index.has_value()) {
    return composition.groups[*parent_group_index].children;
  }
  return composition.root_nodes;
}

void materialize_root_order(CompositionSnapshot& composition) {
  if (composition.root_nodes.empty()) {
    composition.root_nodes = composition_root_nodes(composition);
  }
}

[[nodiscard]] bool same_visual_topology(const CompositionSnapshot& lhs,
                                        const CompositionSnapshot& rhs) {
  if (lhs.layers.size() != rhs.layers.size() ||
      lhs.groups.size() != rhs.groups.size() ||
      composition_root_nodes(lhs) != composition_root_nodes(rhs)) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.layers.size(); ++index) {
    if (lhs.layers[index].layer_id != rhs.layers[index].layer_id) {
      return false;
    }
  }
  for (std::size_t index = 0; index < lhs.groups.size(); ++index) {
    if (lhs.groups[index].group_id != rhs.groups[index].group_id ||
        lhs.groups[index].children != rhs.groups[index].children) {
      return false;
    }
  }
  return true;
}

struct MutableVisualProperties final {
  Transform2D* transform{nullptr};
  std::vector<ScalarAnimation>* animations{nullptr};
};

[[nodiscard]] MutableVisualProperties mutable_visual_properties(
    CompositionSnapshot& composition,
    const VisualNodeRef& node) {
  if (const auto* layer_id = std::get_if<LayerId>(&node)) {
    for (auto& layer : composition.layers) {
      if (layer.layer_id == *layer_id) {
        return MutableVisualProperties{
            .transform = &layer.transform,
            .animations = &layer.animations,
        };
      }
    }
  } else {
    const auto& group_id = std::get<LayerGroupId>(node);
    for (auto& group : composition.groups) {
      if (group.group_id == group_id) {
        return MutableVisualProperties{
            .transform = &group.transform,
            .animations = &group.animations,
        };
      }
    }
  }
  return {};
}

void translate_position_property(Transform2D& transform,
                                 std::vector<ScalarAnimation>& animations,
                                 const AnimatedProperty property,
                                 const double delta) {
  const auto animation = std::find_if(
      animations.begin(), animations.end(),
      [property](const ScalarAnimation& candidate) {
        return candidate.property == property;
      });
  if (animation == animations.end()) {
    if (property == AnimatedProperty::position_x) {
      transform.position_x =
          quantize_authored_pixel(transform.position_x + delta);
    } else {
      transform.position_y =
          quantize_authored_pixel(transform.position_y + delta);
    }
    return;
  }
  for (auto& keyframe : animation->keyframes) {
    keyframe.value = quantize_authored_pixel(keyframe.value + delta);
  }
}

[[nodiscard]] double horizontal_anchor(const LocalRect& bounds,
                                       const HorizontalAlignIntent relation) {
  switch (relation) {
    case HorizontalAlignIntent::left:
      return bounds.left;
    case HorizontalAlignIntent::center:
      return (bounds.left + bounds.right) * 0.5;
    case HorizontalAlignIntent::right:
      return bounds.right;
    case HorizontalAlignIntent::none:
      return 0.0;
  }
  return 0.0;
}

[[nodiscard]] double vertical_anchor(const LocalRect& bounds,
                                     const VerticalAlignIntent relation) {
  switch (relation) {
    case VerticalAlignIntent::top:
      return bounds.top;
    case VerticalAlignIntent::center:
      return (bounds.top + bounds.bottom) * 0.5;
    case VerticalAlignIntent::bottom:
      return bounds.bottom;
    case VerticalAlignIntent::none:
      return 0.0;
  }
  return 0.0;
}

[[nodiscard]] bool valid_horizontal_intent(
    const HorizontalAlignIntent intent) noexcept {
  switch (intent) {
    case HorizontalAlignIntent::none:
    case HorizontalAlignIntent::left:
    case HorizontalAlignIntent::center:
    case HorizontalAlignIntent::right:
      return true;
  }
  return false;
}

[[nodiscard]] bool valid_vertical_intent(
    const VerticalAlignIntent intent) noexcept {
  switch (intent) {
    case VerticalAlignIntent::none:
    case VerticalAlignIntent::top:
    case VerticalAlignIntent::center:
    case VerticalAlignIntent::bottom:
      return true;
  }
  return false;
}

[[nodiscard]] bool valid_bounds_basis(
    const AlignmentBoundsBasis basis) noexcept {
  switch (basis) {
    case AlignmentBoundsBasis::geometry:
    case AlignmentBoundsBasis::logical:
    case AlignmentBoundsBasis::ink:
      return true;
  }
  return false;
}

} // namespace

std::vector<AuthoringCapability> authoring_capabilities() {
  return {
      AuthoringCapability{
          .capability_id = "visual.group-nodes",
          .supported = true,
      },
      AuthoringCapability{
          .capability_id = "visual.reparent-nodes",
          .supported = true,
      },
      AuthoringCapability{
          .capability_id = "layer.effect.add",
          .supported = true,
      },
      AuthoringCapability{
          .capability_id = "layout.align-nodes",
          .supported = true,
      },
      AuthoringCapability{
          .capability_id = "effect.property.animate",
          .supported = false,
          .unavailable_code = "RFX-CAP-FX-ANIMATION-001",
      },
  };
}

ProjectAuthority::ProjectAuthority(
    ProjectSnapshot initial_snapshot,
    std::shared_ptr<TextLayoutPort> text_layout_port)
    : active_(std::move(initial_snapshot)),
      text_layout_port_(std::move(text_layout_port)) {
  if (!portable_ascii_identifier(active_.project_id.value)) {
    throw std::invalid_argument("project ID must be a portable ASCII identifier");
  }
  if (active_.revision_id.value == 0) {
    throw std::invalid_argument("initial revision must be non-zero");
  }
  if (is_blank(active_.display_name) ||
      !validate_preserved_utf8(active_.display_name).valid()) {
    throw std::invalid_argument("project name must be valid preserved UTF-8");
  }
  if (active_.composition) {
    const auto validation = validate_composition(*active_.composition);
    if (!validation.valid) {
      throw std::invalid_argument(validation.code + ": " + validation.message);
    }
  }
}

ProjectSnapshot ProjectAuthority::active_snapshot() const {
  std::scoped_lock lock(mutex_);
  return active_;
}

#define REFUSION_DEFINE_PROJECT_PREVIEW(CommandType)                         \
  ApplyResult ProjectAuthority::preview(const CommandType& command) const {  \
    std::scoped_lock lock(mutex_);                                           \
    ProjectAuthority staging(active_, text_layout_port_);                    \
    staging.idempotency_ledger_ = idempotency_ledger_;                       \
    staging.command_id_index_ = command_id_index_;                           \
    return staging.apply(command);                                           \
  }

REFUSION_DEFINE_PROJECT_PREVIEW(RenameProjectCommand)
REFUSION_DEFINE_PROJECT_PREVIEW(ReplaceProjectCommand)
REFUSION_DEFINE_PROJECT_PREVIEW(SetVisualTransformCommand)
REFUSION_DEFINE_PROJECT_PREVIEW(SetVisualPropertyCommand)
REFUSION_DEFINE_PROJECT_PREVIEW(SetLayerEffectsCommand)
REFUSION_DEFINE_PROJECT_PREVIEW(SetLayerMasksCommand)
REFUSION_DEFINE_PROJECT_PREVIEW(AddVisualLayerCommand)
REFUSION_DEFINE_PROJECT_PREVIEW(GroupNodesCommand)
REFUSION_DEFINE_PROJECT_PREVIEW(ReparentNodesCommand)
REFUSION_DEFINE_PROJECT_PREVIEW(AddEffectCommand)
REFUSION_DEFINE_PROJECT_PREVIEW(AlignNodesCommand)
REFUSION_DEFINE_PROJECT_PREVIEW(AnimateEffectPropertyCommand)

#undef REFUSION_DEFINE_PROJECT_PREVIEW

ApplyResult ProjectAuthority::rejected(const CommandId& command_id,
                                       std::string code,
                                       std::string message) const {
  return ApplyResult{
      .status = ApplyStatus::rejected,
      .command_id = command_id,
      .committed_revision = RevisionId{},
      .active_snapshot = active_,
      .diagnostic =
          Diagnostic{
              .code = std::move(code),
              .message = std::move(message),
              .blocking = true,
          },
  };
}

ApplyResult ProjectAuthority::apply(const RenameProjectCommand& command) {
  std::scoped_lock lock(mutex_);

  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }

  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }

  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.kind == RecordedKind::rename &&
        recorded.envelope == command.envelope &&
        recorded.requested_name == command.requested_name) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }

  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }

  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }

  if (is_blank(command.requested_name) ||
      !validate_preserved_utf8(command.requested_name).valid()) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-001",
                    "project name must be valid preserved UTF-8");
  }

  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }

  active_ = ProjectSnapshot{
      .project_id = active_.project_id,
      .revision_id = RevisionId{active_.revision_id.value + 1},
      .display_name = command.requested_name,
      .composition = active_.composition,
  };

  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(command.envelope.idempotency_key.value,
                              RecordedCommand{
                                  .envelope = command.envelope,
                                  .kind = RecordedKind::rename,
                                  .requested_name = command.requested_name,
                                  .candidate = std::nullopt,
                                  .committed_revision = active_.revision_id,
                              });
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

ApplyResult ProjectAuthority::apply(const ReplaceProjectCommand& command) {
  std::scoped_lock lock(mutex_);

  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }
  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }

  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.kind == RecordedKind::replace &&
        recorded.envelope == command.envelope &&
        recorded.candidate == command.candidate) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }

  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }
  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }
  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }
  if (command.candidate.project_id != active_.project_id) {
    return rejected(command.envelope.command_id, "RFX-PROJECT-ID-409",
                    "candidate project ID does not match active project");
  }
  const RevisionId required_revision{active_.revision_id.value + 1};
  if (command.candidate.revision_id != required_revision) {
    return rejected(command.envelope.command_id, "RFX-REV-NEXT-409",
                    "candidate revision must equal active revision plus one");
  }
  if (is_blank(command.candidate.display_name) ||
      !validate_preserved_utf8(command.candidate.display_name).valid()) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-001",
                    "project name must be valid preserved UTF-8");
  }
  if (!command.candidate.composition.has_value()) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-002",
                    "candidate composition is required");
  }
  const auto validation = validate_composition(*command.candidate.composition);
  if (!validation.valid) {
    return rejected(command.envelope.command_id, validation.code,
                    validation.message);
  }

  active_ = command.candidate;
  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(command.envelope.idempotency_key.value,
                              RecordedCommand{
                                  .envelope = command.envelope,
                                  .kind = RecordedKind::replace,
                                  .requested_name = {},
                                  .candidate = command.candidate,
                                  .committed_revision = active_.revision_id,
                              });
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

ApplyResult ProjectAuthority::apply(const SetVisualTransformCommand& command) {
  std::scoped_lock lock(mutex_);

  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }
  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }

  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.kind == RecordedKind::set_visual_transform &&
        recorded.envelope == command.envelope &&
        recorded.visual_node == command.node &&
        recorded.transform == command.transform) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }

  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }
  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }
  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }
  if (!active_.composition) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-002",
                    "project composition is required");
  }

  auto candidate = active_;
  const auto accepted_transform = quantize_transform_pixels(command.transform);
  bool found = false;
  if (const auto* layer_id = std::get_if<LayerId>(&command.node)) {
    for (auto& layer : candidate.composition->layers) {
      if (layer.layer_id == *layer_id) {
        layer.transform = accepted_transform;
        found = true;
        break;
      }
    }
  } else {
    const auto& group_id = std::get<LayerGroupId>(command.node);
    for (auto& group : candidate.composition->groups) {
      if (group.group_id == group_id) {
        group.transform = accepted_transform;
        found = true;
        break;
      }
    }
  }
  if (!found) {
    return rejected(command.envelope.command_id, "RFX-VISUAL-NODE-404",
                    "visual transform target does not exist");
  }

  const auto validation = validate_composition(*candidate.composition);
  if (!validation.valid) {
    return rejected(command.envelope.command_id, validation.code,
                    validation.message);
  }
  candidate.revision_id = RevisionId{active_.revision_id.value + 1};
  active_ = std::move(candidate);

  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(command.envelope.idempotency_key.value,
                              RecordedCommand{
                                  .envelope = command.envelope,
                                  .kind = RecordedKind::set_visual_transform,
                                  .requested_name = {},
                                  .candidate = std::nullopt,
                                  .visual_node = command.node,
                                  .transform = command.transform,
                                  .committed_revision = active_.revision_id,
                              });
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

ApplyResult ProjectAuthority::apply(const SetVisualPropertyCommand& command) {
  std::scoped_lock lock(mutex_);

  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }
  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }
  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.kind == RecordedKind::set_visual_property &&
        recorded.envelope == command.envelope &&
        recorded.visual_node == command.node &&
        recorded.property_id == command.property_id &&
        recorded.property_value == command.value) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }
  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }
  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }
  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }
  if (!active_.composition) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-002",
                    "project composition is required");
  }

  auto candidate = active_;
  const auto validation = set_visual_property(
      *candidate.composition, command.node, command.property_id, command.value);
  if (!validation.valid) {
    return rejected(command.envelope.command_id, validation.code,
                    validation.message);
  }
  candidate.revision_id = RevisionId{active_.revision_id.value + 1};
  active_ = std::move(candidate);

  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(command.envelope.idempotency_key.value,
                              RecordedCommand{
                                  .envelope = command.envelope,
                                  .kind = RecordedKind::set_visual_property,
                                  .requested_name = {},
                                  .candidate = std::nullopt,
                                  .visual_node = command.node,
                                  .transform = std::nullopt,
                                  .property_id = command.property_id,
                                  .property_value = command.value,
                                  .committed_revision = active_.revision_id,
                              });
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

ApplyResult ProjectAuthority::apply(const SetLayerEffectsCommand& command) {
  std::scoped_lock lock(mutex_);

  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }
  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }
  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.kind == RecordedKind::set_layer_effects &&
        recorded.envelope == command.envelope &&
        recorded.layer_id == command.layer_id &&
        recorded.effects == command.effects) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }
  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }
  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }
  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }
  if (!active_.composition) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-002",
                    "project composition is required");
  }

  auto candidate = active_;
  auto* layer = static_cast<LayerSnapshot*>(nullptr);
  for (auto& item : candidate.composition->layers) {
    if (item.layer_id == command.layer_id) {
      layer = &item;
      break;
    }
  }
  if (layer == nullptr) {
    return rejected(command.envelope.command_id, "RFX-LAYER-404",
                    "effect target Layer does not exist");
  }
  layer->effects = command.effects;
  const auto validation = validate_composition(*candidate.composition);
  if (!validation.valid) {
    return rejected(command.envelope.command_id, validation.code,
                    validation.message);
  }
  candidate.revision_id = RevisionId{active_.revision_id.value + 1};
  active_ = std::move(candidate);

  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(
      command.envelope.idempotency_key.value,
      RecordedCommand{
          .envelope = command.envelope,
          .kind = RecordedKind::set_layer_effects,
          .requested_name = {},
          .candidate = std::nullopt,
          .visual_node = VisualNodeRef{command.layer_id},
          .transform = std::nullopt,
          .property_id = std::nullopt,
          .property_value = std::nullopt,
          .layer_id = command.layer_id,
          .effects = command.effects,
          .committed_revision = active_.revision_id,
      });
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

ApplyResult ProjectAuthority::apply(const SetLayerMasksCommand& command) {
  std::scoped_lock lock(mutex_);

  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }
  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }
  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.kind == RecordedKind::set_layer_masks &&
        recorded.envelope == command.envelope &&
        recorded.layer_id == command.layer_id &&
        recorded.masks == command.masks) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }
  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }
  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }
  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }
  if (!active_.composition) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-002",
                    "project composition is required");
  }

  auto candidate = active_;
  auto* layer = static_cast<LayerSnapshot*>(nullptr);
  for (auto& item : candidate.composition->layers) {
    if (item.layer_id == command.layer_id) {
      layer = &item;
      break;
    }
  }
  if (layer == nullptr) {
    return rejected(command.envelope.command_id, "RFX-LAYER-404",
                    "mask target Layer does not exist");
  }
  layer->masks = command.masks;
  const auto validation = validate_composition(*candidate.composition);
  if (!validation.valid) {
    return rejected(command.envelope.command_id, validation.code,
                    validation.message);
  }
  candidate.revision_id = RevisionId{active_.revision_id.value + 1};
  active_ = std::move(candidate);

  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(
      command.envelope.idempotency_key.value,
      RecordedCommand{
          .envelope = command.envelope,
          .kind = RecordedKind::set_layer_masks,
          .requested_name = {},
          .candidate = std::nullopt,
          .visual_node = VisualNodeRef{command.layer_id},
          .transform = std::nullopt,
          .property_id = std::nullopt,
          .property_value = std::nullopt,
          .layer_id = command.layer_id,
          .effects = std::nullopt,
          .masks = command.masks,
          .committed_revision = active_.revision_id,
      });
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

ApplyResult ProjectAuthority::apply(const AddVisualLayerCommand& command) {
  std::scoped_lock lock(mutex_);

  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }
  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }
  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.kind == RecordedKind::add_visual_layer &&
        recorded.envelope == command.envelope &&
        recorded.layer_preset == command.preset) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }
  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }
  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }
  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }
  if (!active_.composition) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-002",
                    "project composition is required");
  }

  auto candidate = active_;
  auto& composition = *candidate.composition;
  std::string preset_name;
  switch (command.preset) {
  case VisualLayerPreset::background:
    preset_name = "background";
    break;
  case VisualLayerPreset::shape:
    preset_name = "shape";
    break;
  case VisualLayerPreset::text:
    preset_name = "text";
    break;
  }
  std::string layer_id = "lyr_ui_" +
                         std::to_string(active_.revision_id.value + 1) + "_" +
                         preset_name;
  std::uint64_t collision = 1;
  while (find_layer(composition, LayerId{layer_id}) != nullptr) {
    layer_id = "lyr_ui_" + std::to_string(active_.revision_id.value + 1) + "_" +
               preset_name + "_" + std::to_string(++collision);
  }
  const double canvas_width = composition.canvas.width_pixels;
  const double canvas_height = composition.canvas.height_pixels;
  LayerSnapshot layer{
      .layer_id = LayerId{layer_id},
      .display_name = command.preset == VisualLayerPreset::background
                          ? "Background"
                      : command.preset == VisualLayerPreset::shape ? "Shape"
                                                                   : "Text",
      .active_range =
          TimeRangeNs{
              .start = 0,
              .duration = composition.duration,
          },
      .transform =
          Transform2D{
              .position_x = canvas_width * 0.5,
              .position_y = canvas_height * 0.5,
          },
  };
  if (command.preset == VisualLayerPreset::text) {
    layer.content = TextLayerContent{
        .text = "New Text",
        .font = FontIdentity{
            .source = FontSourceKind::packaged_asset,
            .family_name = "Noto Sans",
            .asset_id = "font_noto_sans_regular",
            .content_digest =
                "sha256:f5f552c8c5edb61fe6efb824baf4d4de47b1a8689ab4925ff43f7bd6a4ebece5",
        },
        .font_size = std::max(32.0, canvas_height * 0.05),
        .box = TextBox{
            .width = canvas_width * 0.8,
            .height = std::max(48.0, canvas_height * 0.08),
        },
        .direction = ParagraphDirection::left_to_right,
        .horizontal_alignment = TextHorizontalAlignment::center,
        .vertical_alignment = TextVerticalAlignment::center,
        .wrap = TextWrapMode::no_wrap,
        .overflow = TextOverflowMode::visible,
        .line_height_ratio = 1.2,
        .letter_spacing = 0.0,
        .fill = ColorRgba8{.red = 255, .green = 255, .blue = 255},
    };
  } else if (command.preset == VisualLayerPreset::background) {
    layer.content = ShapeLayerContent{
        .width = canvas_width,
        .height = canvas_height,
        .corner_radius = 0.0,
        .fill =
            LinearGradientFill{
                .start_x = -canvas_width * 0.5,
                .start_y = -canvas_height * 0.5,
                .end_x = canvas_width * 0.5,
                .end_y = canvas_height * 0.5,
                .stops =
                    {
                        {.offset = 0.0,
                         .color =
                             ColorRgba8{.red = 8, .green = 14, .blue = 35}},
                        {.offset = 0.55,
                         .color =
                             ColorRgba8{.red = 54, .green = 28, .blue = 112}},
                        {.offset = 1.0,
                         .color =
                             ColorRgba8{.red = 10, .green = 132, .blue = 180}},
                    },
            },
    };
  } else {
    const double width = std::min(720.0, canvas_width * 0.7);
    const double height = std::min(420.0, canvas_height * 0.3);
    layer.content = ShapeLayerContent{
        .width = width,
        .height = height,
        .corner_radius = std::min(width, height) * 0.12,
        .fill =
            LinearGradientFill{
                .start_x = -width * 0.5,
                .start_y = -height * 0.5,
                .end_x = width * 0.5,
                .end_y = height * 0.5,
                .stops =
                    {
                        {.offset = 0.0,
                         .color =
                             ColorRgba8{.red = 124, .green = 92, .blue = 255}},
                        {.offset = 1.0,
                         .color =
                             ColorRgba8{.red = 32, .green = 208, .blue = 255}},
                    },
            },
        .stroke_width = 2.0,
        .stroke_color =
            ColorRgba8{.red = 255, .green = 255, .blue = 255, .alpha = 96},
    };
  }
  auto roots = composition_root_nodes(composition);
  composition.layers.push_back(layer);
  if (command.preset == VisualLayerPreset::background) {
    roots.insert(roots.begin(), layer.layer_id);
  } else {
    roots.emplace_back(layer.layer_id);
  }
  composition.root_nodes = std::move(roots);

  const auto validation = validate_composition(composition);
  if (!validation.valid) {
    return rejected(command.envelope.command_id, validation.code,
                    validation.message);
  }
  candidate.revision_id = RevisionId{active_.revision_id.value + 1};
  active_ = std::move(candidate);
  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(command.envelope.idempotency_key.value,
                              RecordedCommand{
                                  .envelope = command.envelope,
                                  .kind = RecordedKind::add_visual_layer,
                                  .requested_name = {},
                                  .candidate = std::nullopt,
                                  .visual_node = VisualNodeRef{layer.layer_id},
                                  .transform = std::nullopt,
                                  .property_id = std::nullopt,
                                  .property_value = std::nullopt,
                                  .layer_id = layer.layer_id,
                                  .effects = std::nullopt,
                                  .masks = std::nullopt,
                                  .layer_preset = command.preset,
                                  .committed_revision = active_.revision_id,
                              });
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

ApplyResult ProjectAuthority::apply(const GroupNodesCommand& command) {
  std::scoped_lock lock(mutex_);

  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }
  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }
  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.kind == RecordedKind::group_nodes &&
        recorded.envelope == command.envelope &&
        recorded.group_id == command.group_id &&
        recorded.requested_name == command.display_name &&
        recorded.visual_nodes == command.nodes) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }
  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }
  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }
  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }
  if (!active_.composition) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-002",
                    "project composition is required");
  }
  if (is_blank(command.group_id.value) || is_blank(command.display_name) ||
      command.nodes.empty()) {
    return rejected(command.envelope.command_id, "RFX-INTENT-GROUP-001",
                    "group ID, name and at least one child are required");
  }

  auto candidate = active_;
  auto& composition = *candidate.composition;
  materialize_root_order(composition);
  if (find_layer_group(composition, command.group_id) != nullptr ||
      find_layer(composition, LayerId{command.group_id.value}) != nullptr) {
    return rejected(command.envelope.command_id, "RFX-INTENT-GROUP-003",
                    "group ID conflicts with an existing visual node");
  }

  std::unordered_set<std::string> selected_keys;
  std::optional<std::size_t> common_parent;
  bool first_node = true;
  for (const auto& node : command.nodes) {
    if (!selected_keys.emplace(visual_node_key(node)).second) {
      return rejected(command.envelope.command_id, "RFX-INTENT-GROUP-002",
                      "group child selection contains a duplicate node");
    }
    if (!visual_node_exists(composition, node)) {
      return rejected(command.envelope.command_id, "RFX-INTENT-GROUP-004",
                      "group child does not exist");
    }
    const auto location = locate_node(composition, node);
    if (!location.found) {
      return rejected(command.envelope.command_id, "RFX-INTENT-GROUP-004",
                      "group child is not reachable in the hierarchy");
    }
    if (first_node) {
      common_parent = location.parent_group_index;
      first_node = false;
    } else if (common_parent != location.parent_group_index) {
      return rejected(command.envelope.command_id, "RFX-INTENT-GROUP-005",
                      "grouped nodes must be siblings under one parent");
    }
  }

  auto& siblings = siblings_for_parent(composition, common_parent);
  std::vector<VisualNodeRef> children;
  children.reserve(command.nodes.size());
  std::vector<VisualNodeRef> replacement;
  replacement.reserve(siblings.size() - command.nodes.size() + 1);
  bool group_inserted = false;
  for (const auto& sibling : siblings) {
    if (selected_keys.contains(visual_node_key(sibling))) {
      children.push_back(sibling);
      if (!group_inserted) {
        replacement.emplace_back(command.group_id);
        group_inserted = true;
      }
    } else {
      replacement.push_back(sibling);
    }
  }
  if (children.size() != command.nodes.size()) {
    return rejected(command.envelope.command_id, "RFX-INTENT-GROUP-004",
                    "not every selected child is present under the parent");
  }

  ProjectTimeNs range_start = std::numeric_limits<ProjectTimeNs>::max();
  ProjectTimeNs range_end = 0;
  for (const auto& child : children) {
    const auto& range = visual_node_range(composition, child);
    range_start = std::min(range_start, range.start);
    range_end = std::max(range_end, range.end());
  }
  siblings = std::move(replacement);
  composition.groups.push_back(LayerGroupSnapshot{
      .group_id = command.group_id,
      .display_name = command.display_name,
      .active_range =
          TimeRangeNs{
              .start = range_start,
              .duration = range_end - range_start,
          },
      .transform = Transform2D{},
      .animations = {},
      .children = std::move(children),
  });

  const auto validation = validate_composition(composition);
  if (!validation.valid) {
    return rejected(command.envelope.command_id, validation.code,
                    validation.message);
  }
  candidate.revision_id = RevisionId{active_.revision_id.value + 1};
  active_ = std::move(candidate);
  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(command.envelope.idempotency_key.value,
                              RecordedCommand{
                                  .envelope = command.envelope,
                                  .kind = RecordedKind::group_nodes,
                                  .requested_name = command.display_name,
                                  .group_id = command.group_id,
                                  .visual_nodes = command.nodes,
                                  .committed_revision = active_.revision_id,
                              });
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

ApplyResult ProjectAuthority::apply(const ReparentNodesCommand& command) {
  std::scoped_lock lock(mutex_);

  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }
  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }
  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.kind == RecordedKind::reparent_nodes &&
        recorded.envelope == command.envelope &&
        recorded.visual_nodes == command.nodes &&
        recorded.parent_group_id == command.new_parent_group &&
        recorded.insertion_index == command.insertion_index) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }
  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }
  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }
  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }
  if (!active_.composition) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-002",
                    "project composition is required");
  }
  if (command.nodes.empty()) {
    return rejected(command.envelope.command_id, "RFX-INTENT-REPARENT-001",
                    "at least one visual node is required");
  }

  auto candidate = active_;
  auto& composition = *candidate.composition;
  materialize_root_order(composition);
  std::unordered_set<std::string> selected_keys;
  std::optional<std::size_t> source_parent;
  bool first_node = true;
  for (const auto& node : command.nodes) {
    if (!selected_keys.emplace(visual_node_key(node)).second) {
      return rejected(command.envelope.command_id, "RFX-INTENT-REPARENT-002",
                      "reparent selection contains a duplicate node");
    }
    if (!visual_node_exists(composition, node)) {
      return rejected(command.envelope.command_id, "RFX-INTENT-REPARENT-003",
                      "reparent target node does not exist");
    }
    const auto location = locate_node(composition, node);
    if (!location.found) {
      return rejected(command.envelope.command_id, "RFX-INTENT-REPARENT-003",
                      "reparent target node is not reachable");
    }
    if (first_node) {
      source_parent = location.parent_group_index;
      first_node = false;
    } else if (source_parent != location.parent_group_index) {
      return rejected(command.envelope.command_id, "RFX-INTENT-REPARENT-004",
                      "reparented nodes must be siblings under one parent");
    }
  }

  std::optional<std::size_t> target_parent;
  if (command.new_parent_group.has_value()) {
    for (std::size_t index = 0; index < composition.groups.size(); ++index) {
      if (composition.groups[index].group_id == *command.new_parent_group) {
        target_parent = index;
        break;
      }
    }
    if (!target_parent.has_value()) {
      return rejected(command.envelope.command_id, "RFX-INTENT-REPARENT-005",
                      "new parent Group does not exist");
    }
  }

  const auto source_siblings = siblings_for_parent(composition, source_parent);
  std::vector<VisualNodeRef> moved;
  std::vector<VisualNodeRef> remaining;
  moved.reserve(command.nodes.size());
  remaining.reserve(source_siblings.size() - command.nodes.size());
  for (const auto& sibling : source_siblings) {
    if (selected_keys.contains(visual_node_key(sibling))) {
      moved.push_back(sibling);
    } else {
      remaining.push_back(sibling);
    }
  }
  if (moved.size() != command.nodes.size()) {
    return rejected(
        command.envelope.command_id, "RFX-INTENT-REPARENT-003",
        "not every selected node is present under the source parent");
  }

  if (source_parent == target_parent) {
    if (command.insertion_index > remaining.size()) {
      return rejected(command.envelope.command_id, "RFX-INTENT-REPARENT-006",
                      "insertion index exceeds the target child count");
    }
    remaining.insert(remaining.begin() +
                         static_cast<std::ptrdiff_t>(command.insertion_index),
                     moved.begin(), moved.end());
    siblings_for_parent(composition, source_parent) = std::move(remaining);
  } else {
    siblings_for_parent(composition, source_parent) = std::move(remaining);
    auto& target_siblings = siblings_for_parent(composition, target_parent);
    if (command.insertion_index > target_siblings.size()) {
      return rejected(command.envelope.command_id, "RFX-INTENT-REPARENT-006",
                      "insertion index exceeds the target child count");
    }
    target_siblings.insert(
        target_siblings.begin() +
            static_cast<std::ptrdiff_t>(command.insertion_index),
        moved.begin(), moved.end());
  }

  const auto validation = validate_composition(composition);
  if (!validation.valid) {
    return rejected(command.envelope.command_id, validation.code,
                    validation.message);
  }
  candidate.revision_id = RevisionId{active_.revision_id.value + 1};
  active_ = std::move(candidate);
  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(command.envelope.idempotency_key.value,
                              RecordedCommand{
                                  .envelope = command.envelope,
                                  .kind = RecordedKind::reparent_nodes,
                                  .visual_nodes = command.nodes,
                                  .parent_group_id = command.new_parent_group,
                                  .insertion_index = command.insertion_index,
                                  .committed_revision = active_.revision_id,
                              });
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

ApplyResult ProjectAuthority::apply(const AddEffectCommand& command) {
  std::scoped_lock lock(mutex_);

  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }
  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }
  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.kind == RecordedKind::add_effect &&
        recorded.envelope == command.envelope &&
        recorded.layer_id == command.layer_id &&
        recorded.effect == command.effect &&
        recorded.insertion_index == command.insertion_index) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }
  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }
  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }
  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }
  if (!active_.composition) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-002",
                    "project composition is required");
  }
  if (is_blank(command.effect.effect_id.value)) {
    return rejected(command.envelope.command_id, "RFX-INTENT-EFFECT-001",
                    "effect ID is required");
  }

  auto candidate = active_;
  auto& composition = *candidate.composition;
  auto layer =
      std::find_if(composition.layers.begin(), composition.layers.end(),
                   [&command](const LayerSnapshot& item) {
                     return item.layer_id == command.layer_id;
                   });
  if (layer == composition.layers.end()) {
    return rejected(command.envelope.command_id, "RFX-LAYER-404",
                    "effect target Layer does not exist");
  }
  const std::size_t insertion_index =
      command.insertion_index.value_or(layer->effects.size());
  if (insertion_index > layer->effects.size()) {
    return rejected(command.envelope.command_id, "RFX-INTENT-EFFECT-002",
                    "effect insertion index exceeds the FX stack size");
  }
  layer->effects.insert(layer->effects.begin() +
                            static_cast<std::ptrdiff_t>(insertion_index),
                        command.effect);
  if (!same_visual_topology(*active_.composition, composition)) {
    return rejected(command.envelope.command_id, "RFX-INTENT-TOPOLOGY-001",
                    "AddEffect must preserve visual topology");
  }
  const auto validation = validate_composition(composition);
  if (!validation.valid) {
    return rejected(command.envelope.command_id, validation.code,
                    validation.message);
  }
  candidate.revision_id = RevisionId{active_.revision_id.value + 1};
  active_ = std::move(candidate);
  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(
      command.envelope.idempotency_key.value,
      RecordedCommand{
          .envelope = command.envelope,
          .kind = RecordedKind::add_effect,
          .visual_node = VisualNodeRef{command.layer_id},
          .layer_id = command.layer_id,
          .insertion_index = command.insertion_index,
          .effect = command.effect,
          .committed_revision = active_.revision_id,
      });
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

ApplyResult ProjectAuthority::apply(const AlignNodesCommand& command) {
  std::scoped_lock lock(mutex_);
  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }
  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }
  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.kind == RecordedKind::align_nodes &&
        recorded.envelope == command.envelope &&
        recorded.alignment == command) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }
  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }
  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }
  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }
  if (!active_.composition) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-002",
                    "project composition is required");
  }
  if (!visual_node_exists(*active_.composition, command.subject) ||
      !visual_node_exists(*active_.composition, command.target)) {
    return rejected(command.envelope.command_id, "RFX-VISUAL-NODE-404",
                    "alignment subject and target must exist");
  }
  if (!valid_horizontal_intent(command.horizontal) ||
      !valid_vertical_intent(command.vertical) ||
      !valid_bounds_basis(command.bounds_basis)) {
    return rejected(command.envelope.command_id, "RFX-INTENT-ALIGN-003",
                    "alignment relation or bounds basis is invalid");
  }
  if (command.subject == command.target ||
      (command.horizontal == HorizontalAlignIntent::none &&
       command.vertical == VerticalAlignIntent::none)) {
    return rejected(command.envelope.command_id, "RFX-INTENT-ALIGN-001",
                    "alignment requires distinct nodes and at least one axis");
  }
  const auto& composition = *active_.composition;
  if (visual_node_is_ancestor(composition, command.subject, command.target) ||
      visual_node_is_ancestor(composition, command.target, command.subject)) {
    return rejected(
        command.envelope.command_id, "RFX-INTENT-ALIGN-002",
        "one-shot alignment does not accept ancestor/descendant targets");
  }

  const auto measured = measure_visual_nodes(
      composition, command.composition_time, text_layout_port_.get());
  if (!measured.succeeded()) {
    return rejected(command.envelope.command_id,
                    measured.diagnostic->code,
                    measured.diagnostic->message);
  }
  const auto* subject = find_visual_measurement(
      *measured.snapshot, command.subject);
  const auto* target = find_visual_measurement(
      *measured.snapshot, command.target);
  if (subject == nullptr || target == nullptr) {
    return rejected(command.envelope.command_id,
                    "RFX-MEASURE-NODE-INACTIVE-001",
                    "alignment nodes must both be active at the exact time");
  }
  const auto subject_bounds = measurement_bounds(
      *subject, command.bounds_basis);
  const auto target_bounds = measurement_bounds(*target, command.bounds_basis);
  if (!subject_bounds || !target_bounds) {
    return rejected(command.envelope.command_id, "RFX-MEASURE-PORT-001",
                    "selected alignment basis requires an admitted Text layout port");
  }

  const double world_delta_x =
      command.horizontal == HorizontalAlignIntent::none
          ? 0.0
          : horizontal_anchor(*target_bounds, command.horizontal) -
                horizontal_anchor(*subject_bounds, command.horizontal);
  const double world_delta_y =
      command.vertical == VerticalAlignIntent::none
          ? 0.0
          : vertical_anchor(*target_bounds, command.vertical) -
                vertical_anchor(*subject_bounds, command.vertical);
  if (std::abs(world_delta_x) <= 1.0e-9 &&
      std::abs(world_delta_y) <= 1.0e-9) {
    return rejected(command.envelope.command_id, "RFX-INTENT-ALIGN-NOOP-001",
                    "nodes are already aligned on the requested axes");
  }

  const auto& parent = subject->parent_world_transform;
  const double determinant = parent.m00 * parent.m11 - parent.m01 * parent.m10;
  if (!std::isfinite(determinant) || std::abs(determinant) <= 1.0e-12) {
    return rejected(command.envelope.command_id,
                    "RFX-INTENT-ALIGN-TRANSFORM-001",
                    "subject parent transform is not invertible");
  }
  const double local_delta_x =
      (parent.m11 * world_delta_x - parent.m01 * world_delta_y) /
      determinant;
  const double local_delta_y =
      (-parent.m10 * world_delta_x + parent.m00 * world_delta_y) /
      determinant;

  auto candidate = active_;
  auto properties = mutable_visual_properties(
      *candidate.composition, command.subject);
  if (properties.transform == nullptr || properties.animations == nullptr) {
    return rejected(command.envelope.command_id, "RFX-VISUAL-NODE-404",
                    "alignment subject transform does not exist");
  }
  translate_position_property(*properties.transform, *properties.animations,
                              AnimatedProperty::position_x, local_delta_x);
  translate_position_property(*properties.transform, *properties.animations,
                              AnimatedProperty::position_y, local_delta_y);

  if (!same_visual_topology(composition, *candidate.composition)) {
    return rejected(command.envelope.command_id, "RFX-INTENT-TOPOLOGY-001",
                    "AlignNodes must preserve visual topology");
  }
  const auto validation = validate_composition(*candidate.composition);
  if (!validation.valid) {
    return rejected(command.envelope.command_id, validation.code,
                    validation.message);
  }

  const auto verified = measure_visual_nodes(
      *candidate.composition, command.composition_time,
      text_layout_port_.get());
  if (!verified.succeeded()) {
    return rejected(command.envelope.command_id,
                    verified.diagnostic->code,
                    verified.diagnostic->message);
  }
  const auto* verified_subject = find_visual_measurement(
      *verified.snapshot, command.subject);
  const auto* verified_target = find_visual_measurement(
      *verified.snapshot, command.target);
  if (verified_subject == nullptr || verified_target == nullptr) {
    return rejected(command.envelope.command_id,
                    "RFX-MEASURE-POSTCONDITION-001",
                    "aligned candidate lost an active measurement node");
  }
  const auto verified_subject_bounds = measurement_bounds(
      *verified_subject, command.bounds_basis);
  const auto verified_target_bounds = measurement_bounds(
      *verified_target, command.bounds_basis);
  if (!verified_subject_bounds || !verified_target_bounds) {
    return rejected(command.envelope.command_id,
                    "RFX-MEASURE-POSTCONDITION-001",
                    "aligned candidate lost its requested bounds basis");
  }
  constexpr double tolerance_pixels = 0.25;
  const bool horizontal_ok =
      command.horizontal == HorizontalAlignIntent::none ||
      std::abs(horizontal_anchor(*verified_target_bounds, command.horizontal) -
               horizontal_anchor(*verified_subject_bounds,
                                 command.horizontal)) <= tolerance_pixels;
  const bool vertical_ok =
      command.vertical == VerticalAlignIntent::none ||
      std::abs(vertical_anchor(*verified_target_bounds, command.vertical) -
               vertical_anchor(*verified_subject_bounds,
                               command.vertical)) <= tolerance_pixels;
  if (!horizontal_ok || !vertical_ok) {
    return rejected(command.envelope.command_id,
                    "RFX-MEASURE-POSTCONDITION-001",
                    "aligned candidate exceeds the 0.25px postcondition");
  }

  candidate.revision_id = RevisionId{active_.revision_id.value + 1};
  active_ = std::move(candidate);
  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(
      command.envelope.idempotency_key.value,
      RecordedCommand{
          .envelope = command.envelope,
          .kind = RecordedKind::align_nodes,
          .alignment = command,
          .committed_revision = active_.revision_id,
      });
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

ApplyResult
ProjectAuthority::apply(const AnimateEffectPropertyCommand& command) {
  std::scoped_lock lock(mutex_);
  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000",
                    "command ID is required");
  }
  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }
  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }
  if (!active_.composition) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-002",
                    "project composition is required");
  }
  if (is_blank(command.property_id) || command.keyframes.empty()) {
    return rejected(command.envelope.command_id, "RFX-INTENT-EFFECT-003",
                    "effect animation property and keyframes are required");
  }
  const auto* layer = find_layer(*active_.composition, command.layer_id);
  if (layer == nullptr) {
    return rejected(command.envelope.command_id, "RFX-LAYER-404",
                    "effect animation target Layer does not exist");
  }
  const auto effect = std::find_if(layer->effects.begin(), layer->effects.end(),
                                   [&command](const LayerEffect& item) {
                                     return item.effect_id == command.effect_id;
                                   });
  if (effect == layer->effects.end()) {
    return rejected(command.envelope.command_id, "RFX-EFFECT-404",
                    "effect animation target does not exist");
  }
  return rejected(
      command.envelope.command_id, "RFX-CAP-FX-ANIMATION-001",
      "effect properties are not animatable in the current capability profile");
}

} // namespace refusion::core
