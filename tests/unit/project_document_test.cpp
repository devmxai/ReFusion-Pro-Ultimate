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

  auto invalid_scale_keyframe = composition_fixture();
  invalid_scale_keyframe.layers.front().animations = {
      ScalarAnimation{
          .property = AnimatedProperty::scale_x,
          .keyframes = {{.time = 0, .value = 1.0},
                        {.time = 1'000'000'000, .value = 0.0}},
      },
  };
  require(validate_composition(invalid_scale_keyframe).code ==
          "RFX-PROJECT-113");

  auto invalid_opacity_keyframe = composition_fixture();
  invalid_opacity_keyframe.layers.front().animations = {
      ScalarAnimation{
          .property = AnimatedProperty::opacity,
          .keyframes = {{.time = 0, .value = 0.5},
                        {.time = 1'000'000'000, .value = 1.01}},
      },
  };
  require(validate_composition(invalid_opacity_keyframe).code ==
          "RFX-PROJECT-113");

  const TextBox box{
      .width = 600.0,
      .height = 180.0,
      .padding_top = 10.0,
      .padding_right = 20.0,
      .padding_bottom = 30.0,
      .padding_left = 40.0,
  };
  require(text_box_bounds(box) ==
          LocalRect{.left = -300.0, .top = -90.0,
                    .right = 300.0, .bottom = 90.0});
  require(text_box_content_bounds(box) ==
          LocalRect{.left = -260.0, .top = -80.0,
                    .right = 280.0, .bottom = 60.0});

  auto text_composition = composition_fixture();
  text_composition.layers.front().content = TextLayerContent{
      .text = "مرحبا ReFusion",
      .font = FontIdentity{.family_name = "Noto Sans Arabic"},
      .font_size = 64.0,
      .box = box,
      .direction = ParagraphDirection::right_to_left,
      .horizontal_alignment = TextHorizontalAlignment::center,
      .vertical_alignment = TextVerticalAlignment::center,
      .wrap = TextWrapMode::word,
      .overflow = TextOverflowMode::clip,
      .line_height_ratio = 1.4,
      .letter_spacing = 0.0,
  };
  require(validate_composition(text_composition).valid);
  auto invalid_padding = text_composition;
  std::get<TextLayerContent>(invalid_padding.layers.front().content)
      .box.padding_left = 590.0;
  require(validate_composition(invalid_padding).code == "RFX-PROJECT-114");

  auto invalid_packaged_font = text_composition;
  std::get<TextLayerContent>(invalid_packaged_font.layers.front().content).font =
      FontIdentity{
          .source = FontSourceKind::packaged_asset,
          .family_name = "Inter",
          .asset_id = "font_inter_regular",
          .content_digest = "sha256:not-a-digest",
      };
  require(validate_composition(invalid_packaged_font).code ==
          "RFX-PROJECT-142");

  composition.layers.front().effects = {
      LayerEffect{
          .effect_id = EffectId{"fx_shadow"},
          .enabled = true,
          .parameters = DropShadowEffect{
              .offset_x = 8.0,
              .offset_y = 12.0,
              .sigma_x = 16.0,
              .sigma_y = 16.0,
              .color = ColorRgba8{.alpha = 128},
          },
      },
      LayerEffect{
          .effect_id = EffectId{"fx_glow"},
          .enabled = false,
          .parameters = GlowEffect{
              .sigma = 20.0,
              .color = ColorRgba8{.red = 124, .green = 92, .blue = 255},
          },
      },
  };
  require(validate_composition(composition).valid);
  require(evaluate_visual_layers(composition, 1).front().effects ==
          composition.layers.front().effects);

  auto duplicate_effect = composition;
  duplicate_effect.layers.front().effects.push_back(
      duplicate_effect.layers.front().effects.front());
  require(validate_composition(duplicate_effect).code == "RFX-PROJECT-131");

  auto invalid_blur = composition;
  invalid_blur.layers.front().effects = {
      LayerEffect{
          .effect_id = EffectId{"fx_invalid"},
          .parameters = GaussianBlurEffect{.sigma_x = 300.0, .sigma_y = 2.0},
      },
  };
  require(validate_composition(invalid_blur).code == "RFX-PROJECT-132");

  auto gradient_composition = composition;
  std::get<ShapeLayerContent>(
      gradient_composition.layers.front().content).fill =
      LinearGradientFill{
          .start_x = -100.0,
          .start_y = 0.0,
          .end_x = 100.0,
          .end_y = 0.0,
          .stops = {
              {.offset = 0.0, .color = ColorRgba8{.red = 20}},
              {.offset = 1.0, .color = ColorRgba8{.blue = 255}},
          },
      };
  require(validate_composition(gradient_composition).valid);
  auto invalid_gradient = gradient_composition;
  std::get<LinearGradientFill>(
      std::get<ShapeLayerContent>(invalid_gradient.layers.front().content).fill)
      .stops.back().offset = 0.0;
  require(validate_composition(invalid_gradient).code == "RFX-PROJECT-135");

  auto masked = composition;
  masked.layers.front().masks = {
      LayerMask{
          .mask_id = MaskId{"mask_round"},
          .enabled = true,
          .geometry = RoundedRectMask{
              .width = 180.0,
              .height = 180.0,
              .corner_radius = 32.0,
          },
      },
  };
  require(validate_composition(masked).valid);
  require(evaluate_visual_layers(masked, 1).front().masks ==
          masked.layers.front().masks);
  auto invalid_mask = masked;
  invalid_mask.layers.front().masks.front().geometry.width = 0.0;
  require(validate_composition(invalid_mask).code == "RFX-PROJECT-140");

  composition.groups.push_back(LayerGroupSnapshot{
      .group_id = LayerGroupId{"grp_motion"},
      .display_name = "Motion Group",
      .active_range = {.start = 0, .duration = 30'000'000'000},
      .transform = Transform2D{.position_x = 10.0, .position_y = 20.0},
      .animations = {
          ScalarAnimation{
              .property = AnimatedProperty::position_x,
              .keyframes = {
                  {.time = 0, .value = 10.0},
                  {.time = 15'000'000'000, .value = 110.0},
                  {.time = 30'000'000'000, .value = 10.0},
              },
          },
      },
      .children = {LayerId{"lyr_shape"}},
  });
  composition.root_nodes = {LayerGroupId{"grp_motion"}};
  require(validate_composition(composition).valid);
  const auto evaluated = evaluate_visual_layers(composition, 15'000'000'000);
  require(evaluated.size() == 1);
  require(evaluated.front().layer_id == LayerId{"lyr_shape"});
  require(std::abs(evaluated.front().world_transform.m02 - 810.0) < 0.0001);
  require(std::abs(evaluated.front().world_transform.m12 - 220.0) < 0.0001);

  auto invalid_opacity = composition;
  invalid_opacity.groups.front().transform.opacity = 0.5;
  require(validate_composition(invalid_opacity).code == "RFX-PROJECT-121");

  auto unknown_child = composition;
  unknown_child.groups.front().children = {LayerId{"lyr_missing"}};
  require(validate_composition(unknown_child).code == "RFX-PROJECT-123");

  auto cycle = composition;
  cycle.groups.push_back(LayerGroupSnapshot{
      .group_id = LayerGroupId{"grp_cycle"},
      .display_name = "Cycle",
      .active_range = {.start = 0, .duration = 30'000'000'000},
      .children = {LayerGroupId{"grp_motion"}},
  });
  cycle.groups.front().children = {LayerGroupId{"grp_cycle"}};
  require(validate_composition(cycle).code == "RFX-PROJECT-127");

  composition.layers.push_back(composition.layers.front());
  require(!validate_composition(composition).valid);
}
