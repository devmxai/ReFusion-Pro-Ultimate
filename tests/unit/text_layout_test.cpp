#include "refusion/core/ProjectDocument.hpp"
#include "refusion/core/TextLayout.hpp"

#include <stdexcept>
#include <string>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("text layout test requirement failed");
  }
}

class FixedTextLayout final : public refusion::core::TextLayoutPort {
 public:
  [[nodiscard]] std::string layout_engine_digest() const override {
    return "fixed-layout-v1";
  }

  [[nodiscard]] refusion::core::TextLayoutOutcome layout(
      const refusion::core::TextLayoutRequest& request) override {
    using namespace refusion::core;
    ++calls;
    const auto key = text_layout_cache_key(request, layout_engine_digest());
    return TextLayoutOutcome{
        .result = TextLayoutResult{
            .layout_box = text_box_bounds(request.text.box),
            .content_box = text_box_content_bounds(request.text.box),
            .logical_bounds = {.left = -10.0,
                               .top = -5.0,
                               .right = 20.0,
                               .bottom = 10.0},
            .ink_bounds = {.left = -10.0,
                           .top = -5.0,
                           .right = 20.0,
                           .bottom = 10.0},
            .clipped_bounds = {.left = -10.0,
                               .top = -5.0,
                               .right = 20.0,
                               .bottom = 10.0},
            .lines = {{.utf8_length = request.text.text.size(),
                       .baseline_y = 5.0,
                       .logical_width = 30.0,
                       .ink_bounds = {.left = -10.0,
                                      .top = -5.0,
                                      .right = 20.0,
                                      .bottom = 10.0}}},
            .baselines = {5.0},
            .ascent = 10.0,
            .descent = 5.0,
            .font_qualified = false,
            .resolved_font_digest = "fixed-font",
            .layout_engine_digest = layout_engine_digest(),
            .cache_key = key,
        },
    };
  }

  int calls{0};
};

class RejectingTextLayout final : public refusion::core::TextLayoutPort {
 public:
  [[nodiscard]] std::string layout_engine_digest() const override {
    return "reject-layout-v1";
  }

  [[nodiscard]] refusion::core::TextLayoutOutcome layout(
      const refusion::core::TextLayoutRequest&) override {
    return refusion::core::TextLayoutOutcome{
        .diagnostic = refusion::core::TextLayoutDiagnostic{
            .code = "RFX-TEXT-LAYOUT-TEST-REJECT",
            .message = "deliberate rejection",
        },
    };
  }
};

[[nodiscard]] refusion::core::TextLayoutRequest request_fixture() {
  using namespace refusion::core;
  return TextLayoutRequest{
      .text = TextLayerContent{
          .text = "ReFusion",
          .font = FontIdentity{.family_name = "Arial"},
          .font_size = 48.0,
          .box = TextBox{.width = 300.0, .height = 100.0},
          .horizontal_alignment = TextHorizontalAlignment::center,
          .vertical_alignment = TextVerticalAlignment::center,
          .fill = ColorRgba8{.red = 255, .green = 255, .blue = 255},
      },
  };
}

}  // namespace

int main() {
  using namespace refusion::core;
  const auto request = request_fixture();
  const auto key = text_layout_cache_key(request, "fixed-layout-v1");
  require(key.starts_with("rfx-text-layout-fnv1a64:"));
  require(key == text_layout_cache_key(request, "fixed-layout-v1"));
  auto changed = request;
  changed.text.letter_spacing = 1.0;
  require(key != text_layout_cache_key(changed, "fixed-layout-v1"));
  require(key != text_layout_cache_key(request, "fixed-layout-v2"));

  CompositionSnapshot composition{
      .composition_id = CompositionId{"cmp_text_layout"},
      .display_name = "Text Layout",
      .canvas = {.width_pixels = 640, .height_pixels = 360},
      .frame_rate = {.numerator = 30, .denominator = 1},
      .duration = 1'000'000'000,
      .layers = {
          LayerSnapshot{
              .layer_id = LayerId{"lyr_text"},
              .display_name = "Text",
              .active_range = {.start = 0, .duration = 1'000'000'000},
              .transform = {.position_x = 100.0, .position_y = 200.0},
              .content = request.text,
              .effects = {
                  LayerEffect{
                      .effect_id = EffectId{"fx_glow"},
                      .parameters = GlowEffect{
                          .sigma = 2.0,
                          .color = ColorRgba8{.blue = 255, .alpha = 255},
                      },
                  },
              },
          },
      },
  };
  require(validate_composition(composition).valid);
  FixedTextLayout layout;
  const auto evaluated = evaluate_visual_layers(composition, 0, layout);
  require(layout.calls == 1);
  require(evaluated.size() == 1);
  require(evaluated.front().text_layout.has_value());
  require(evaluated.front().text_layout->cache_key == key);
  require(evaluated.front().bounds.geometry_local ==
          LocalRect{.left = -10.0, .top = -5.0,
                    .right = 20.0, .bottom = 10.0});
  require(evaluated.front().bounds.effect_local ==
          LocalRect{.left = -16.0, .top = -11.0,
                    .right = 26.0, .bottom = 16.0});
  require(evaluated.front().bounds.world ==
          LocalRect{.left = 84.0, .top = 189.0,
                    .right = 126.0, .bottom = 216.0});

  bool rejected = false;
  try {
    RejectingTextLayout rejecting;
    (void)evaluate_visual_layers(composition, 0, rejecting);
  } catch (const std::runtime_error& error) {
    rejected = std::string{error.what()}.starts_with(
        "RFX-TEXT-LAYOUT-TEST-REJECT:");
  }
  require(rejected);
}
