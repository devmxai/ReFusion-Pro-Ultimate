#include "refusion/core/CanonicalCoordinates.hpp"

#include <limits>
#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error(
        "canonical coordinates test requirement failed");
  }
}

}  // namespace

int main() {
  using namespace refusion::core;

  require(kAuthoredPixelQuantum == 0.0009765625);
  require(quantize_authored_pixel(12.123456) == 12.123046875);
  require(quantize_authored_pixel(-12.123456) == -12.123046875);
  require(quantize_authored_pixel(kAuthoredPixelQuantum * 0.5) ==
          kAuthoredPixelQuantum);
  require(quantize_authored_pixel(-kAuthoredPixelQuantum * 0.5) ==
          -kAuthoredPixelQuantum);
  require(quantize_authored_pixel(-0.0) == 0.0);
  require(is_quantized_authored_pixel(12.125));
  require(!is_quantized_authored_pixel(12.123456));

  const auto transformed = quantize_transform_pixels(Transform2D{
      .position_x = 100.123456,
      .position_y = 200.654321,
      .anchor_x = 10.333333,
      .anchor_y = 20.666667,
      .scale_x = 1.123456,
      .scale_y = 0.987654,
      .rotation_degrees = 13.123456,
      .opacity = 0.876543,
  });
  require(is_quantized_authored_pixel(transformed.position_x));
  require(is_quantized_authored_pixel(transformed.position_y));
  require(is_quantized_authored_pixel(transformed.anchor_x));
  require(is_quantized_authored_pixel(transformed.anchor_y));
  require(transformed.scale_x == 1.123456);
  require(transformed.scale_y == 0.987654);
  require(transformed.rotation_degrees == 13.123456);
  require(transformed.opacity == 0.876543);

  const auto infinity = std::numeric_limits<double>::infinity();
  require(quantize_authored_pixel(infinity) == infinity);
  require(!is_quantized_authored_pixel(infinity));
}
