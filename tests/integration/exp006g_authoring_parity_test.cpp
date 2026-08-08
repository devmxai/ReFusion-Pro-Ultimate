#include "StudioBridge.hpp"

#include "refusion/application/ProjectCommandService.hpp"
#include "refusion/core/AgentIntrospection.hpp"
#include "refusion/core/ProjectRfx.hpp"

#include <QCoreApplication>

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

[[nodiscard]] ProjectSnapshot fixture() {
  std::ifstream input(REFUSION_EXP006G_PROJECT_PATH, std::ios::binary);
  require(static_cast<bool>(input), "cannot read EXP-006G parity fixture");
  const std::string source{std::istreambuf_iterator<char>(input),
                           std::istreambuf_iterator<char>()};
  const auto compiled = compile_project_rfx(source);
  require(compiled.succeeded(), "EXP-006G parity fixture does not compile");
  return *compiled.project;
}

[[nodiscard]] CommandEnvelope agent_envelope(const std::string& id) {
  return CommandEnvelope{
      .command_id = CommandId{"cmd_agent_" + id},
      .expected_revision = RevisionId{1},
      .idempotency_key = IdempotencyKey{"idem_agent_" + id},
  };
}

void ui_and_agent_effect_intents_have_identical_semantics() {
  auto base = fixture();
  for (auto& layer : base.composition->layers) {
    if (layer.layer_id == LayerId{"lyr_title"}) {
      layer.effects.erase(layer.effects.begin() + 1);
    }
  }
  require(validate_composition(*base.composition).valid,
          "effect parity base is invalid");

  auto ui_commands =
      refusion::application::create_application_host(base);
  StudioBridge ui(*ui_commands);
  ui.selectVisualNode(QStringLiteral("lyr_title"), false);
  ui.addSelectedEffect(QStringLiteral("glow"));
  require(ui.revision() == 2 && ui.diagnostic().isEmpty(),
          "UI Glow intent was rejected");
  const auto ui_snapshot = ui_commands->active_snapshot();

  auto agent_commands =
      refusion::application::create_application_host(base);
  const auto agent_result = agent_commands->submit(AddEffectCommand{
      .envelope = agent_envelope("add_glow"),
      .layer_id = LayerId{"lyr_title"},
      .effect = LayerEffect{
          .effect_id = EffectId{"fx_ui_2_2"},
          .enabled = true,
          .parameters = GlowEffect{
              .sigma = 18.0,
              .color = ColorRgba8{
                  .red = 124, .green = 92, .blue = 255, .alpha = 192},
          },
      },
  });
  require(agent_result.accepted(), "Agent Glow intent was rejected");
  const auto agent_snapshot = agent_commands->active_snapshot();
  require(ui_snapshot == agent_snapshot,
          "UI and Agent Glow produced different snapshots");
  require(project_snapshot_digest(ui_snapshot) ==
              project_snapshot_digest(agent_snapshot),
          "UI and Agent Glow produced different semantic digests");

  const auto ui_diff = agent_project_diff(base, ui_snapshot);
  const auto agent_diff = agent_project_diff(base, agent_snapshot);
  require(ui_diff.changed_nodes == agent_diff.changed_nodes &&
              ui_diff.changed_nodes ==
                  std::vector<VisualNodeRef>{LayerId{"lyr_title"}} &&
              !ui_diff.topology_changed && !agent_diff.topology_changed,
          "Glow parity changed visual topology or the wrong owner");
}

void ui_and_agent_alignment_intents_have_identical_semantics() {
  const auto base = fixture();
  constexpr ProjectTimeNs kTime = 1'000'000'000;

  auto ui_commands =
      refusion::application::create_application_host(base);
  StudioBridge ui(*ui_commands);
  ui.setCompositionTimeProvider([] { return kTime; });
  ui.selectVisualNode(QStringLiteral("lyr_title"), false);
  ui.submitSelectedAlignment(
      QStringLiteral("lyr_bg_base"), false, QStringLiteral("Left"),
      QStringLiteral("Top"), QStringLiteral("Geometry"));
  require(ui.revision() == 2 && ui.diagnostic().isEmpty(),
          "UI Align intent was rejected");
  const auto ui_snapshot = ui_commands->active_snapshot();

  auto agent_commands =
      refusion::application::create_application_host(base);
  const auto agent_result = agent_commands->submit(AlignNodesCommand{
      .envelope = agent_envelope("align_title"),
      .subject = LayerId{"lyr_title"},
      .target = LayerId{"lyr_bg_base"},
      .composition_time = kTime,
      .horizontal = HorizontalAlignIntent::left,
      .vertical = VerticalAlignIntent::top,
      .bounds_basis = AlignmentBoundsBasis::geometry,
  });
  require(agent_result.accepted(), "Agent Align intent was rejected");
  const auto agent_snapshot = agent_commands->active_snapshot();
  require(ui_snapshot == agent_snapshot,
          "UI and Agent Align produced different snapshots");
  require(project_snapshot_digest(ui_snapshot) ==
              project_snapshot_digest(agent_snapshot),
          "UI and Agent Align produced different semantic digests");

  const auto diff = agent_project_diff(base, agent_snapshot);
  require(diff.next_revision && !diff.topology_changed &&
              diff.changed_nodes ==
                  std::vector<VisualNodeRef>{LayerId{"lyr_title"}},
          "Align parity changed topology or the wrong owner");
}

}  // namespace

int main(int argc, char** argv) {
  QCoreApplication application(argc, argv);
  ui_and_agent_effect_intents_have_identical_semantics();
  ui_and_agent_alignment_intents_have_identical_semantics();
  return 0;
}
