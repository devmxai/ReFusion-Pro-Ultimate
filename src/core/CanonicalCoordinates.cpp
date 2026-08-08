#include "refusion/core/CanonicalCoordinates.hpp"

#include <cmath>

namespace refusion::core {

double quantize_authored_pixel(const double value) noexcept {
  if (!std::isfinite(value)) {
    return value;
  }
  const double scaled = value * kAuthoredSubpixelsPerPixel;
  if (!std::isfinite(scaled)) {
    return value;
  }
  const double integral = std::round(scaled);
  const double result = integral / kAuthoredSubpixelsPerPixel;
  return result == 0.0 ? 0.0 : result;
}

bool is_quantized_authored_pixel(const double value) noexcept {
  return std::isfinite(value) && quantize_authored_pixel(value) == value;
}

Transform2D quantize_transform_pixels(
    const Transform2D& transform) noexcept {
  auto result = transform;
  result.position_x = quantize_authored_pixel(result.position_x);
  result.position_y = quantize_authored_pixel(result.position_y);
  result.anchor_x = quantize_authored_pixel(result.anchor_x);
  result.anchor_y = quantize_authored_pixel(result.anchor_y);
  return result;
}

}  // namespace refusion::core
