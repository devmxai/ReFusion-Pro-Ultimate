#include "refusion/core/ProjectAuthority.hpp"

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using refusion::core::ApplyResult;
using refusion::core::CommandEnvelope;
using refusion::core::CommandId;
using refusion::core::IdempotencyKey;
using refusion::core::ProjectAuthority;
using refusion::core::ProjectId;
using refusion::core::ProjectSnapshot;
using refusion::core::RenameProjectCommand;
using refusion::core::ReplaceProjectCommand;
using refusion::core::RevisionId;
using refusion::core::SetVisualTransformCommand;
using refusion::core::SetVisualPropertyCommand;
using refusion::core::SetLayerEffectsCommand;

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("project authority test requirement failed");
  }
}

[[nodiscard]] RenameProjectCommand rename_command(std::string command_id,
                                                  std::string idempotency_key,
                                                  const std::uint64_t revision,
                                                  std::string requested_name) {
  return RenameProjectCommand{
      .envelope = CommandEnvelope{
          .command_id = CommandId{std::move(command_id)},
          .expected_revision = RevisionId{revision},
          .idempotency_key = IdempotencyKey{std::move(idempotency_key)},
      },
      .requested_name = std::move(requested_name),
  };
}

[[nodiscard]] refusion::core::CompositionSnapshot composition_fixture() {
  using namespace refusion::core;
  return CompositionSnapshot{
      .composition_id = CompositionId{"cmp_authority"},
      .display_name = "Authority Composition",
      .canvas = CanvasExtent{.width_pixels = 1080, .height_pixels = 1920},
      .frame_rate = RationalRate{.numerator = 30, .denominator = 1},
      .duration = 30'000'000'000,
      .layers = {
          LayerSnapshot{
              .layer_id = LayerId{"lyr_background"},
              .display_name = "Background",
              .active_range = TimeRangeNs{
                  .start = 0,
                  .duration = 30'000'000'000,
              },
              .transform = Transform2D{
                  .position_x = 540.0,
                  .position_y = 960.0,
              },
              .content = ShapeLayerContent{
                  .width = 1080.0,
                  .height = 1920.0,
                  .corner_radius = 0.0,
                  .fill = ColorRgba8{.red = 5, .green = 6, .blue = 10},
              },
          },
      },
  };
}

void constructor_rejects_invalid_snapshot() {
  bool rejected_empty_id = false;
  try {
    static_cast<void>(ProjectAuthority(ProjectSnapshot{
        .project_id = ProjectId{" "},
        .revision_id = RevisionId{1},
        .display_name = "Valid",
    }));
  } catch (const std::invalid_argument&) {
    rejected_empty_id = true;
  }
  require(rejected_empty_id);

  bool rejected_zero_revision = false;
  try {
    static_cast<void>(ProjectAuthority(ProjectSnapshot{
        .project_id = ProjectId{"prj_valid"},
        .revision_id = RevisionId{0},
        .display_name = "Valid",
    }));
  } catch (const std::invalid_argument&) {
    rejected_zero_revision = true;
  }
  require(rejected_zero_revision);

  bool rejected_blank_name = false;
  try {
    static_cast<void>(ProjectAuthority(ProjectSnapshot{
        .project_id = ProjectId{"prj_valid"},
        .revision_id = RevisionId{1},
        .display_name = "\t",
    }));
  } catch (const std::invalid_argument&) {
    rejected_blank_name = true;
  }
  require(rejected_blank_name);
}

void command_contract_is_deterministic_and_preserves_lkg() {
  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_stable"},
      .revision_id = RevisionId{7},
      .display_name = "Before",
  });

  const auto first_command =
      rename_command("cmd_001", "idem_001", 7, "After");
  const auto accepted = authority.apply(first_command);
  require(accepted.accepted());
  require(!accepted.replayed());
  require(accepted.command_id == CommandId{"cmd_001"});
  require(!accepted.diagnostic.blocking);
  require(accepted.diagnostic.code.empty());
  require(accepted.committed_revision == RevisionId{8});
  require(accepted.active_snapshot.project_id == ProjectId{"prj_stable"});
  require(accepted.active_snapshot.revision_id == RevisionId{8});
  require(accepted.active_snapshot.display_name == "After");

  const auto immediate_replay = authority.apply(first_command);
  require(immediate_replay.accepted());
  require(immediate_replay.replayed());
  require(immediate_replay.committed_revision == RevisionId{8});
  require(immediate_replay.active_snapshot.revision_id == RevisionId{8});

  const auto second = authority.apply(
      rename_command("cmd_002", "idem_002", 8, "Latest accepted"));
  require(second.accepted());
  require(second.committed_revision == RevisionId{9});

  const auto late_replay = authority.apply(first_command);
  require(late_replay.accepted());
  require(late_replay.replayed());
  require(late_replay.committed_revision == RevisionId{8});
  require(late_replay.active_snapshot.revision_id == RevisionId{9});
  require(late_replay.active_snapshot.display_name == "Latest accepted");

  const auto stale =
      authority.apply(rename_command("cmd_003", "idem_003", 7, "Stale overwrite"));
  require(!stale.accepted());
  require(stale.diagnostic.blocking);
  require(stale.diagnostic.code == "RFX-REV-409");
  require(stale.active_snapshot.revision_id == RevisionId{9});
  require(stale.active_snapshot.display_name == "Latest accepted");

  const auto invalid =
      authority.apply(rename_command("cmd_004", "idem_004", 9, "  \t"));
  require(!invalid.accepted());
  require(invalid.diagnostic.code == "RFX-SCHEMA-001");
  require(authority.active_snapshot().display_name == "Latest accepted");

  const auto reused_key = authority.apply(
      rename_command("cmd_other", "idem_001", 9, "Different intent"));
  require(!reused_key.accepted());
  require(reused_key.diagnostic.code == "RFX-CMD-002");

  const auto reused_command_id = authority.apply(
      rename_command("cmd_002", "idem_other", 9, "Different identity"));
  require(!reused_command_id.accepted());
  require(reused_command_id.diagnostic.code == "RFX-CMD-003");

  const auto missing_command_id =
      authority.apply(rename_command("", "idem_005", 9, "Ignored"));
  require(!missing_command_id.accepted());
  require(missing_command_id.diagnostic.code == "RFX-CMD-000");

  const auto missing_idempotency_key =
      authority.apply(rename_command("cmd_006", " ", 9, "Ignored"));
  require(!missing_idempotency_key.accepted());
  require(missing_idempotency_key.diagnostic.code == "RFX-CMD-001");

  const auto final_snapshot = authority.active_snapshot();
  require(final_snapshot.revision_id == RevisionId{9});
  require(final_snapshot.display_name == "Latest accepted");
}

void revision_overflow_fails_closed() {
  const auto maximum_revision = std::numeric_limits<std::uint64_t>::max();
  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_overflow"},
      .revision_id = RevisionId{maximum_revision},
      .display_name = "Last known good",
  });

  const auto result = authority.apply(
      rename_command("cmd_overflow", "idem_overflow", maximum_revision, "Never applied"));
  require(!result.accepted());
  require(result.diagnostic.code == "RFX-REV-OVERFLOW");
  require(result.active_snapshot.revision_id == RevisionId{maximum_revision});
  require(result.active_snapshot.display_name == "Last known good");
}

void complete_candidate_replace_is_atomic_and_preserves_lkg() {
  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_replace"},
      .revision_id = RevisionId{7},
      .display_name = "Last known good",
      .composition = composition_fixture(),
  });

  auto accepted_candidate = authority.active_snapshot();
  accepted_candidate.revision_id = RevisionId{8};
  accepted_candidate.display_name = "Agent revision";
  accepted_candidate.composition->layers.front().transform.position_x = 600.0;
  const ReplaceProjectCommand accepted_command{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_replace_8"},
          .expected_revision = RevisionId{7},
          .idempotency_key = IdempotencyKey{"idem_replace_8"},
      },
      .candidate = accepted_candidate,
  };
  const auto accepted = authority.apply(accepted_command);
  require(accepted.accepted());
  require(accepted.active_snapshot.revision_id == RevisionId{8});
  require(accepted.active_snapshot.display_name == "Agent revision");
  require(accepted.active_snapshot.composition->layers.front()
              .transform.position_x == 600.0);

  const auto replayed = authority.apply(accepted_command);
  require(replayed.accepted());
  require(replayed.replayed());
  require(replayed.committed_revision == RevisionId{8});

  auto invalid_candidate = authority.active_snapshot();
  invalid_candidate.revision_id = RevisionId{9};
  invalid_candidate.composition->layers.push_back(
      invalid_candidate.composition->layers.front());
  const auto invalid = authority.apply(ReplaceProjectCommand{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_replace_9_invalid"},
          .expected_revision = RevisionId{8},
          .idempotency_key = IdempotencyKey{"idem_replace_9_invalid"},
      },
      .candidate = invalid_candidate,
  });
  require(!invalid.accepted());
  require(invalid.diagnostic.code == "RFX-PROJECT-107");
  const auto after_invalid = authority.active_snapshot();
  require(after_invalid.revision_id == RevisionId{8});
  require(after_invalid.composition->layers.size() == 1);

  auto stale_candidate = after_invalid;
  stale_candidate.revision_id = RevisionId{9};
  const auto stale = authority.apply(ReplaceProjectCommand{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_replace_stale"},
          .expected_revision = RevisionId{7},
          .idempotency_key = IdempotencyKey{"idem_replace_stale"},
      },
      .candidate = stale_candidate,
  });
  require(!stale.accepted());
  require(stale.diagnostic.code == "RFX-REV-409");
  require(authority.active_snapshot() == after_invalid);
}

void visual_transform_command_is_atomic_and_preserves_lkg() {
  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_visual_command"},
      .revision_id = RevisionId{7},
      .display_name = "Visual command",
      .composition = composition_fixture(),
  });

  const SetVisualTransformCommand command{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_transform_8"},
          .expected_revision = RevisionId{7},
          .idempotency_key = IdempotencyKey{"idem_transform_8"},
      },
      .node = refusion::core::LayerId{"lyr_background"},
      .transform = refusion::core::Transform2D{
          .position_x = 600.0,
          .position_y = 900.0,
          .anchor_x = 10.0,
          .anchor_y = 20.0,
          .scale_x = 1.2,
          .scale_y = 0.8,
          .rotation_degrees = 15.0,
          .opacity = 0.75,
      },
  };
  const auto accepted = authority.apply(command);
  require(accepted.accepted());
  require(accepted.committed_revision == RevisionId{8});
  require(accepted.active_snapshot.composition->layers.front().transform ==
          command.transform);

  const auto replayed = authority.apply(command);
  require(replayed.accepted());
  require(replayed.replayed());
  require(replayed.committed_revision == RevisionId{8});

  auto invalid_transform = command.transform;
  invalid_transform.scale_x = 0.0;
  const auto invalid = authority.apply(SetVisualTransformCommand{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_transform_9_invalid"},
          .expected_revision = RevisionId{8},
          .idempotency_key = IdempotencyKey{"idem_transform_9_invalid"},
      },
      .node = refusion::core::LayerId{"lyr_background"},
      .transform = invalid_transform,
  });
  require(!invalid.accepted());
  require(invalid.diagnostic.code == "RFX-PROJECT-110");
  require(authority.active_snapshot().revision_id == RevisionId{8});
  require(authority.active_snapshot().composition->layers.front().transform ==
          command.transform);

  const auto missing = authority.apply(SetVisualTransformCommand{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_transform_9_missing"},
          .expected_revision = RevisionId{8},
          .idempotency_key = IdempotencyKey{"idem_transform_9_missing"},
      },
      .node = refusion::core::LayerId{"lyr_missing"},
      .transform = command.transform,
  });
  require(!missing.accepted());
  require(missing.diagnostic.code == "RFX-VISUAL-NODE-404");
  require(authority.active_snapshot().revision_id == RevisionId{8});
}

void visual_property_command_uses_registry_and_preserves_lkg() {
  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_property_command"},
      .revision_id = RevisionId{4},
      .display_name = "Property command",
      .composition = composition_fixture(),
  });

  const refusion::core::ColorRgba8 color{
      .red = 20,
      .green = 40,
      .blue = 60,
      .alpha = 220,
  };
  const SetVisualPropertyCommand command{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_property_5"},
          .expected_revision = RevisionId{4},
          .idempotency_key = IdempotencyKey{"idem_property_5"},
      },
      .node = refusion::core::LayerId{"lyr_background"},
      .property_id = refusion::core::VisualPropertyId{"shape.fill"},
      .value = color,
  };
  const auto accepted = authority.apply(command);
  require(accepted.accepted());
  require(accepted.committed_revision == RevisionId{5});
  require(std::get<refusion::core::ColorRgba8>(
              std::get<refusion::core::ShapeLayerContent>(
                  accepted.active_snapshot.composition->layers.front().content)
                  .fill) == color);
  const auto replayed = authority.apply(command);
  require(replayed.replayed());
  require(replayed.committed_revision == RevisionId{5});

  const auto invalid = authority.apply(SetVisualPropertyCommand{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_property_6_invalid"},
          .expected_revision = RevisionId{5},
          .idempotency_key = IdempotencyKey{"idem_property_6_invalid"},
      },
      .node = refusion::core::LayerId{"lyr_background"},
      .property_id = refusion::core::VisualPropertyId{"shape.width"},
      .value = 0.0,
  });
  require(!invalid.accepted());
  require(invalid.diagnostic.code == "RFX-PROJECT-113");
  require(authority.active_snapshot().revision_id == RevisionId{5});
  require(std::get<refusion::core::ShapeLayerContent>(
              authority.active_snapshot().composition->layers.front().content)
              .width == 1080.0);
}

void layer_effect_command_is_atomic_and_preserves_lkg() {
  using namespace refusion::core;
  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_effect_command"},
      .revision_id = RevisionId{10},
      .display_name = "Effect command",
      .composition = composition_fixture(),
  });
  const std::vector<LayerEffect> effects{
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
  };
  const SetLayerEffectsCommand command{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_effect_11"},
          .expected_revision = RevisionId{10},
          .idempotency_key = IdempotencyKey{"idem_effect_11"},
      },
      .layer_id = LayerId{"lyr_background"},
      .effects = effects,
  };
  const auto accepted = authority.apply(command);
  require(accepted.accepted());
  require(accepted.committed_revision == RevisionId{11});
  require(accepted.active_snapshot.composition->layers.front().effects == effects);
  require(authority.apply(command).replayed());

  auto invalid_effects = effects;
  std::get<DropShadowEffect>(invalid_effects.front().parameters).sigma_x = 300.0;
  const auto invalid = authority.apply(SetLayerEffectsCommand{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_effect_12_invalid"},
          .expected_revision = RevisionId{11},
          .idempotency_key = IdempotencyKey{"idem_effect_12_invalid"},
      },
      .layer_id = LayerId{"lyr_background"},
      .effects = invalid_effects,
  });
  require(!invalid.accepted());
  require(invalid.diagnostic.code == "RFX-PROJECT-133");
  require(authority.active_snapshot().revision_id == RevisionId{11});
  require(authority.active_snapshot().composition->layers.front().effects == effects);
}

void add_visual_layer_command_uses_core_owned_preset() {
  using namespace refusion::core;
  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_add_layer"},
      .revision_id = RevisionId{20},
      .display_name = "Add layer",
      .composition = composition_fixture(),
  });
  const AddVisualLayerCommand command{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_add_shape_21"},
          .expected_revision = RevisionId{20},
          .idempotency_key = IdempotencyKey{"idem_add_shape_21"},
      },
      .preset = VisualLayerPreset::shape,
  };
  const auto accepted = authority.apply(command);
  require(accepted.accepted());
  require(accepted.committed_revision == RevisionId{21});
  require(accepted.active_snapshot.composition->layers.size() == 2);
  const auto& added = accepted.active_snapshot.composition->layers.back();
  require(added.layer_id.value == "lyr_ui_21_shape");
  require(added.active_range.duration ==
          accepted.active_snapshot.composition->duration);
  require(std::holds_alternative<ShapeLayerContent>(added.content));
  require(std::holds_alternative<LinearGradientFill>(
      std::get<ShapeLayerContent>(added.content).fill));
  require(authority.apply(command).replayed());
}

void concurrent_commands_have_one_revision_winner() {
  constexpr std::size_t contender_count = 12;
  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_concurrent"},
      .revision_id = RevisionId{100},
      .display_name = "Before race",
  });

  std::mutex start_mutex;
  std::condition_variable start_condition;
  std::size_t ready_count = 0;
  bool start = false;
  std::vector<ApplyResult> results(contender_count);
  std::vector<std::thread> workers;
  workers.reserve(contender_count);

  for (std::size_t index = 0; index < contender_count; ++index) {
    workers.emplace_back([&, index] {
      {
        std::unique_lock lock(start_mutex);
        ++ready_count;
        start_condition.notify_all();
        start_condition.wait(lock, [&start] { return start; });
      }

      const auto suffix = std::to_string(index);
      results[index] = authority.apply(rename_command(
          "cmd_race_" + suffix, "idem_race_" + suffix, 100,
          "Concurrent winner " + suffix));
    });
  }

  {
    std::unique_lock lock(start_mutex);
    start_condition.wait(lock, [&ready_count] { return ready_count == contender_count; });
    start = true;
  }
  start_condition.notify_all();

  for (auto& worker : workers) {
    worker.join();
  }

  const auto accepted_count = std::count_if(
      results.begin(), results.end(), [](const ApplyResult& result) {
        return result.accepted() && !result.replayed();
      });
  const auto stale_count = std::count_if(
      results.begin(), results.end(), [](const ApplyResult& result) {
        return !result.accepted() && result.diagnostic.code == "RFX-REV-409";
      });

  require(accepted_count == 1);
  require(stale_count == contender_count - 1);
  require(authority.active_snapshot().revision_id == RevisionId{101});
}

}  // namespace

int main() {
  constructor_rejects_invalid_snapshot();
  command_contract_is_deterministic_and_preserves_lkg();
  revision_overflow_fails_closed();
  complete_candidate_replace_is_atomic_and_preserves_lkg();
  visual_transform_command_is_atomic_and_preserves_lkg();
  visual_property_command_uses_registry_and_preserves_lkg();
  layer_effect_command_is_atomic_and_preserves_lkg();
  add_visual_layer_command_uses_core_owned_preset();
  concurrent_commands_have_one_revision_winner();
}
