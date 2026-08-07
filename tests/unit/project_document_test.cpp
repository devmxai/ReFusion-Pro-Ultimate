#include "refusion/core/ProjectDocument.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("project document test requirement failed");
  }
}

[[nodiscard]] refusion::core::CompositionSnapshot composition_fixture() {
  using namespace refusion::core;
  std::vector<LayerSnapshot> layers;
  layers.push_back(LayerSnapshot{
      .layer_id = LayerId{"lyr_shape"},
      .display_name = "Animated Shape",
      .active_range = TimeRangeNs{.start = 0, .duration = 30'000'000'000},
      .transform = Transform2D{.position_x = 100.0, .position_y = 200.0},
      .animations = {
          ScalarAnimation{
              .property = AnimatedProperty::position_x,
              .keyframes = {
                  ScalarKeyframe{.time = 0, .value = 100.0},
                  ScalarKeyframe{.time = 15'000'000'000, .value = 700.0},
                  ScalarKeyframe{.time = 30'000'000'000, .value = 100.0},
              },
          },
      },
      .content = ShapeLayerContent{
          .width = 200.0,
          .height = 200.0,
          .corner_radius = 40.0,
          .fill = ColorRgba8{.red = 124, .green = 92, .blue = 255},
      },
  });
  return CompositionSnapshot{
      .composition_id = CompositionId{"cmp_reel"},
      .display_name = "Reel",
      .canvas = CanvasExtent{.width_pixels = 1080, .height_pixels = 1920},
      .frame_rate = RationalRate{.numerator = 30, .denominator = 1},
      .duration = 30'000'000'000,
      .layers = std::move(layers),
  };
}

}  // namespace

int main() {
  using namespace refusion::core;
  auto composition = composition_fixture();
  require(validate_composition(composition).valid);
  const auto& layer = composition.layers.front();
  require(std::abs(evaluate_animated_property(
                       layer, AnimatedProperty::position_x, 7'500'000'000) -
                   400.0) < 0.0001);
  require(std::abs(evaluate_animated_property(
                       layer, AnimatedProperty::position_y, 7'500'000'000) -
                   200.0) < 0.0001);
  require(layer.active_range.contains(29'999'999'999));
  require(!layer.active_range.contains(30'000'000'000));

  composition.layers.push_back(composition.layers.front());
  require(!validate_composition(composition).valid);
}
