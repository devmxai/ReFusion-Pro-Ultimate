#pragma once

#include <cstdint>

namespace refusion::runtime::render {

enum class CanvasViewportMode : std::uint8_t {
  fit,
  custom_zoom,
};

enum class CanvasRasterQuality : std::uint8_t {
  full_resolution,
};

// Presentation-only state. It never enters project persistence, exact-time
// evaluation, or the VisualRenderPlan semantic digest.
struct CanvasViewportState final {
  CanvasViewportMode mode{CanvasViewportMode::fit};
  CanvasRasterQuality raster_quality{CanvasRasterQuality::full_resolution};
  double zoom{1.0};
  double pan_x_pixels{0.0};
  double pan_y_pixels{0.0};

  [[nodiscard]] bool valid() const noexcept;

  friend bool operator==(const CanvasViewportState&,
                         const CanvasViewportState&) = default;
};

struct ViewportMappingRequest final {
  std::uint32_t canvas_width_pixels{0};
  std::uint32_t canvas_height_pixels{0};
  std::uint32_t target_width_pixels{0};
  std::uint32_t target_height_pixels{0};
  CanvasViewportState viewport;

  [[nodiscard]] bool valid() const noexcept;
};

struct ViewportMapping final {
  double canvas_to_target_scale{0.0};
  double destination_left{0.0};
  double destination_top{0.0};
  double destination_width{0.0};
  double destination_height{0.0};
  bool requires_full_resolution_intermediate{false};

  [[nodiscard]] bool valid() const noexcept;
};

// Resolves one platform-neutral mapping in physical pixels. Fit always
// preserves aspect ratio. Custom zoom 1.0 is the pixel-true contract:
// one Composition pixel maps to one physical target pixel.
[[nodiscard]] ViewportMapping resolve_viewport_mapping(
    const ViewportMappingRequest& request);

}  // namespace refusion::runtime::render
