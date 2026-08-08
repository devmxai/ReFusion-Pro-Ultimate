#pragma once

#include "refusion/core/ProjectDocument.hpp"
#include "refusion/core/VisualMeasurement.hpp"
#include "refusion/core/VisualPropertyRegistry.hpp"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace refusion::core {

struct CommandId final {
  std::string value;

  friend bool operator==(const CommandId&, const CommandId&) = default;
};

struct IdempotencyKey final {
  std::string value;

  friend bool operator==(const IdempotencyKey&,
                         const IdempotencyKey&) = default;
};

struct CommandEnvelope final {
  CommandId command_id;
  RevisionId expected_revision;
  IdempotencyKey idempotency_key;

  friend bool operator==(const CommandEnvelope&,
                         const CommandEnvelope&) = default;
};

struct RenameProjectCommand final {
  CommandEnvelope envelope;
  std::string requested_name;
};

// A complete candidate replacement used by validated external project sources.
// It must target the same project and carry exactly active_revision + 1.
struct ReplaceProjectCommand final {
  CommandEnvelope envelope;
  ProjectSnapshot candidate;
};

// Bounded EXP-003 UI/Agent parity slice. The command targets one stable visual
// node and replaces its typed Transform2D atomically; QML never receives or
// mutates a ProjectSnapshot.
struct SetVisualTransformCommand final {
  CommandEnvelope envelope;
  VisualNodeRef node;
  Transform2D transform;
};

struct SetVisualPropertyCommand final {
  CommandEnvelope envelope;
  VisualNodeRef node;
  VisualPropertyId property_id;
  VisualPropertyValue value;
};

// Replaces one Layer's ordered local FX stack atomically. The stack is Core
// project state; Studio only submits intent and never owns backend filters.
struct SetLayerEffectsCommand final {
  CommandEnvelope envelope;
  LayerId layer_id;
  std::vector<LayerEffect> effects;
};

struct SetLayerMasksCommand final {
  CommandEnvelope envelope;
  LayerId layer_id;
  std::vector<LayerMask> masks;
};

enum class VisualLayerPreset : std::uint8_t {
  background,
  shape,
  text,
};

struct AddVisualLayerCommand final {
  CommandEnvelope envelope;
  VisualLayerPreset preset{VisualLayerPreset::shape};
};

// EXP-006A semantic intents. These commands mutate hierarchy/effect ownership
// atomically through the same revision authority used by every authoring
// client.
struct GroupNodesCommand final {
  CommandEnvelope envelope;
  LayerGroupId group_id;
  std::string display_name;
  std::vector<VisualNodeRef> nodes;
};

struct ReparentNodesCommand final {
  CommandEnvelope envelope;
  std::vector<VisualNodeRef> nodes;
  std::optional<LayerGroupId> new_parent_group;
  std::size_t insertion_index{0};
};

struct AddEffectCommand final {
  CommandEnvelope envelope;
  LayerId layer_id;
  LayerEffect effect;
  std::optional<std::size_t> insertion_index;
};

enum class HorizontalAlignIntent : std::uint8_t {
  none,
  left,
  center,
  right,
};

enum class VerticalAlignIntent : std::uint8_t {
  none,
  top,
  center,
  bottom,
};

// One-shot measured authoring intent. It changes authored Position values only;
// no persistent constraint or derived bounds enter project truth.
struct AlignNodesCommand final {
  CommandEnvelope envelope;
  VisualNodeRef subject;
  VisualNodeRef target;
  ProjectTimeNs composition_time{0};
  HorizontalAlignIntent horizontal{HorizontalAlignIntent::none};
  VerticalAlignIntent vertical{VerticalAlignIntent::none};
  AlignmentBoundsBasis bounds_basis{AlignmentBoundsBasis::geometry};

  friend bool operator==(const AlignNodesCommand&,
                         const AlignNodesCommand&) = default;
};

// Current project state has no animation binding for effect properties. A typed
// unavailable command prevents clients from inventing duplicate-Layer
// fallbacks.
struct AnimateEffectPropertyCommand final {
  CommandEnvelope envelope;
  LayerId layer_id;
  EffectId effect_id;
  std::string property_id;
  std::vector<ScalarKeyframe> keyframes;
};

struct AuthoringCapability final {
  std::string capability_id;
  bool supported{false};
  std::string unavailable_code;

  friend bool operator==(const AuthoringCapability&,
                         const AuthoringCapability&) = default;
};

[[nodiscard]] std::vector<AuthoringCapability> authoring_capabilities();

struct Diagnostic final {
  std::string code;
  std::string message;
  bool blocking{false};
};

enum class ApplyStatus : std::uint8_t {
  rejected,
  accepted,
  replayed,
};

struct ApplyResult final {
  ApplyStatus status{ApplyStatus::rejected};
  CommandId command_id;
  RevisionId committed_revision;
  ProjectSnapshot active_snapshot;
  Diagnostic diagnostic;

  [[nodiscard]] bool accepted() const noexcept {
    return status == ApplyStatus::accepted || status == ApplyStatus::replayed;
  }

  [[nodiscard]] bool replayed() const noexcept {
    return status == ApplyStatus::replayed;
  }
};

class ProjectAuthority final {
 public:
  explicit ProjectAuthority(
      ProjectSnapshot initial_snapshot,
      std::shared_ptr<TextLayoutPort> text_layout_port = nullptr);

  [[nodiscard]] ProjectSnapshot active_snapshot() const;
  [[nodiscard]] ApplyResult preview(const RenameProjectCommand& command) const;
  [[nodiscard]] ApplyResult preview(const ReplaceProjectCommand& command) const;
  [[nodiscard]] ApplyResult preview(
      const SetVisualTransformCommand& command) const;
  [[nodiscard]] ApplyResult preview(
      const SetVisualPropertyCommand& command) const;
  [[nodiscard]] ApplyResult preview(
      const SetLayerEffectsCommand& command) const;
  [[nodiscard]] ApplyResult preview(
      const SetLayerMasksCommand& command) const;
  [[nodiscard]] ApplyResult preview(const AddVisualLayerCommand& command) const;
  [[nodiscard]] ApplyResult preview(const GroupNodesCommand& command) const;
  [[nodiscard]] ApplyResult preview(const ReparentNodesCommand& command) const;
  [[nodiscard]] ApplyResult preview(const AddEffectCommand& command) const;
  [[nodiscard]] ApplyResult preview(const AlignNodesCommand& command) const;
  [[nodiscard]] ApplyResult preview(
      const AnimateEffectPropertyCommand& command) const;
  [[nodiscard]] ApplyResult apply(const RenameProjectCommand& command);
  [[nodiscard]] ApplyResult apply(const ReplaceProjectCommand& command);
  [[nodiscard]] ApplyResult apply(const SetVisualTransformCommand& command);
  [[nodiscard]] ApplyResult apply(const SetVisualPropertyCommand& command);
  [[nodiscard]] ApplyResult apply(const SetLayerEffectsCommand& command);
  [[nodiscard]] ApplyResult apply(const SetLayerMasksCommand& command);
  [[nodiscard]] ApplyResult apply(const AddVisualLayerCommand& command);
  [[nodiscard]] ApplyResult apply(const GroupNodesCommand& command);
  [[nodiscard]] ApplyResult apply(const ReparentNodesCommand& command);
  [[nodiscard]] ApplyResult apply(const AddEffectCommand& command);
  [[nodiscard]] ApplyResult apply(const AlignNodesCommand& command);
  [[nodiscard]] ApplyResult apply(const AnimateEffectPropertyCommand& command);

 private:
  enum class RecordedKind : std::uint8_t {
    rename,
    replace,
    set_visual_transform,
    set_visual_property,
    set_layer_effects,
    set_layer_masks,
    add_visual_layer,
    group_nodes,
    reparent_nodes,
    add_effect,
    align_nodes,
  };

  struct RecordedCommand final {
    CommandEnvelope envelope;
    RecordedKind kind{RecordedKind::rename};
    std::string requested_name;
    std::optional<ProjectSnapshot> candidate;
    std::optional<VisualNodeRef> visual_node;
    std::optional<Transform2D> transform;
    std::optional<VisualPropertyId> property_id;
    std::optional<VisualPropertyValue> property_value;
    std::optional<LayerId> layer_id;
    std::optional<std::vector<LayerEffect>> effects;
    std::optional<std::vector<LayerMask>> masks;
    std::optional<VisualLayerPreset> layer_preset;
    std::optional<LayerGroupId> group_id;
    std::optional<std::vector<VisualNodeRef>> visual_nodes;
    std::optional<LayerGroupId> parent_group_id;
    std::optional<std::size_t> insertion_index;
    std::optional<LayerEffect> effect;
    std::optional<AlignNodesCommand> alignment;
    RevisionId committed_revision;
  };

  [[nodiscard]] ApplyResult rejected(const CommandId& command_id,
                                     std::string code,
                                     std::string message) const;

  mutable std::mutex mutex_;
  ProjectSnapshot active_;
  std::shared_ptr<TextLayoutPort> text_layout_port_;
  std::unordered_map<std::string, RecordedCommand> idempotency_ledger_;
  std::unordered_map<std::string, std::string> command_id_index_;
};

} // namespace refusion::core
