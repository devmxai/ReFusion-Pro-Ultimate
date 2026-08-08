#include "refusion/core/ProjectClock.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/runtime/render/RenderPlanCompiler.hpp"

#include <array>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(const bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] std::string fixture_source() {
  std::ifstream input(REFUSION_XPLAT_VISUAL_FIXTURE_PATH, std::ios::binary);
  require(static_cast<bool>(input), "cannot read xplat visual fixture");
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] std::vector<std::pair<std::uint64_t, std::string>>
expected_receipts() {
  std::ifstream input(REFUSION_XPLAT_VISUAL_RECEIPT_PATH);
  require(static_cast<bool>(input), "cannot read xplat visual receipts");
  std::vector<std::pair<std::uint64_t, std::string>> result;
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line.front() == '#') {
      continue;
    }
    std::istringstream record(line);
    std::uint64_t frame = 0;
    std::string digest;
    require(static_cast<bool>(record >> frame >> digest),
            "invalid xplat visual receipt record");
    result.emplace_back(frame, std::move(digest));
  }
  return result;
}

class DeterministicTextLayout final : public refusion::core::TextLayoutPort {
 public:
  [[nodiscard]] std::string layout_engine_digest() const override {
    return "rfx-xplat-layout-stub-v1";
  }

  [[nodiscard]] refusion::core::TextLayoutOutcome layout(
      const refusion::core::TextLayoutRequest& request) override {
    const auto box = refusion::core::text_box_bounds(request.text.box);
    const auto content =
        refusion::core::text_box_content_bounds(request.text.box);
    const refusion::core::LocalRect logical{
        .left = -180.0, .top = -22.0, .right = 180.0, .bottom = 22.0};
    return {.result = refusion::core::TextLayoutResult{
                .layout_box = box,
                .content_box = content,
                .logical_bounds = logical,
                .ink_bounds = logical,
                .clipped_bounds = logical,
                .lines = {{.utf8_start = 0,
                           .utf8_length = request.text.text.size(),
                           .origin_x = -180.0,
                           .baseline_y = 12.0,
                           .logical_width = 360.0,
                           .ink_bounds = logical}},
                .baselines = {12.0},
                .ascent = 32.0,
                .descent = 10.0,
                .leading = 4.0,
                .font_qualified = true,
                .resolved_font_digest = "sha256:xplat-layout-stub-v1",
                .layout_engine_digest = layout_engine_digest(),
                .cache_key = "xplat-text-cache-v1",
            }};
  }
};

class HostileNumericPunctuation final : public std::numpunct<char> {
 protected:
  [[nodiscard]] char do_decimal_point() const override { return ','; }
  [[nodiscard]] char do_thousands_sep() const override { return '_'; }
  [[nodiscard]] std::string do_grouping() const override { return "\3"; }
};

[[nodiscard]] std::string operation_summary(
    const refusion::runtime::render::VisualRenderPlan& plan) {
  std::ostringstream result;
  result << plan.semantic_digest;
  for (const auto& layer : plan.layers) {
    result << "\n" << layer.layer_id << ':'
           << layer.content.index() << ':'
           << static_cast<unsigned>(layer.blend_mode) << ':'
           << layer.masks.size() << ':' << layer.effects.size();
  }
  return result.str();
}

}  // namespace

int main() {
  using namespace refusion;
  const auto compiled = core::compile_project_rfx(fixture_source());
  require(compiled.succeeded(), "xplat visual Project.rfx did not compile");
  const auto program =
      runtime::render::compile_visual_render_program(*compiled.project);
  DeterministicTextLayout layout;
  const auto& composition = *compiled.project->composition;
  const core::ProjectClockSpec clock{
      .duration_ns = composition.duration,
      .frame_rate = composition.frame_rate,
      .loop = false,
  };

  constexpr std::array<std::uint64_t, 4> frames{0, 30, 60, 119};
  const auto expected = expected_receipts();
  require(expected.size() == frames.size(),
          "xplat visual receipt count changed");
  std::vector<runtime::render::VisualRenderPlan> plans;
  plans.reserve(frames.size());
  for (std::size_t index = 0; index < frames.size(); ++index) {
    auto plan = runtime::render::evaluate_visual_render_plan(
        program, clock.time_at_frame(frames[index]), 77, layout);
    require(expected[index].first == frames[index],
            "xplat visual receipt frame order changed");
    plans.push_back(std::move(plan));
  }

  if (const char* receipt_path =
          std::getenv("REFUSION_XPLAT_RENDER_RECEIPT_OUTPUT");
      receipt_path != nullptr && receipt_path[0] != '\0') {
    std::ofstream output(receipt_path, std::ios::trunc);
    require(static_cast<bool>(output),
            "cannot open requested xplat RenderPlan receipt");
    output << "# schema=refusion.xplat-render-plan-conformance.v2\n"
           << "# color_contract="
           << core::desktop_v1_sdr_color_contract_digest() << "\n"
           << "# frame semantic_digest\n";
    for (std::size_t index = 0; index < frames.size(); ++index) {
      output << frames[index] << ' ' << plans[index].semantic_digest << '\n';
    }
    require(static_cast<bool>(output),
            "cannot write requested xplat RenderPlan receipt");
  }
  for (std::size_t index = 0; index < frames.size(); ++index) {
    require(plans[index].semantic_digest == expected[index].second,
            "xplat receipt mismatch at frame " +
                std::to_string(frames[index]) + ":\n" +
                operation_summary(plans[index]));
  }

  const auto original_locale = std::locale();
  std::locale::global(
      std::locale(original_locale, new HostileNumericPunctuation));
  const auto hostile_plan = runtime::render::evaluate_visual_render_plan(
      program, clock.time_at_frame(60), 91, layout);
  std::locale::global(original_locale);
  require(hostile_plan.semantic_digest == expected[2].second,
          "host locale changed RenderPlan semantic digest");

  const auto& plan = plans.front();
  require(plan.color_contract ==
              core::desktop_v1_sdr_color_contract() &&
              plan.color_contract_digest ==
                  core::desktop_v1_sdr_color_contract_digest(),
          "cross-toolchain plan did not bind the color contract");
  require(plan.layers.size() == 4, "root/group operation order changed");
  require(plan.layers[0].layer_id == "lyr_solid_normal" &&
              plan.layers[1].layer_id == "lyr_linear_multiply" &&
              plan.layers[2].layer_id == "lyr_radial_screen" &&
              plan.layers[3].layer_id == "lyr_text_overlay",
          "Layer/Group traversal order changed");
  require(std::holds_alternative<runtime::render::ColorRgba8>(
              std::get<runtime::render::DrawShape>(plan.layers[0].content)
                  .fill) &&
              std::holds_alternative<runtime::render::LinearGradient>(
                  std::get<runtime::render::DrawShape>(plan.layers[1].content)
                      .fill) &&
              std::holds_alternative<runtime::render::RadialGradient>(
                  std::get<runtime::render::DrawShape>(plan.layers[2].content)
                      .fill) &&
              std::holds_alternative<runtime::render::DrawText>(
                  plan.layers[3].content),
          "solid/linear/radial/text lowering changed");
  require(plan.layers[1].masks.size() == 1 &&
              !plan.layers[1].masks.front().inverted &&
              plan.layers[2].masks.size() == 1 &&
              plan.layers[2].masks.front().inverted,
          "normal/inverted Mask lowering changed");
  require(plan.layers[1].effects.size() == 3 &&
              std::holds_alternative<runtime::render::GaussianBlur>(
                  plan.layers[1].effects[0].parameters) &&
              std::holds_alternative<runtime::render::DropShadow>(
                  plan.layers[1].effects[1].parameters) &&
              std::holds_alternative<runtime::render::Glow>(
                  plan.layers[1].effects[2].parameters),
          "Blur -> Shadow -> Glow ordering changed");
  require(plan.layers[1].effects[2].capability_id ==
              "visual.fx.glow.v1" &&
              plan.layers[1].masks.front().capability_id ==
                  "visual.mask.rounded_rect.v1",
          "registered contribution identity was not lowered into RenderPlan");
}
