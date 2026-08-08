#include "refusion/core/ProjectAuthority.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/core/SemanticAuthoring.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace refusion::core;

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error(
        "semantic authoring intent test requirement failed");
  }
}

[[nodiscard]] LayerSnapshot shape_layer(std::string id, std::string name,
                                        const double position_x) {
  return LayerSnapshot{
      .layer_id = LayerId{std::move(id)},
      .display_name = std::move(name),
      .active_range =
          TimeRangeNs{
              .start = 0,
              .duration = 30'000'000'000,
          },
      .transform =
          Transform2D{
              .position_x = position_x,
              .position_y = 960.0,
          },
      .content =
          ShapeLayerContent{
              .width = 1080.0,
              .height = 1920.0,
              .fill = ColorRgba8{.red = 12, .green = 20, .blue = 48},
          },
  };
}

[[nodiscard]] LayerSnapshot text_layer() {
  return LayerSnapshot{
      .layer_id = LayerId{"lyr_title"},
      .display_name = "Title",
      .active_range =
          TimeRangeNs{
              .start = 0,
              .duration = 30'000'000'000,
          },
      .transform =
          Transform2D{
              .position_x = 540.0,
              .position_y = 960.0,
          },
      .content =
          TextLayerContent{
              .text = "REFUSION PRO",
              .font = FontIdentity{.family_name = "Arial"},
              .font_size = 96.0,
              .box = TextBox{.width = 900.0, .height = 120.0},
              .fill =
                  ColorRgba8{
                      .red = 255, .green = 255, .blue = 255, .alpha = 255},
          },
  };
}

[[nodiscard]] CompositionSnapshot composition_fixture() {
  std::vector<LayerSnapshot> layers;
  layers.push_back(
      shape_layer("lyr_background_base", "Background Base", 540.0));
  layers.push_back(shape_layer("lyr_background_violet", "Violet Glow", 420.0));
  layers.push_back(shape_layer("lyr_background_cyan", "Cyan Glow", 660.0));
  layers.push_back(text_layer());
  return CompositionSnapshot{
      .composition_id = CompositionId{"cmp_semantic_intents"},
      .display_name = "Semantic Intents",
      .canvas = CanvasExtent{.width_pixels = 1080, .height_pixels = 1920},
      .frame_rate = RationalRate{.numerator = 30, .denominator = 1},
      .duration = 30'000'000'000,
      .layers = std::move(layers),
      .root_nodes =
          {
              LayerId{"lyr_background_base"},
              LayerId{"lyr_background_violet"},
              LayerId{"lyr_background_cyan"},
              LayerId{"lyr_title"},
          },
  };
}

[[nodiscard]] ProjectSnapshot project_fixture(const std::uint64_t revision) {
  return ProjectSnapshot{
      .project_id = ProjectId{"prj_semantic_intents"},
      .revision_id = RevisionId{revision},
      .display_name = "Semantic Intents",
      .composition = composition_fixture(),
  };
}

[[nodiscard]] CommandEnvelope envelope(std::string command_id,
                                       std::string idempotency_key,
                                       const std::uint64_t revision) {
  return CommandEnvelope{
      .command_id = CommandId{std::move(command_id)},
      .expected_revision = RevisionId{revision},
      .idempotency_key = IdempotencyKey{std::move(idempotency_key)},
  };
}

void grouping_and_reparenting_are_atomic_and_ordered() {
  ProjectAuthority authority(project_fixture(20));
  const GroupNodesCommand group_command{
      .envelope = envelope("cmd_group_21", "idem_group_21", 20),
      .group_id = LayerGroupId{"grp_background"},
      .display_name = "Background",
      // Selection order must not change the existing visual stacking order.
      .nodes =
          {
              LayerId{"lyr_background_cyan"},
              LayerId{"lyr_background_base"},
              LayerId{"lyr_background_violet"},
          },
  };
  const auto grouped = authority.apply(group_command);
  require(grouped.accepted());
  require(grouped.committed_revision == RevisionId{21});
  require(grouped.active_snapshot.composition->layers.size() == 4);
  require(grouped.active_snapshot.composition->groups.size() == 1);
  require(grouped.active_snapshot.composition->root_nodes ==
          std::vector<VisualNodeRef>{LayerGroupId{"grp_background"},
                                     LayerId{"lyr_title"}});
  require(grouped.active_snapshot.composition->groups.front().children ==
          std::vector<VisualNodeRef>{LayerId{"lyr_background_base"},
                                     LayerId{"lyr_background_violet"},
                                     LayerId{"lyr_background_cyan"}});
  require(authority.apply(group_command).replayed());

  const ReparentNodesCommand reparent_command{
      .envelope = envelope("cmd_reparent_22", "idem_reparent_22", 21),
      .nodes = {LayerId{"lyr_title"}},
      .new_parent_group = LayerGroupId{"grp_background"},
      .insertion_index = 3,
  };
  const auto reparented = authority.apply(reparent_command);
  require(reparented.accepted());
  require(reparented.committed_revision == RevisionId{22});
  require(reparented.active_snapshot.composition->root_nodes ==
          std::vector<VisualNodeRef>{LayerGroupId{"grp_background"}});
  require(reparented.active_snapshot.composition->groups.front().children ==
          std::vector<VisualNodeRef>{
              LayerId{"lyr_background_base"}, LayerId{"lyr_background_violet"},
              LayerId{"lyr_background_cyan"}, LayerId{"lyr_title"}});
  require(authority.apply(reparent_command).replayed());
  const auto source = serialize_project_rfx(reparented.active_snapshot);
  const auto round_trip = compile_project_rfx(source);
  require(round_trip.succeeded());
  require(*round_trip.project == reparented.active_snapshot);

  const auto before_cycle = authority.active_snapshot();
  const auto cycle = authority.apply(ReparentNodesCommand{
      .envelope = envelope("cmd_reparent_cycle", "idem_reparent_cycle", 22),
      .nodes = {LayerGroupId{"grp_background"}},
      .new_parent_group = LayerGroupId{"grp_background"},
      .insertion_index = 0,
  });
  require(!cycle.accepted());
  require(cycle.diagnostic.code == "RFX-PROJECT-127");
  require(authority.active_snapshot() == before_cycle);
}

void add_effect_preserves_topology_and_lkg() {
  ProjectAuthority authority(project_fixture(30));
  const auto before = authority.active_snapshot();
  const AddEffectCommand add_glow{
      .envelope = envelope("cmd_add_glow_31", "idem_add_glow_31", 30),
      .layer_id = LayerId{"lyr_title"},
      .effect =
          LayerEffect{
              .effect_id = EffectId{"fx_title_glow"},
              .enabled = true,
              .parameters =
                  GlowEffect{
                      .sigma = 18.0,
                      .color = ColorRgba8{.red = 124,
                                          .green = 92,
                                          .blue = 255,
                                          .alpha = 255},
                  },
          },
  };
  const auto accepted = authority.apply(add_glow);
  require(accepted.accepted());
  require(accepted.committed_revision == RevisionId{31});
  require(accepted.active_snapshot.composition->layers.size() ==
          before.composition->layers.size());
  require(accepted.active_snapshot.composition->groups.size() ==
          before.composition->groups.size());
  require(accepted.active_snapshot.composition->root_nodes ==
          before.composition->root_nodes);
  const auto* title =
      find_layer(*accepted.active_snapshot.composition, LayerId{"lyr_title"});
  require(title != nullptr);
  require(title->effects.size() == 1);
  require(title->effects.front() == add_glow.effect);
  require(authority.apply(add_glow).replayed());

  const auto before_duplicate = authority.active_snapshot();
  const auto duplicate = authority.apply(AddEffectCommand{
      .envelope = envelope("cmd_duplicate_fx", "idem_duplicate_fx", 31),
      .layer_id = LayerId{"lyr_background_base"},
      .effect = add_glow.effect,
  });
  require(!duplicate.accepted());
  require(duplicate.diagnostic.code == "RFX-PROJECT-131");
  require(authority.active_snapshot() == before_duplicate);

  const auto invalid_index = authority.apply(AddEffectCommand{
      .envelope = envelope("cmd_invalid_fx_index", "idem_invalid_fx_index", 31),
      .layer_id = LayerId{"lyr_title"},
      .effect =
          LayerEffect{
              .effect_id = EffectId{"fx_shadow"},
              .parameters = DropShadowEffect{},
          },
      .insertion_index = 5,
  });
  require(!invalid_index.accepted());
  require(invalid_index.diagnostic.code == "RFX-INTENT-EFFECT-002");
  require(authority.active_snapshot() == before_duplicate);
}

void unavailable_capabilities_fail_closed() {
  ProjectAuthority authority(project_fixture(40));
  const AddEffectCommand add_glow{
      .envelope = envelope("cmd_add_glow_41", "idem_add_glow_41", 40),
      .layer_id = LayerId{"lyr_title"},
      .effect =
          LayerEffect{
              .effect_id = EffectId{"fx_title_glow"},
              .parameters =
                  GlowEffect{
                      .sigma = 12.0,
                      .color = ColorRgba8{.red = 255, .green = 80, .blue = 120},
                  },
          },
  };
  require(authority.apply(add_glow).accepted());
  const auto last_known_good = authority.active_snapshot();

  const auto animated = authority.apply(AnimateEffectPropertyCommand{
      .envelope = envelope("cmd_animate_glow", "idem_animate_glow", 41),
      .layer_id = LayerId{"lyr_title"},
      .effect_id = EffectId{"fx_title_glow"},
      .property_id = "effect.glow.color",
      .keyframes =
          {
              {.time = 0, .value = 0.0},
              {.time = 1'000'000'000, .value = 1.0},
          },
  });
  require(!animated.accepted());
  require(animated.diagnostic.code == "RFX-CAP-FX-ANIMATION-001");
  require(authority.active_snapshot() == last_known_good);

  const auto aligned = authority.apply(AlignNodesCommand{
      .envelope = envelope("cmd_align", "idem_align", 41),
      .subject = LayerId{"lyr_title"},
      .target = LayerId{"lyr_background_base"},
      .composition_time = 0,
      .horizontal = HorizontalAlignIntent::center,
      .vertical = VerticalAlignIntent::center,
      .bounds_basis = AlignmentBoundsBasis::logical,
  });
  require(!aligned.accepted());
  require(aligned.diagnostic.code == "RFX-MEASURE-PORT-001");
  require(authority.active_snapshot() == last_known_good);

  const auto capabilities = authoring_capabilities();
  const auto find_capability = [&capabilities](const std::string& id) {
    return std::find_if(capabilities.begin(), capabilities.end(),
                        [&id](const AuthoringCapability& capability) {
                          return capability.capability_id == id;
                        });
  };
  const auto grouping = find_capability("visual.group-nodes");
  require(grouping != capabilities.end());
  require(grouping->supported);
  const auto effect_animation = find_capability("effect.property.animate");
  require(effect_animation != capabilities.end());
  require(!effect_animation->supported);
  require(effect_animation->unavailable_code == "RFX-CAP-FX-ANIMATION-001");
  const auto alignment = find_capability("layout.align-nodes");
  require(alignment != capabilities.end());
  require(alignment->supported);
}

void semantic_lint_is_advisory_and_detects_known_topology_patterns() {
  auto composition = composition_fixture();
  const auto flat_issues = semantic_authoring_lint(composition);
  require(std::any_of(flat_issues.begin(), flat_issues.end(),
                      [](const SemanticAuthoringIssue& issue) {
                        return issue.code ==
                               "RFX-LINT-TOPOLOGY-UNGROUPED-BACKGROUND";
                      }));

  auto duplicate = text_layer();
  duplicate.layer_id = LayerId{"lyr_title_glow_clone"};
  duplicate.display_name = "Title Glow Clone";
  duplicate.effects.push_back(LayerEffect{
      .effect_id = EffectId{"fx_clone_glow"},
      .parameters =
          GlowEffect{
              .sigma = 12.0,
              .color = ColorRgba8{.red = 255, .green = 80, .blue = 120},
          },
  });
  composition.layers.push_back(duplicate);
  composition.root_nodes.emplace_back(duplicate.layer_id);
  require(validate_composition(composition).valid);
  const auto duplicate_issues = semantic_authoring_lint(composition);
  require(std::any_of(duplicate_issues.begin(), duplicate_issues.end(),
                      [](const SemanticAuthoringIssue& issue) {
                        return issue.code == "RFX-LINT-FX-DUPLICATE-TEXT-LAYER";
                      }));

  // Lint never mutates or rejects the otherwise valid authored Composition.
  require(validate_composition(composition).valid);
}

} // namespace

int main() {
  grouping_and_reparenting_are_atomic_and_ordered();
  add_effect_preserves_topology_and_lkg();
  unavailable_capabilities_fail_closed();
  semantic_lint_is_advisory_and_detects_known_topology_patterns();
}
