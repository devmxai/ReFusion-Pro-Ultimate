#include "refusion/runtime/render/RenderPlanCompiler.hpp"
#include "refusion/runtime/render/VisualOutputContract.hpp"

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
    return "visual-output-contract-test";
  }

  [[nodiscard]] refusion::core::TextLayoutOutcome layout(
      const refusion::core::TextLayoutRequest&) override {
    return {.diagnostic = refusion::core::TextLayoutDiagnostic{
                .code = "TEST-NO-TEXT",
                .message = "this visual-output fixture contains no text",
            }};
  }
};

[[nodiscard]] refusion::core::ProjectSnapshot project_fixture() {
  using namespace refusion::core;
  return ProjectSnapshot{
      .project_id = ProjectId{"prj_output_contract"},
      .revision_id = RevisionId{12},
      .display_name = "Output Contract",
      .composition = CompositionSnapshot{
          .composition_id = CompositionId{"cmp_main"},
          .display_name = "Main",
          .canvas = {.width_pixels = 1920, .height_pixels = 1080},
          .frame_rate = {.numerator = 60, .denominator = 1},
          .duration = 30'000'000'000ULL,
          .layers = {LayerSnapshot{
              .layer_id = LayerId{"lyr_shape"},
              .display_name = "Shape",
              .active_range = {.start = 0,
                               .duration = 30'000'000'000ULL},
              .transform = {.position_x = 960.0, .position_y = 540.0},
              .content = ShapeLayerContent{
                  .width = 640.0,
                  .height = 240.0,
                  .corner_radius = 48.0,
                  .fill = ColorRgba8{
                      .red = 85, .green = 65, .blue = 240, .alpha = 255},
              },
              .effects = {LayerEffect{
                  .effect_id = EffectId{"fx_glow"},
                  .parameters = GlowEffect{
                      .sigma = 16.0,
                      .color = {.red = 80,
                                .green = 210,
                                .blue = 255,
                                .alpha = 220},
                  },
              }},
          }},
          .root_nodes = {LayerId{"lyr_shape"}},
      },
  };
}

}  // namespace

int main() {
  using namespace refusion::runtime::render;
  NoTextLayout preview_layout;
  NoTextLayout export_layout;
  const auto program = compile_visual_render_program(project_fixture());

  const auto preview = prepare_visual_output_frame(
      VisualOutputConsumer::interactive_preview, program,
      5'000'000'000ULL, 41, preview_layout);
  const auto export_frame = prepare_visual_output_frame(
      VisualOutputConsumer::offline_export, program,
      5'000'000'000ULL, 0, export_layout);
  require(preview.valid() && export_frame.valid(),
          "both visual output frames must be valid");
  require(preview.plan.stamp.clock_epoch != export_frame.plan.stamp.clock_epoch,
          "Preview and Export schedulers must retain independent epochs");

  const auto matched =
      compare_visual_output_semantics(preview, export_frame);
  require(matched.matched,
          "Preview and Offline Export must resolve one visual meaning");
  require(matched.code == "RFX-VISUAL-OUTPUT-PARITY-MATCHED",
          "matched parity returned the wrong diagnostic code");
  require(matched.semantic_digest == preview.plan.semantic_digest &&
              matched.semantic_digest == export_frame.plan.semantic_digest,
          "parity receipt did not bind the shared RenderPlan digest");

  const auto later_export = prepare_visual_output_frame(
      VisualOutputConsumer::offline_export, program,
      6'000'000'000ULL, 0, export_layout);
  const auto time_mismatch =
      compare_visual_output_semantics(preview, later_export);
  require(!time_mismatch.matched &&
              time_mismatch.code == "RFX-VISUAL-OUTPUT-PARITY-003",
          "different ProjectTime samples must fail parity");

  const auto invalid_pair =
      compare_visual_output_semantics(preview, preview);
  require(!invalid_pair.matched &&
              invalid_pair.code == "RFX-VISUAL-OUTPUT-PARITY-002",
          "two Preview frames must not masquerade as Preview/Export proof");

  bool rejected_unknown_consumer = false;
  try {
    static_cast<void>(prepare_visual_output_frame(
        static_cast<VisualOutputConsumer>(255), program,
        5'000'000'000ULL, 0, preview_layout));
  } catch (const std::invalid_argument&) {
    rejected_unknown_consumer = true;
  }
  require(rejected_unknown_consumer,
          "unknown visual output consumers must fail closed");
}
