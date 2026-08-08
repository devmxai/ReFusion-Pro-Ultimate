#include "refusion/core/AgentIntrospection.hpp"
#include "refusion/core/ProjectClock.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/core/SemanticAuthoring.hpp"

#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace {

using namespace refusion::core;

void require(const bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

[[nodiscard]] std::string fixture_source() {
  std::ifstream input(REFUSION_EXP006G_PROJECT_PATH, std::ios::binary);
  require(static_cast<bool>(input), "cannot read EXP-006G fixture");
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] const EvaluatedVisualLayer& evaluated_layer(
    const EvaluatedVisualScene& scene, const std::string& id) {
  for (const auto& layer : scene.layers) {
    if (layer.layer_id.value == id) return layer;
  }
  throw std::runtime_error("evaluated fixture layer is missing");
}

void topology_ownership_and_round_trip_are_stable() {
  const auto compiled = compile_project_rfx(fixture_source());
  require(compiled.succeeded(), "EXP-006G fixture does not compile");
  const auto& project = *compiled.project;
  require(project.project_id == ProjectId{"prj_exp006g_reels"},
          "fixture project ID changed");
  require(project.revision_id == RevisionId{1}, "fixture revision changed");
  const auto& composition = *project.composition;
  require(composition.canvas == CanvasExtent{1080, 1920},
          "fixture Canvas changed");
  require(composition.frame_rate == RationalRate{60, 1},
          "fixture frame rate changed");
  require(composition.duration == 30'000'000'000,
          "fixture duration is not 30 seconds");
  require(composition.layers.size() == 7 && composition.groups.size() == 2,
          "fixture node count changed");
  require(composition_root_nodes(composition) ==
              std::vector<VisualNodeRef>{LayerGroupId{"grp_background"},
                                         LayerId{"lyr_title"},
                                         LayerGroupId{"grp_subscribe"}},
          "fixture roots changed");

  const auto* background =
      find_layer_group(composition, LayerGroupId{"grp_background"});
  require(background != nullptr, "Background Group is missing");
  require(background->children ==
              std::vector<VisualNodeRef>{LayerId{"lyr_bg_base"},
                                         LayerId{"lyr_bg_violet"},
                                         LayerId{"lyr_bg_cyan"}},
          "Background child order changed");
  const auto* subscribe =
      find_layer_group(composition, LayerGroupId{"grp_subscribe"});
  require(subscribe != nullptr && subscribe->children.size() == 3,
          "Subscribe Group is missing or flat");

  const auto* title = find_layer(composition, LayerId{"lyr_title"});
  require(title != nullptr, "Title Layer is missing");
  require(title->effects.size() == 2,
          "Title Shadow/Glow must be owner-local effects");
  require(std::holds_alternative<DropShadowEffect>(
              title->effects[0].parameters) &&
              std::holds_alternative<GlowEffect>(title->effects[1].parameters),
          "Title effect order changed");
  require(semantic_authoring_lint(composition).empty(),
          "sanitized fixture triggers semantic guardrail lint");

  const auto outline = agent_project_outline(project);
  const auto* title_address =
      find_agent_visual_node(outline, LayerId{"lyr_title"});
  require(title_address != nullptr && title_address->parent_path.empty(),
          "Title semantic address changed");
  require(title_address->timeline_row == 1,
          "Title Timeline row changed");
  require(title_address->owned_effects ==
              std::vector<EffectId>{EffectId{"fx_title_shadow"},
                                    EffectId{"fx_title_glow"}},
          "Agent projection lost Title FX ownership");

  const auto canonical = serialize_project_rfx(project);
  const auto reopened = compile_project_rfx(canonical);
  require(reopened.succeeded(), "canonical fixture failed to reopen");
  require(*reopened.project == project, "save/reopen changed fixture semantics");
  require(project_snapshot_digest(*reopened.project) ==
              project_snapshot_digest(project),
          "save/reopen changed fixture digest");
}

void arbitrary_seek_order_is_deterministic() {
  const auto project = *compile_project_rfx(fixture_source()).project;
  const auto& composition = *project.composition;
  const ProjectClockSpec clock{.duration_ns = composition.duration,
                               .frame_rate = composition.frame_rate,
                               .loop = false};
  require(clock.frame_count() == 1800, "fixture Clock frame count changed");

  const std::vector<std::uint64_t> seek_frames{
      0, 900, 60, 1799, 24, 900, 0, 60, 1799, 24};
  std::unordered_map<std::uint64_t, EvaluatedVisualScene> first_result;
  for (const auto frame : seek_frames) {
    const auto time = clock.time_at_frame(frame);
    const auto scene = evaluate_visual_scene(composition, time);
    const auto [stored, inserted] = first_result.emplace(frame, scene);
    if (!inserted) {
      require(stored->second == scene,
              "seek order changed exact-time evaluated scene");
    }
  }

  const auto at_start = evaluate_visual_scene(composition, clock.time_at_frame(0));
  const auto at_settled =
      evaluate_visual_scene(composition, clock.time_at_frame(60));
  require(std::abs(evaluated_layer(at_start, "lyr_title").effective_opacity) <
              0.0001,
          "Title start opacity changed");
  require(std::abs(evaluated_layer(at_settled, "lyr_title")
                       .effective_opacity -
                   1.0) < 0.0001,
          "Title settled opacity changed");
  require(at_settled == evaluate_visual_scene(composition,
                                               clock.time_at_frame(60)),
          "repeated settled seek is not deterministic");
}

}  // namespace

int main() {
  topology_ownership_and_round_trip_are_stable();
  arbitrary_seek_order_is_deterministic();
  return 0;
}
