#include "refusion/runtime/render/RenderPlanCompiler.hpp"

#include <stdexcept>
#include <string>

namespace {

void require(const bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

class NoTextLayout final : public refusion::core::TextLayoutPort {
 public:
  [[nodiscard]] std::string layout_engine_digest() const override {
    return "test-layout";
  }

  [[nodiscard]] refusion::core::TextLayoutOutcome layout(
      const refusion::core::TextLayoutRequest&) override {
    return {.diagnostic = refusion::core::TextLayoutDiagnostic{
                .code = "TEST-NO-TEXT",
                .message = "this fixture contains no text",
            }};
  }
};

[[nodiscard]] refusion::core::ProjectSnapshot project_fixture() {
  using namespace refusion::core;
  return ProjectSnapshot{
      .project_id = ProjectId{"prj_render_plan"},
      .revision_id = RevisionId{7},
      .display_name = "Render Plan",
      .composition = CompositionSnapshot{
          .composition_id = CompositionId{"cmp_main"},
          .display_name = "Main",
          .canvas = {.width_pixels = 1080, .height_pixels = 1920},
          .frame_rate = {.numerator = 60, .denominator = 1},
          .duration = 30'000'000'000ULL,
          .layers = {LayerSnapshot{
              .layer_id = LayerId{"lyr_shape"},
              .display_name = "Shape",
              .active_range = {.start = 0, .duration = 30'000'000'000ULL},
              .transform = {.position_x = 540.0, .position_y = 960.0},
              .blend_mode = BlendMode::screen,
              .content = ShapeLayerContent{
                  .width = 480.0,
                  .height = 180.0,
                  .corner_radius = 36.0,
                  .fill = LinearGradientFill{
                      .start_x = -240.0,
                      .start_y = 0.0,
                      .end_x = 240.0,
                      .end_y = 0.0,
                      .stops = {
                          {.offset = 0.0,
                           .color = {.red = 50, .green = 20, .blue = 240}},
                          {.offset = 1.0,
                           .color = {.red = 0, .green = 220, .blue = 255}},
                      }},
                  .stroke_width = 2.0,
                  .stroke_color = {.red = 255,
                                   .green = 255,
                                   .blue = 255,
                                   .alpha = 160}},
              .masks = {LayerMask{
                  .mask_id = MaskId{"mask_main"},
                  .geometry = {.position_x = 0.0,
                               .position_y = 0.0,
                               .width = 460.0,
                               .height = 160.0,
                               .corner_radius = 30.0}}},
              .effects = {
                  LayerEffect{
                      .effect_id = EffectId{"fx_blur"},
                      .parameters = GaussianBlurEffect{
                          .sigma_x = 2.0, .sigma_y = 3.0}},
                  LayerEffect{
                      .effect_id = EffectId{"fx_shadow"},
                      .parameters = DropShadowEffect{
                          .offset_x = 8.0,
                          .offset_y = 12.0,
                          .sigma_x = 6.0,
                          .sigma_y = 7.0,
                          .color = {.red = 0,
                                    .green = 0,
                                    .blue = 0,
                                    .alpha = 180}}},
                  LayerEffect{
                      .effect_id = EffectId{"fx_glow"},
                      .parameters = GlowEffect{
                          .sigma = 12.0,
                          .color = {.red = 80,
                                    .green = 120,
                                    .blue = 255,
                                    .alpha = 220}}},
              },
          }},
          .root_nodes = {LayerId{"lyr_shape"}},
      }};
}

}  // namespace

int main() {
  using namespace refusion::runtime::render;
  NoTextLayout layout;
  const auto program = compile_visual_render_program(project_fixture());
  require(program.valid(), "compiled program must be valid");
  require(program.project_id() == "prj_render_plan", "project stamp mismatch");
  require(program.revision() == 7, "revision stamp mismatch");
  require(program.composition_id() == "cmp_main", "composition stamp mismatch");

  const auto first = evaluate_visual_render_plan(
      program, 1'000'000'000ULL, 11, layout);
  const auto repeated = evaluate_visual_render_plan(
      program, 1'000'000'000ULL, 11, layout);
  require(first.valid(), "render plan must be valid");
  require(first.semantic_digest == repeated.semantic_digest,
          "same accepted revision/time must produce one digest");
  require(first.semantic_digest ==
              "rfx-render-plan-fnv1a64:fe9550afd21eaa35",
          "cross-toolchain digest fixture: " + first.semantic_digest);
  require(first.layers.size() == 1, "one visible layer expected");
  require(first.layers.front().effects.size() == 3,
          "ordered FX must remain embedded in their Layer operation");
  require(std::holds_alternative<GaussianBlur>(
              first.layers.front().effects[0].parameters) &&
              std::holds_alternative<DropShadow>(
                  first.layers.front().effects[1].parameters) &&
              std::holds_alternative<Glow>(
                  first.layers.front().effects[2].parameters),
          "Blur -> Shadow -> Glow operation order changed");
  require(first.layers.front().effects[2].capability_id ==
              "visual.fx.glow.v1",
          "RenderPlan did not bind the registered FX capability identity");
  require(first.layers.front().masks.front().capability_id ==
              "visual.mask.rounded_rect.v1",
          "RenderPlan did not bind the registered Mask capability identity");
  require(first.layers.front().masks.size() == 1,
          "Mask must remain embedded in its Layer operation");
  require(first.layers.front().isolation_bounds.valid(),
          "effect isolation must be deterministically bounded");

  const auto next_epoch = evaluate_visual_render_plan(
      program, 1'000'000'000ULL, 12, layout);
  require(first.semantic_digest == next_epoch.semantic_digest,
          "transport epoch must not redefine visual semantics");
  require(first.stamp.clock_epoch != next_epoch.stamp.clock_epoch,
          "transport epoch must remain explicit for stale-frame rejection");

  bool rejected = false;
  try {
    static_cast<void>(evaluate_visual_render_plan(
        program, 30'000'000'001ULL, 11, layout));
  } catch (const std::out_of_range&) {
    rejected = true;
  }
  require(rejected, "out-of-domain ProjectTime must fail closed");
}
