#pragma once

#include "refusion/core/ProjectDocument.hpp"
#include "refusion/runtime/render/RenderPlanCompiler.hpp"

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
  layers.push_back(LayerSnapshot{
      .layer_id = LayerId{"lyr_test_text"},
      .display_name = "Test Text",
      .active_range = TimeRangeNs{.start = 0, .duration = 30'000'000'000},
      .transform = Transform2D{
          .position_x = 320.0,
          .position_y = 300.0,
      },
      .animations = {},
      .content = TextLayerContent{
          .text = "ReFusion",
          .font = FontIdentity{.family_name = "Arial"},
          .font_size = 36.0,
          .box = TextBox{.width = 300.0, .height = 60.0},
          .horizontal_alignment = TextHorizontalAlignment::center,
          .vertical_alignment = TextVerticalAlignment::center,
          .overflow = TextOverflowMode::clip,
          .fill = ColorRgba8{.red = 255,
                             .green = 255,
                             .blue = 255,
                             .alpha = 255},
      },
  });
  return CompositionSnapshot{
      .composition_id = CompositionId{"cmp_test"},
      .display_name = "Test Composition",
      .canvas = CanvasExtent{.width_pixels = 640, .height_pixels = 360},
      .frame_rate = RationalRate{.numerator = 30, .denominator = 1},
      .duration = 30'000'000'000,
      .layers = std::move(layers),
      .groups = {
          LayerGroupSnapshot{
              .group_id = LayerGroupId{"grp_test"},
              .display_name = "Test Group",
              .active_range = {.start = 0, .duration = 30'000'000'000},
              .transform = Transform2D{
                  .position_x = 320.0,
                  .position_y = 180.0,
                  .anchor_x = 320.0,
                  .anchor_y = 180.0,
              },
              .animations = {
                  ScalarAnimation{
                      .property = AnimatedProperty::rotation_degrees,
                      .keyframes = {
                          {.time = 0, .value = 0.0},
                          {.time = 15'000'000'000, .value = 15.0},
                          {.time = 30'000'000'000, .value = 0.0},
                      },
                  },
              },
              .children = {LayerId{"lyr_test_shape"},
                           LayerId{"lyr_test_text"}},
          },
      },
      .root_nodes = {LayerGroupId{"grp_test"}},
  };
}

[[nodiscard]] inline refusion::core::ProjectSnapshot test_project() {
  return refusion::core::ProjectSnapshot{
      .project_id = refusion::core::ProjectId{"prj_test"},
      .revision_id = refusion::core::RevisionId{1},
      .display_name = "Test Project",
      .composition = test_composition(),
  };
}

[[nodiscard]] inline refusion::runtime::render::VisualRenderProgram
test_render_program() {
  return refusion::runtime::render::compile_visual_render_program(
      test_project());
}
