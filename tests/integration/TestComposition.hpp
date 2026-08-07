#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <utility>
#include <vector>

[[nodiscard]] inline refusion::core::CompositionSnapshot test_composition() {
  using namespace refusion::core;
  std::vector<LayerSnapshot> layers;
  layers.push_back(LayerSnapshot{
      .layer_id = LayerId{"lyr_test_shape"},
      .display_name = "Test Shape",
      .active_range = TimeRangeNs{.start = 0, .duration = 30'000'000'000},
      .transform = Transform2D{
          .position_x = 320.0,
          .position_y = 180.0,
      },
      .animations = {},
      .content = ShapeLayerContent{
          .width = 160.0,
          .height = 160.0,
          .corner_radius = 24.0,
          .fill = ColorRgba8{.red = 124, .green = 92, .blue = 255, .alpha = 255},
      },
  });
  return CompositionSnapshot{
      .composition_id = CompositionId{"cmp_test"},
      .display_name = "Test Composition",
      .canvas = CanvasExtent{.width_pixels = 640, .height_pixels = 360},
      .frame_rate = RationalRate{.numerator = 30, .denominator = 1},
      .duration = 30'000'000'000,
      .layers = std::move(layers),
  };
}
