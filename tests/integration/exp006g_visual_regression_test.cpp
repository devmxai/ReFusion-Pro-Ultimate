#include "refusion/adapters/skia/SkiaTextLayout.hpp"

#include "refusion/core/ProjectClock.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/core/VisualMeasurement.hpp"

#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace refusion::core;

void require(const bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

[[nodiscard]] std::string fixture_source() {
  std::ifstream input(REFUSION_EXP006G_PROJECT_PATH, std::ios::binary);
  require(static_cast<bool>(input), "cannot read EXP-006G visual fixture");
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] const EvaluatedVisualLayer& evaluated_layer(
    const EvaluatedVisualScene& scene, const std::string& id) {
  for (const auto& layer : scene.layers) {
    if (layer.layer_id.value == id) return layer;
  }
  throw std::runtime_error("evaluated visual layer is missing");
}

[[nodiscard]] double center_x(const LocalRect& bounds) {
  return (bounds.left + bounds.right) * 0.5;
}

[[nodiscard]] double center_y(const LocalRect& bounds) {
  return (bounds.top + bounds.bottom) * 0.5;
}

[[nodiscard]] double width(const LocalRect& bounds) {
  return bounds.right - bounds.left;
}

[[nodiscard]] double height(const LocalRect& bounds) {
  return bounds.bottom - bounds.top;
}

[[nodiscard]] bool near(const double lhs, const double rhs,
                        const double tolerance = 0.25) {
  return std::abs(lhs - rhs) <= tolerance;
}

void measured_text_fx_and_seek_share_one_layout_contract() {
  const auto compiled = compile_project_rfx(fixture_source());
  require(compiled.succeeded(), "EXP-006G visual fixture does not compile");
  const auto& composition = *compiled.project->composition;
  auto layout = refusion::adapters::skia::create_skia_text_layout_port();
  require(layout != nullptr && !layout->layout_engine_digest().empty(),
          "Skia Text layout port is unavailable");
  const ProjectClockSpec clock{.duration_ns = composition.duration,
                               .frame_rate = composition.frame_rate,
                               .loop = false};

  for (const auto frame : std::vector<std::uint64_t>{0, 24, 60, 900, 1799}) {
    const auto time = clock.time_at_frame(frame);
    const auto first = evaluate_visual_scene(composition, time, *layout);
    const auto second = evaluate_visual_scene(composition, time, *layout);
    require(first == second,
            "Skia exact-time evaluation changed after arbitrary seek");
  }

  const auto settled_time = clock.time_at_frame(60);
  const auto scene = evaluate_visual_scene(composition, settled_time, *layout);
  const auto& title = evaluated_layer(scene, "lyr_title");
  const auto& label = evaluated_layer(scene, "lyr_subscribe_label");
  require(title.text_layout.has_value() && label.text_layout.has_value(),
          "Text layout result is missing from evaluated scene");
  require(title.text_layout->layout_engine_digest ==
              layout->layout_engine_digest() &&
              label.text_layout->layout_engine_digest ==
                  layout->layout_engine_digest(),
          "preview Text layout digest diverged from admitted port");
  require(!title.text_layout->resolved_font_digest.empty() &&
              !label.text_layout->resolved_font_digest.empty(),
          "resolved Font digest is missing");

  const auto measured =
      measure_visual_nodes(composition, settled_time, layout.get());
  require(measured.succeeded(), "offline visual measurement failed");
  require(measured.snapshot->layout_engine_digest ==
              layout->layout_engine_digest(),
          "offline measurement digest diverged from preview");
  const auto* title_measure =
      find_visual_measurement(*measured.snapshot, LayerId{"lyr_title"});
  const auto* body_measure = find_visual_measurement(
      *measured.snapshot, LayerId{"lyr_subscribe_body"});
  const auto* label_measure = find_visual_measurement(
      *measured.snapshot, LayerId{"lyr_subscribe_label"});
  require(title_measure != nullptr && title_measure->logical_world &&
              title_measure->ink_world && body_measure != nullptr &&
              label_measure != nullptr && label_measure->logical_world,
          "measured logical/ink geometry is incomplete");
  require(near(center_x(*title_measure->logical_world), 540.0) &&
              near(center_y(*title_measure->logical_world), 700.0),
          "Title paragraph is not centered in Composition pixels");
  require(near(center_x(*label_measure->logical_world),
               center_x(body_measure->geometry_world)) &&
              near(center_y(*label_measure->logical_world),
                   center_y(body_measure->geometry_world)),
          "Subscribe label is not measured at the body center");

  require(width(title.bounds.effect_local) >
                  width(title.text_layout->ink_bounds) &&
              height(title.bounds.effect_local) >
                  height(title.text_layout->ink_bounds),
          "Title Shadow/Glow did not expand owner-local effect bounds");

  auto without_fx = composition;
  for (auto& layer : without_fx.layers) {
    if (layer.layer_id == LayerId{"lyr_title"}) layer.effects.clear();
  }
  const auto no_fx_scene =
      evaluate_visual_scene(without_fx, settled_time, *layout);
  const auto& title_without_fx = evaluated_layer(no_fx_scene, "lyr_title");
  require(title_without_fx.text_layout == title.text_layout,
          "Shadow/Glow changed paragraph metrics");
  require(width(title.bounds.effect_local) >
                  width(title_without_fx.bounds.effect_local) &&
              height(title.bounds.effect_local) >
                  height(title_without_fx.bounds.effect_local),
          "owner-local FX did not expand the no-FX result");
}

}  // namespace

int main() {
  measured_text_fx_and_seek_share_one_layout_contract();
  return 0;
}
