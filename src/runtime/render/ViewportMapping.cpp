#include "refusion/runtime/render/ViewportMapping.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace refusion::runtime::render {
namespace {

constexpr double kMinimumZoom = 1.0 / 64.0;
constexpr double kMaximumZoom = 64.0;

[[nodiscard]] bool finite(const double value) noexcept {
  return std::isfinite(value);
}

}  // namespace

bool CanvasViewportState::valid() const noexcept {
  const bool known_mode = mode == CanvasViewportMode::fit ||
                          mode == CanvasViewportMode::custom_zoom;
  return known_mode && raster_quality == CanvasRasterQuality::full_resolution &&
         finite(zoom) && zoom >= kMinimumZoom && zoom <= kMaximumZoom &&
         finite(pan_x_pixels) && finite(pan_y_pixels);
}

bool ViewportMappingRequest::valid() const noexcept {
  return canvas_width_pixels != 0 && canvas_height_pixels != 0 &&
         target_width_pixels != 0 && target_height_pixels != 0 &&
         viewport.valid();
}

bool ViewportMapping::valid() const noexcept {
  return finite(canvas_to_target_scale) && canvas_to_target_scale > 0.0 &&
         finite(destination_left) && finite(destination_top) &&
         finite(destination_width) && destination_width > 0.0 &&
         finite(destination_height) && destination_height > 0.0;
}

ViewportMapping resolve_viewport_mapping(
    const ViewportMappingRequest& request) {
  if (!request.valid()) {
    throw std::invalid_argument(
        "RFX-VIEWPORT-MAPPING-001: invalid Canvas viewport mapping request");
  }

  const double canvas_width = static_cast<double>(request.canvas_width_pixels);
  const double canvas_height =
      static_cast<double>(request.canvas_height_pixels);
  const double target_width = static_cast<double>(request.target_width_pixels);
  const double target_height =
      static_cast<double>(request.target_height_pixels);
  const double scale =
      request.viewport.mode == CanvasViewportMode::fit
          ? std::min(target_width / canvas_width, target_height / canvas_height)
          : request.viewport.zoom;
  const double destination_width = canvas_width * scale;
  const double destination_height = canvas_height * scale;
  const ViewportMapping mapping{
      .canvas_to_target_scale = scale,
      .destination_left = (target_width - destination_width) * 0.5 +
                          request.viewport.pan_x_pixels,
      .destination_top = (target_height - destination_height) * 0.5 +
                         request.viewport.pan_y_pixels,
      .destination_width = destination_width,
      .destination_height = destination_height,
      .requires_full_resolution_intermediate = scale < 1.0,
  };
  if (!mapping.valid()) {
    throw std::runtime_error(
        "RFX-VIEWPORT-MAPPING-002: Canvas viewport mapping overflowed");
  }
  return mapping;
}

}  // namespace refusion::runtime::render
