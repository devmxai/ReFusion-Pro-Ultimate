#include "refusion/core/AgentIntrospection.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace {

using namespace refusion::core;

void require(const bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] LayerSnapshot shape(std::string id, std::string name,
                                  const double x) {
  return LayerSnapshot{
      .layer_id = LayerId{std::move(id)},
      .display_name = std::move(name),
      .active_range = {.start = 0, .duration = 3'000'000'000},
      .transform = {.position_x = x, .position_y = 540.0},
      .content = ShapeLayerContent{
          .width = 320.0,
          .height = 180.0,
          .corner_radius = 24.0,
          .fill = ColorRgba8{.red = 30, .green = 48, .blue = 96},
      },
  };
}

[[nodiscard]] ProjectSnapshot fixture(const std::uint64_t revision = 7) {
  auto background = shape("lyr_background", "Background", 960.0);
  background.masks.push_back(LayerMask{
      .mask_id = MaskId{"mask_background"},
      .geometry = {.width = 300.0, .height = 160.0, .corner_radius = 20.0},
  });
  background.effects.push_back(LayerEffect{
      .effect_id = EffectId{"fx_background_glow"},
      .parameters = GlowEffect{
          .sigma = 18.0,
          .color = ColorRgba8{.red = 124, .green = 92, .blue = 255},
      },
  });
  background.animations.push_back(ScalarAnimation{
      .property = AnimatedProperty::position_x,
      .keyframes = {{.time = 0, .value = 960.0},
                    {.time = 1'000'000'000, .value = 1000.0}},
  });
  auto title = shape("lyr_title", "Title Plate", 600.0);
  LayerGroupSnapshot group{
      .group_id = LayerGroupId{"grp_title"},
      .display_name = "Title Group",
      .active_range = {.start = 0, .duration = 3'000'000'000},
      .children = {LayerId{"lyr_title"}},
  };
  return ProjectSnapshot{
      .project_id = ProjectId{"prj_agent_eye"},
      .revision_id = RevisionId{revision},
      .display_name = "Agent Eye",
      .composition = CompositionSnapshot{
          .composition_id = CompositionId{"cmp_agent_eye"},
          .display_name = "Main",
          .canvas = {.width_pixels = 1920, .height_pixels = 1080},
          .frame_rate = {.numerator = 30, .denominator = 1},
          .duration = 3'000'000'000,
          .layers = {std::move(background), std::move(title)},
          .groups = {std::move(group)},
          .root_nodes = {LayerId{"lyr_background"}, LayerGroupId{"grp_title"}},
      },
  };
}

void outline_has_stable_semantic_addresses_and_ownership() {
  const auto project = fixture();
  const auto outline = agent_project_outline(project);
  require(outline.project_id == project.project_id, "project ID changed");
  require(outline.revision_id == RevisionId{7}, "revision changed");
  require(outline.registry_digest == visual_property_registry_digest(),
          "registry digest changed");
  require(outline.contribution_registry_digest ==
              visual_contribution_registry_digest(),
          "contribution registry digest changed");
  require(outline.snapshot_digest == project_snapshot_digest(project),
          "snapshot digest changed");
  require(outline.roots.size() == 2, "root count changed");
  require(outline.nodes.size() == 3, "node count changed");

  const auto* background = find_agent_visual_node(
      outline, LayerId{"lyr_background"});
  require(background != nullptr, "background address missing");
  require(background->parent_path.empty(), "root received a parent");
  require(background->sibling_index == 0 && background->timeline_row == 1,
          "root Timeline projection changed");
  require(background->owned_masks ==
              std::vector<MaskId>{MaskId{"mask_background"}},
          "mask ownership changed");
  require(background->owned_effects ==
              std::vector<EffectId>{EffectId{"fx_background_glow"}},
          "effect ownership changed");
  require(background->animated_properties ==
              std::vector<AnimatedProperty>{AnimatedProperty::position_x},
          "animation ownership changed");

  const auto* title = find_agent_visual_node(outline, LayerId{"lyr_title"});
  require(title != nullptr, "nested title address missing");
  require(title->parent_group == LayerGroupId{"grp_title"},
          "nested parent changed");
  require(title->parent_path ==
              std::vector<LayerGroupId>{LayerGroupId{"grp_title"}},
          "nested parent path changed");
  require(title->sibling_index == 0 && title->timeline_row == 0,
          "nested Timeline row changed");

  const auto canonical = serialize_project_rfx(project);
  const auto round_trip = compile_project_rfx(canonical);
  require(round_trip.succeeded(), "canonical project failed to compile");
  require(project_snapshot_digest(*round_trip.project) ==
              outline.snapshot_digest,
          "round-trip snapshot digest changed");
}

void diff_reports_revision_content_and_topology_separately() {
  const auto before = fixture();
  auto property_candidate = before;
  property_candidate.revision_id = RevisionId{8};
  auto* title = const_cast<LayerSnapshot*>(find_layer(
      *property_candidate.composition, LayerId{"lyr_title"}));
  require(title != nullptr, "candidate title missing");
  title->transform.position_x = 720.0;

  const auto property_diff = agent_project_diff(before, property_candidate);
  require(property_diff.same_project_id, "same project was not recognized");
  require(property_diff.next_revision, "next revision was not recognized");
  require(!property_diff.topology_changed,
          "property edit was reported as topology change");
  require(property_diff.changed_nodes ==
              std::vector<VisualNodeRef>{LayerId{"lyr_title"}},
          "changed node set is incorrect");
  require(property_diff.added_nodes.empty() &&
              property_diff.removed_nodes.empty(),
          "property edit added or removed nodes");
  require(property_diff.before_digest != property_diff.after_digest,
          "changed project kept the same digest");

  auto topology_candidate = before;
  topology_candidate.revision_id = RevisionId{8};
  topology_candidate.composition->groups.clear();
  topology_candidate.composition->root_nodes = {
      LayerId{"lyr_background"}, LayerId{"lyr_title"}};
  const auto topology_diff = agent_project_diff(before, topology_candidate);
  require(topology_diff.topology_changed,
          "reparenting was not reported as topology change");
}

}  // namespace

int main() {
  outline_has_stable_semantic_addresses_and_ownership();
  diff_reports_revision_content_and_topology_separately();
  return 0;
}
