#pragma once

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

struct RationalRate final {
  std::uint32_t numerator{0};
  std::uint32_t denominator{1};

  [[nodiscard]] bool valid() const noexcept;
};

struct TimeRangeNs final {
  ProjectTimeNs start{0};
  ProjectTimeNs duration{0};

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] ProjectTimeNs end() const noexcept;
  [[nodiscard]] bool contains(ProjectTimeNs time) const noexcept;
};

struct CanvasExtent final {
  std::uint32_t width_pixels{0};
  std::uint32_t height_pixels{0};

  [[nodiscard]] bool valid() const noexcept;
};

struct ColorRgba8 final {
  std::uint8_t red{0};
  std::uint8_t green{0};
  std::uint8_t blue{0};
  std::uint8_t alpha{255};
};

struct Transform2D final {
  double position_x{0.0};
  double position_y{0.0};
  double scale_x{1.0};
  double scale_y{1.0};
  double rotation_degrees{0.0};
  double opacity{1.0};
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
};

struct ScalarAnimation final {
  AnimatedProperty property{AnimatedProperty::position_x};
  std::vector<ScalarKeyframe> keyframes;
};

struct ShapeLayerContent final {
  double width{0.0};
  double height{0.0};
  double corner_radius{0.0};
  ColorRgba8 fill;
};

struct TextLayerContent final {
  std::string text;
  std::string font_family;
  double font_size{0.0};
  double layout_width{0.0};
  bool left_to_right{true};
  ColorRgba8 fill;
};

using LayerContent = std::variant<ShapeLayerContent, TextLayerContent>;

struct LayerSnapshot final {
  LayerId layer_id;
  std::string display_name;
  TimeRangeNs active_range;
  Transform2D transform;
  std::vector<ScalarAnimation> animations;
  LayerContent content;
};

struct CompositionSnapshot final {
  CompositionId composition_id;
  std::string display_name;
  CanvasExtent canvas;
  RationalRate frame_rate;
  ProjectTimeNs duration{0};
  std::vector<LayerSnapshot> layers;
};

struct ProjectSnapshot final {
  ProjectId project_id;
  RevisionId revision_id;
  std::string display_name;
  std::optional<CompositionSnapshot> composition;
};

struct CompositionValidation final {
  bool valid{false};
  std::string code;
  std::string message;
};

[[nodiscard]] CompositionValidation validate_composition(
    const CompositionSnapshot& composition);

[[nodiscard]] double evaluate_animated_property(
    const LayerSnapshot& layer,
    AnimatedProperty property,
    ProjectTimeNs composition_time) noexcept;

}  // namespace refusion::core
