#include "refusion/core/VisualPropertyRegistry.hpp"

#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("visual property registry test requirement failed");
  }
}

[[nodiscard]] refusion::core::CompositionSnapshot fixture() {
  using namespace refusion::core;
  return CompositionSnapshot{
      .composition_id = CompositionId{"cmp_registry"},
      .display_name = "Registry",
      .canvas = {.width_pixels = 1080, .height_pixels = 1920},
      .frame_rate = {.numerator = 30, .denominator = 1},
      .duration = 30'000'000'000,
      .layers = {
          LayerSnapshot{
              .layer_id = LayerId{"lyr_shape"},
              .display_name = "Shape",
              .active_range = {.start = 0, .duration = 30'000'000'000},
              .transform = {.position_x = 540.0, .position_y = 960.0},
              .content = ShapeLayerContent{
                  .width = 800.0,
                  .height = 500.0,
                  .corner_radius = 40.0,
                  .fill = ColorRgba8{.red = 124, .green = 92, .blue = 255},
              },
          },
          LayerSnapshot{
              .layer_id = LayerId{"lyr_text"},
              .display_name = "Text",
              .active_range = {.start = 0, .duration = 30'000'000'000},
              .content = TextLayerContent{
                  .text = "ReFusion",
                  .font = FontIdentity{.family_name = "Arial"},
                  .font_size = 72.0,
                  .box = TextBox{.width = 900.0, .height = 100.0},
              },
          },
      },
      .groups = {
          LayerGroupSnapshot{
              .group_id = LayerGroupId{"grp_main"},
              .display_name = "Main Group",
              .active_range = {.start = 0, .duration = 30'000'000'000},
              .children = {LayerId{"lyr_shape"}, LayerId{"lyr_text"}},
          },
      },
      .root_nodes = {LayerGroupId{"grp_main"}},
  };
}

}  // namespace

int main() {
  using namespace refusion::core;
  require(visual_property_descriptors().size() == 36);
  require(visual_property_registry_digest().starts_with("rfx-vp-fnv1a64:"));
  const auto registry_markdown = visual_property_registry_markdown();
  require(registry_markdown.find(visual_property_registry_digest()) !=
          std::string::npos);
  require(registry_markdown.find("text.box.width") != std::string::npos);

  auto composition = fixture();
  require(validate_composition(composition).valid);
  const auto shape_properties = inspect_visual_properties(
      composition, LayerId{"lyr_shape"});
  const auto text_properties = inspect_visual_properties(
      composition, LayerId{"lyr_text"});
  const auto group_properties = inspect_visual_properties(
      composition, LayerGroupId{"grp_main"});
  require(shape_properties.size() == 16);
  require(text_properties.size() == 30);
  require(group_properties.size() == 8);

  const ColorRgba8 updated_fill{
      .red = 10,
      .green = 20,
      .blue = 30,
      .alpha = 200,
  };
  require(set_visual_property(composition, LayerId{"lyr_shape"},
                              VisualPropertyId{"shape.fill"},
                              updated_fill)
              .valid);
  require(std::get<ColorRgba8>(
              std::get<ShapeLayerContent>(composition.layers.front().content)
                  .fill) == updated_fill);

  require(set_visual_property(composition, LayerId{"lyr_text"},
                              VisualPropertyId{"text.value"},
                              std::string{"Agent-native"})
              .valid);
  require(std::get<TextLayerContent>(composition.layers.back().content).text ==
          "Agent-native");

  require(set_visual_property(composition, LayerId{"lyr_text"},
                              VisualPropertyId{"text.box.width"}, 720.0)
              .valid);
  require(std::get<TextLayerContent>(composition.layers.back().content)
              .box.width == 720.0);

  require(set_visual_property(composition, LayerId{"lyr_text"},
                              VisualPropertyId{"text.box.width"},
                              720.0006)
              .valid);
  require(std::get<TextLayerContent>(composition.layers.back().content)
              .box.width == 720.0009765625);

  const auto readonly_font_source = set_visual_property(
      composition, LayerId{"lyr_text"},
      VisualPropertyId{"text.font.source"}, std::string{"packaged_asset"});
  require(!readonly_font_source.valid);
  require(readonly_font_source.code == "RFX-PROPERTY-READONLY-400");

  require(set_visual_property(composition, LayerGroupId{"grp_main"},
                              VisualPropertyId{"transform.position.x"}, 600.0)
              .valid);
  require(composition.groups.front().transform.position_x == 600.0);

  const auto wrong_owner = set_visual_property(
      composition, LayerGroupId{"grp_main"}, VisualPropertyId{"shape.fill"},
      updated_fill);
  require(!wrong_owner.valid);
  require(wrong_owner.code == "RFX-PROPERTY-OWNER-400");

  const auto wrong_type = set_visual_property(
      composition, LayerId{"lyr_shape"}, VisualPropertyId{"shape.width"},
      std::string{"wide"});
  require(!wrong_type.valid);
  require(wrong_type.code == "RFX-PROPERTY-TYPE-400");

  const auto invalid_value = set_visual_property(
      composition, LayerId{"lyr_shape"},
      VisualPropertyId{"transform.scale.x"}, 0.0);
  require(!invalid_value.valid);
  require(invalid_value.code == "RFX-PROJECT-110");

  const auto missing_property = set_visual_property(
      composition, LayerId{"lyr_shape"}, VisualPropertyId{"fx.unknown"}, 1.0);
  require(!missing_property.valid);
  require(missing_property.code == "RFX-PROPERTY-404");
}
