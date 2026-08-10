#pragma once

#include "refusion/core/ColorContract.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace refusion::runtime::render {

using ProjectTimeNs = std::uint64_t;

struct ColorRgba8 final {
  std::uint8_t red{0};
  std::uint8_t green{0};
  std::uint8_t blue{0};
  std::uint8_t alpha{255};

  friend bool operator==(const ColorRgba8&, const ColorRgba8&) = default;
};

struct LocalRect final {
  double left{0.0};
  double top{0.0};
  double right{0.0};
  double bottom{0.0};

  [[nodiscard]] bool valid() const noexcept {
    return right >= left && bottom >= top;
  }

  friend bool operator==(const LocalRect&, const LocalRect&) = default;
};

struct AffineTransform2D final {
  double m00{1.0};
  double m01{0.0};
  double m02{0.0};
  double m10{0.0};
  double m11{1.0};
  double m12{0.0};

  friend bool operator==(const AffineTransform2D&,
                         const AffineTransform2D&) = default;
};

struct GradientStop final {
  double offset{0.0};
  ColorRgba8 color;

  friend bool operator==(const GradientStop&, const GradientStop&) = default;
};

struct LinearGradient final {
  double start_x{0.0};
  double start_y{0.0};
  double end_x{0.0};
  double end_y{0.0};
  std::vector<GradientStop> stops;

  friend bool operator==(const LinearGradient&, const LinearGradient&) = default;
};

struct RadialGradient final {
  double center_x{0.0};
  double center_y{0.0};
  double radius{0.0};
  std::vector<GradientStop> stops;

  friend bool operator==(const RadialGradient&, const RadialGradient&) = default;
};

using ShapeFill = std::variant<ColorRgba8, LinearGradient, RadialGradient>;

struct DrawShape final {
  double width{0.0};
  double height{0.0};
  double corner_radius{0.0};
  ShapeFill fill;
  double stroke_width{0.0};
  ColorRgba8 stroke_color;

  friend bool operator==(const DrawShape&, const DrawShape&) = default;
};

struct DrawText final {
  std::string cache_key;
  ColorRgba8 fill;
  std::optional<LocalRect> clip;

  friend bool operator==(const DrawText&, const DrawText&) = default;
};

struct DrawVideoFrame final {
  std::string video_clip_id;
  std::string media_source_id;
  std::string stream_id;
  std::int64_t source_time_value{0};
  std::int32_t source_time_scale{0};
  std::uint32_t source_width_pixels{0};
  std::uint32_t source_height_pixels{0};
  double destination_width{0.0};
  double destination_height{0.0};

  [[nodiscard]] bool valid() const noexcept {
    return !video_clip_id.empty() && !media_source_id.empty() &&
           !stream_id.empty() && source_time_scale > 0 &&
           source_width_pixels > 0 && source_height_pixels > 0 &&
           destination_width > 0.0 && destination_height > 0.0;
  }

  friend bool operator==(const DrawVideoFrame&, const DrawVideoFrame&) = default;
};

using DrawContent = std::variant<DrawShape, DrawText, DrawVideoFrame>;

struct RoundedRectMask final {
  std::string descriptor_id;
  std::string capability_id;
  std::uint32_t schema_version{0};
  bool inverted{false};
  double position_x{0.0};
  double position_y{0.0};
  double width{0.0};
  double height{0.0};
  double corner_radius{0.0};

  friend bool operator==(const RoundedRectMask&,
                         const RoundedRectMask&) = default;
};

struct GaussianBlur final {
  double sigma_x{0.0};
  double sigma_y{0.0};

  friend bool operator==(const GaussianBlur&, const GaussianBlur&) = default;
};

struct DropShadow final {
  double offset_x{0.0};
  double offset_y{0.0};
  double sigma_x{0.0};
  double sigma_y{0.0};
  ColorRgba8 color;

  friend bool operator==(const DropShadow&, const DropShadow&) = default;
};

struct Glow final {
  double sigma{0.0};
  ColorRgba8 color;

  friend bool operator==(const Glow&, const Glow&) = default;
};

using EffectParameters = std::variant<GaussianBlur, DropShadow, Glow>;

struct Effect final {
  std::string descriptor_id;
  std::string capability_id;
  std::uint32_t schema_version{0};
  EffectParameters parameters;

  friend bool operator==(const Effect&, const Effect&) = default;
};

enum class BlendMode : std::uint8_t {
  normal,
  multiply,
  screen,
  overlay,
};

struct DrawLayer final {
  std::string layer_id;
  AffineTransform2D world_transform;
  double effective_opacity{1.0};
  BlendMode blend_mode{BlendMode::normal};
  LocalRect isolation_bounds;
  std::vector<RoundedRectMask> masks;
  std::vector<Effect> effects;
  DrawContent content;

  friend bool operator==(const DrawLayer&, const DrawLayer&) = default;
};

struct EvaluationStamp final {
  std::string project_id;
  std::uint64_t revision{0};
  std::string composition_id;
  ProjectTimeNs project_time_ns{0};
  std::uint64_t clock_epoch{0};

  friend bool operator==(const EvaluationStamp&, const EvaluationStamp&) = default;
};

// Immutable, backend-neutral rendering truth. Native backends receive this
// value only; they never inspect Project/Layer/FX authoring types.
struct VisualRenderPlan final {
  EvaluationStamp stamp;
  std::uint32_t canvas_width_pixels{0};
  std::uint32_t canvas_height_pixels{0};
  core::VisualColorContract color_contract;
  std::string color_contract_digest;
  ColorRgba8 clear_color{.red = 0, .green = 0, .blue = 0, .alpha = 255};
  std::vector<DrawLayer> layers;
  std::string semantic_digest;

  [[nodiscard]] bool valid() const noexcept {
    return canvas_width_pixels != 0 && canvas_height_pixels != 0 &&
           !stamp.project_id.empty() && !stamp.composition_id.empty() &&
           core::is_desktop_v1_sdr_color_contract(color_contract) &&
           color_contract_digest ==
               core::desktop_v1_sdr_color_contract_digest() &&
           !semantic_digest.empty();
  }
};

// Pimpl keeps ProjectDocument out of every backend-facing header.
class VisualRenderProgram final {
 public:
  VisualRenderProgram() = default;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] const std::string& project_id() const noexcept;
  [[nodiscard]] std::uint64_t revision() const noexcept;
  [[nodiscard]] const std::string& composition_id() const noexcept;
  [[nodiscard]] std::uint32_t canvas_width_pixels() const noexcept;
  [[nodiscard]] std::uint32_t canvas_height_pixels() const noexcept;
  [[nodiscard]] const core::VisualColorContract& color_contract() const noexcept;
  [[nodiscard]] const std::string& color_contract_digest() const noexcept;

 private:
  struct State;
  explicit VisualRenderProgram(std::shared_ptr<const State> state);

  std::shared_ptr<const State> state_;

  friend struct VisualRenderProgramAccess;
};

}  // namespace refusion::runtime::render
