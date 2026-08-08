#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace refusion::core {

using ProjectTimeNs = std::uint64_t;

struct ProjectId final {
  std::string value;

  friend bool operator==(const ProjectId&, const ProjectId&) = default;
};

struct RevisionId final {
  std::uint64_t value{0};

  friend bool operator==(const RevisionId&, const RevisionId&) = default;
};

struct CompositionId final {
  std::string value;

  friend bool operator==(const CompositionId&, const CompositionId&) = default;
};

struct LayerId final {
  std::string value;

  friend bool operator==(const LayerId&, const LayerId&) = default;
};

struct LayerGroupId final {
  std::string value;

  friend bool operator==(const LayerGroupId&, const LayerGroupId&) = default;
};

using VisualNodeRef = std::variant<LayerId, LayerGroupId>;

struct RationalRate final {
  std::uint32_t numerator{0};
  std::uint32_t denominator{1};

  [[nodiscard]] bool valid() const noexcept;

  friend bool operator==(const RationalRate&, const RationalRate&) = default;
};

struct TimeRangeNs final {
  ProjectTimeNs start{0};
  ProjectTimeNs duration{0};

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] ProjectTimeNs end() const noexcept;
  [[nodiscard]] bool contains(ProjectTimeNs time) const noexcept;

  friend bool operator==(const TimeRangeNs&, const TimeRangeNs&) = default;
};

struct CanvasExtent final {
  std::uint32_t width_pixels{0};
  std::uint32_t height_pixels{0};

  [[nodiscard]] bool valid() const noexcept;

  friend bool operator==(const CanvasExtent&, const CanvasExtent&) = default;
};

struct ColorRgba8 final {
  std::uint8_t red{0};
  std::uint8_t green{0};
  std::uint8_t blue{0};
  std::uint8_t alpha{255};

  friend bool operator==(const ColorRgba8&, const ColorRgba8&) = default;
};

struct Transform2D final {
  // position is always expressed in parent pixels. For a root node, the
  // parent is the Composition. anchor is always expressed in local pixels.
  // RFX4 spells these domains as parent_px and local_px respectively.
  double position_x{0.0};
  double position_y{0.0};
  double anchor_x{0.0};
  double anchor_y{0.0};
  double scale_x{1.0};
  double scale_y{1.0};
  double rotation_degrees{0.0};
  double opacity{1.0};

  friend bool operator==(const Transform2D&, const Transform2D&) = default;
};

struct GradientStop final {
  double offset{0.0};
  ColorRgba8 color;

  friend bool operator==(const GradientStop&, const GradientStop&) = default;
};

struct LinearGradientFill final {
  double start_x{0.0};
  double start_y{0.0};
  double end_x{0.0};
  double end_y{0.0};
  std::vector<GradientStop> stops;

  friend bool operator==(const LinearGradientFill&,
                         const LinearGradientFill&) = default;
};

struct RadialGradientFill final {
  double center_x{0.0};
  double center_y{0.0};
  double radius{0.0};
  std::vector<GradientStop> stops;

  friend bool operator==(const RadialGradientFill&,
                         const RadialGradientFill&) = default;
};

// A single portable paint value. ColorRgba8 is the solid-fill case; gradients
// carry ordered stops and geometry in the Layer's local composition-pixel space.
using ShapeFill =
    std::variant<ColorRgba8, LinearGradientFill, RadialGradientFill>;

enum class BlendMode : std::uint8_t {
  normal,
  multiply,
  screen,
  overlay,
};

enum class AnimatedProperty : std::uint8_t {
  position_x,
  position_y,
  scale_x,
  scale_y,
  rotation_degrees,
  opacity,
};

struct ScalarKeyframe final {
  ProjectTimeNs time{0};
  double value{0.0};

  friend bool operator==(const ScalarKeyframe&, const ScalarKeyframe&) = default;
};

struct ScalarAnimation final {
  AnimatedProperty property{AnimatedProperty::position_x};
  std::vector<ScalarKeyframe> keyframes;

  friend bool operator==(const ScalarAnimation&, const ScalarAnimation&) = default;
};

struct ShapeLayerContent final {
  double width{0.0};
  double height{0.0};
  double corner_radius{0.0};
  ShapeFill fill;
  double stroke_width{0.0};
  ColorRgba8 stroke_color;

  friend bool operator==(const ShapeLayerContent&, const ShapeLayerContent&) = default;
};

enum class FontSourceKind : std::uint8_t {
  system_family,
  packaged_asset,
};

// A system family is a non-qualified authoring convenience. A packaged asset
// is qualified only when both its stable asset ID and immutable sha256 digest
// are present; resolution against actual bytes belongs to TextLayoutPort.
struct FontIdentity final {
  FontSourceKind source{FontSourceKind::system_family};
  std::string family_name;
  std::string asset_id;
  std::string content_digest;

  [[nodiscard]] bool qualified() const noexcept;

  friend bool operator==(const FontIdentity&, const FontIdentity&) = default;
};

struct TextBox final {
  double width{0.0};
  double height{0.0};
  double padding_top{0.0};
  double padding_right{0.0};
  double padding_bottom{0.0};
  double padding_left{0.0};

  friend bool operator==(const TextBox&, const TextBox&) = default;
};

enum class ParagraphDirection : std::uint8_t {
  left_to_right,
  right_to_left,
};

enum class TextHorizontalAlignment : std::uint8_t {
  start,
  center,
  end,
  left,
  right,
};

enum class TextVerticalAlignment : std::uint8_t {
  top,
  center,
  bottom,
};

enum class TextWrapMode : std::uint8_t {
  no_wrap,
  word,
};

enum class TextOverflowMode : std::uint8_t {
  clip,
  visible,
};

struct LocalRect final {
  double left{0.0};
  double top{0.0};
  double right{0.0};
  double bottom{0.0};

  friend bool operator==(const LocalRect&, const LocalRect&) = default;
};

struct TextLineLayout final {
  std::size_t utf8_start{0};
  std::size_t utf8_length{0};
  double origin_x{0.0};
  double baseline_y{0.0};
  double logical_width{0.0};
  LocalRect ink_bounds;
  // Derived shaping receipt. These values are never serialized as project
  // truth; they let qualification compare the exact glyph program produced by
  // the same packaged font bytes on different toolchains.
  std::vector<std::uint32_t> glyph_ids;
  std::vector<double> glyph_positions_x;

  friend bool operator==(const TextLineLayout&, const TextLineLayout&) = default;
};

// Immutable derived layout. This record is produced from the accepted Text
// descriptor by TextLayoutPort and must never be serialized as authored truth.
struct TextLayoutResult final {
  LocalRect layout_box;
  LocalRect content_box;
  LocalRect logical_bounds;
  LocalRect ink_bounds;
  LocalRect clipped_bounds;
  std::vector<TextLineLayout> lines;
  std::vector<double> baselines;
  double ascent{0.0};
  double descent{0.0};
  double leading{0.0};
  bool overflowed{false};
  bool font_qualified{false};
  std::string resolved_font_digest;
  std::string layout_engine_digest;
  std::string cache_key;

  friend bool operator==(const TextLayoutResult&, const TextLayoutResult&) = default;
};

struct DerivedVisualBounds final {
  LocalRect geometry_local;
  LocalRect masked_local;
  LocalRect effect_local;
  LocalRect world;

  friend bool operator==(const DerivedVisualBounds&,
                         const DerivedVisualBounds&) = default;
};

struct TextLayerContent final {
  std::string text;
  FontIdentity font;
  double font_size{0.0};
  TextBox box;
  ParagraphDirection direction{ParagraphDirection::left_to_right};
  TextHorizontalAlignment horizontal_alignment{
      TextHorizontalAlignment::start};
  TextVerticalAlignment vertical_alignment{TextVerticalAlignment::center};
  TextWrapMode wrap{TextWrapMode::no_wrap};
  TextOverflowMode overflow{TextOverflowMode::visible};
  double line_height_ratio{1.2};
  double letter_spacing{0.0};
  ColorRgba8 fill;

  friend bool operator==(const TextLayerContent&, const TextLayerContent&) = default;
};

using LayerContent = std::variant<ShapeLayerContent, TextLayerContent>;

struct MaskId final {
  std::string value;

  friend bool operator==(const MaskId&, const MaskId&) = default;
};

struct RoundedRectMask final {
  double position_x{0.0};
  double position_y{0.0};
  double width{0.0};
  double height{0.0};
  double corner_radius{0.0};

  friend bool operator==(const RoundedRectMask&,
                         const RoundedRectMask&) = default;
};

struct LayerMask final {
  MaskId mask_id;
  bool enabled{true};
  bool inverted{false};
  RoundedRectMask geometry;

  friend bool operator==(const LayerMask&, const LayerMask&) = default;
};

struct EffectId final {
  std::string value;

  friend bool operator==(const EffectId&, const EffectId&) = default;
};

struct GaussianBlurEffect final {
  double sigma_x{0.0};
  double sigma_y{0.0};

  friend bool operator==(const GaussianBlurEffect&,
                         const GaussianBlurEffect&) = default;
};

struct DropShadowEffect final {
  double offset_x{0.0};
  double offset_y{0.0};
  double sigma_x{0.0};
  double sigma_y{0.0};
  ColorRgba8 color;

  friend bool operator==(const DropShadowEffect&,
                         const DropShadowEffect&) = default;
};

struct GlowEffect final {
  double sigma{0.0};
  ColorRgba8 color;

  friend bool operator==(const GlowEffect&, const GlowEffect&) = default;
};

using LayerEffectParameters =
    std::variant<GaussianBlurEffect, DropShadowEffect, GlowEffect>;

struct LayerEffect final {
  EffectId effect_id;
  bool enabled{true};
  LayerEffectParameters parameters;

  friend bool operator==(const LayerEffect&, const LayerEffect&) = default;
};

struct LayerSnapshot final {
  LayerId layer_id;
  std::string display_name;
  TimeRangeNs active_range;
  Transform2D transform;
  BlendMode blend_mode{BlendMode::normal};
  std::vector<ScalarAnimation> animations;
  LayerContent content;
  std::vector<LayerMask> masks;
  std::vector<LayerEffect> effects;

  friend bool operator==(const LayerSnapshot&, const LayerSnapshot&) = default;
};

// EXP-002 hierarchy seed. A group is a semantic visual parent, never an NLE
// Track and never a second clock. The current pass-through proof deliberately
// requires opacity 1.0; isolated group compositing remains a later capability.
struct LayerGroupSnapshot final {
  LayerGroupId group_id;
  std::string display_name;
  TimeRangeNs active_range;
  Transform2D transform;
  std::vector<ScalarAnimation> animations;
  std::vector<VisualNodeRef> children;

  friend bool operator==(const LayerGroupSnapshot&,
                         const LayerGroupSnapshot&) = default;
};

struct CompositionSnapshot final {
  CompositionId composition_id;
  std::string display_name;
  CanvasExtent canvas;
  RationalRate frame_rate;
  ProjectTimeNs duration{0};
  std::vector<LayerSnapshot> layers;
  std::vector<LayerGroupSnapshot> groups;
  std::vector<VisualNodeRef> root_nodes;

  friend bool operator==(const CompositionSnapshot&, const CompositionSnapshot&) = default;
};

struct AffineTransform2D final {
  // x' = m00*x + m01*y + m02; y' = m10*x + m11*y + m12.
  double m00{1.0};
  double m01{0.0};
  double m02{0.0};
  double m10{0.0};
  double m11{1.0};
  double m12{0.0};

  friend bool operator==(const AffineTransform2D&,
                         const AffineTransform2D&) = default;
};

struct EvaluatedVisualLayer final {
  LayerId layer_id;
  std::string display_name;
  LayerContent content;
  std::vector<LayerMask> masks;
  std::vector<LayerEffect> effects;
  BlendMode blend_mode{BlendMode::normal};
  AffineTransform2D world_transform;
  double effective_opacity{1.0};
  std::optional<TextLayoutResult> text_layout;
  DerivedVisualBounds bounds;

  friend bool operator==(const EvaluatedVisualLayer&,
                         const EvaluatedVisualLayer&) = default;
};

struct EvaluatedVisualNodeTransform final {
  VisualNodeRef node;
  AffineTransform2D parent_world_transform;
  AffineTransform2D world_transform;
  double effective_opacity{1.0};

  friend bool operator==(const EvaluatedVisualNodeTransform&,
                         const EvaluatedVisualNodeTransform&) = default;
};

// Exact-time immutable evaluation shared by render probes, measurement and
// authoring commands. Transforms and Layers are produced by one hierarchy
// traversal, preventing consumers from observing independently evaluated
// geometry at the same Project time.
struct EvaluatedVisualScene final {
  std::vector<EvaluatedVisualNodeTransform> transforms;
  std::vector<EvaluatedVisualLayer> layers;

  friend bool operator==(const EvaluatedVisualScene&,
                         const EvaluatedVisualScene&) = default;
};

struct ProjectSnapshot final {
  ProjectId project_id;
  RevisionId revision_id;
  std::string display_name;
  std::optional<CompositionSnapshot> composition;

  friend bool operator==(const ProjectSnapshot&, const ProjectSnapshot&) = default;
};

struct CompositionValidation final {
  bool valid{false};
  std::string code;
  std::string message;
};

[[nodiscard]] CompositionValidation validate_composition(
    const CompositionSnapshot& composition);

// Authored TextBox geometry is centered in Layer-local pixels. These bounds
// are schema geometry only, not shaped glyph/logical/ink measurement.
[[nodiscard]] LocalRect text_box_bounds(const TextBox& box) noexcept;
[[nodiscard]] LocalRect text_box_content_bounds(const TextBox& box) noexcept;

[[nodiscard]] double evaluate_animated_property(
    const LayerSnapshot& layer,
    AnimatedProperty property,
    ProjectTimeNs composition_time) noexcept;

[[nodiscard]] double evaluate_animated_property(
    const LayerGroupSnapshot& group,
    AnimatedProperty property,
    ProjectTimeNs composition_time) noexcept;

[[nodiscard]] const LayerSnapshot* find_layer(
    const CompositionSnapshot& composition,
    const LayerId& layer_id) noexcept;

[[nodiscard]] const LayerGroupSnapshot* find_layer_group(
    const CompositionSnapshot& composition,
    const LayerGroupId& group_id) noexcept;

// Shared hierarchy query for command validation and read-only projections.
// A node is never considered its own ancestor.
[[nodiscard]] bool visual_node_is_ancestor(
    const CompositionSnapshot& composition,
    const VisualNodeRef& ancestor,
    const VisualNodeRef& descendant) noexcept;

// Legacy flat snapshots derive one root reference per layer. RFX2 snapshots
// persist an explicit root order.
[[nodiscard]] std::vector<VisualNodeRef> composition_root_nodes(
    const CompositionSnapshot& composition);

// One hierarchy traversal for exact-time parent/world transforms. Layer
// evaluation, measurement and authoring commands consume this shared result;
// no client reimplements hierarchy mathematics.
[[nodiscard]] std::vector<EvaluatedVisualNodeTransform>
evaluate_visual_node_transforms(const CompositionSnapshot& composition,
                                ProjectTimeNs composition_time);

[[nodiscard]] LocalRect transform_local_rect(
    const LocalRect& rect,
    const AffineTransform2D& transform) noexcept;

class TextLayoutPort;

[[nodiscard]] EvaluatedVisualScene evaluate_visual_scene(
    const CompositionSnapshot& composition,
    ProjectTimeNs composition_time);

[[nodiscard]] EvaluatedVisualScene evaluate_visual_scene(
    const CompositionSnapshot& composition,
    ProjectTimeNs composition_time,
    TextLayoutPort& text_layout_port);

// Topology/transform-only probe retained for portable hierarchy tests. For a
// Text Layer it has no shaped result and uses authored TextBox geometry; it is
// not an admitted render or measurement path.
[[nodiscard]] std::vector<EvaluatedVisualLayer> evaluate_visual_layers(
    const CompositionSnapshot& composition,
    ProjectTimeNs composition_time);

// Admitted EXP-006C evaluation path. Layout is evaluated once through the
// supplied engine-owned port, then the immutable result and all derived bounds
// travel with the same evaluated Layer snapshot consumed by preview/offline
// probes.
[[nodiscard]] std::vector<EvaluatedVisualLayer> evaluate_visual_layers(
    const CompositionSnapshot& composition,
    ProjectTimeNs composition_time,
    TextLayoutPort& text_layout_port);

}  // namespace refusion::core
