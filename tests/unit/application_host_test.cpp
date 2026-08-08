#include "refusion/application/ProjectCommandService.hpp"

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("application host test requirement failed");
  }
}

[[nodiscard]] refusion::core::LayerSnapshot shape_layer(std::string id,
                                                        const double x) {
  using namespace refusion::core;
  return LayerSnapshot{
      .layer_id = LayerId{std::move(id)},
      .display_name = "Shape",
      .active_range = TimeRangeNs{.start = 0, .duration = 1'000'000'000},
      .transform = Transform2D{.position_x = x},
      .content =
          ShapeLayerContent{
              .width = 100.0,
              .height = 100.0,
              .fill = ColorRgba8{},
          },
  };
}

struct AdmissionState final {
  bool reject{false};
  bool engine_committed{false};
  bool projections_published{false};
  bool projection_observed_committed_state{false};
  std::uint64_t prepared_revision{0};
  std::uint64_t projection_observed_revision{0};
  std::function<void()> projection_observer;
};

class PreparedRevision final
    : public refusion::application::PreparedProjectRevision {
 public:
  explicit PreparedRevision(std::shared_ptr<AdmissionState> state)
      : state_(std::move(state)) {}

  void commit_engine_state() noexcept override {
    state_->engine_committed = true;
  }

  void publish_observer_projections() noexcept override {
    state_->projection_observed_committed_state = state_->engine_committed;
    state_->projections_published = true;
    if (state_->projection_observer) {
      state_->projection_observer();
    }
  }

 private:
  std::shared_ptr<AdmissionState> state_;
};

class AdmissionPort final
    : public refusion::application::ProjectCandidateAdmissionPort {
 public:
  explicit AdmissionPort(std::shared_ptr<AdmissionState> state)
      : state_(std::move(state)) {}

  [[nodiscard]] refusion::application::CandidatePreparationResult prepare(
      const refusion::core::ProjectSnapshot& candidate) override {
    state_->prepared_revision = candidate.revision_id.value;
    state_->engine_committed = false;
    state_->projections_published = false;
    state_->projection_observed_committed_state = false;
    state_->projection_observed_revision = 0;
    if (state_->reject) {
      return {
          .diagnostic = refusion::core::Diagnostic{
              .code = "RFX-TEST-RUNTIME-REJECTED",
              .message = "test candidate rejected before publication",
              .blocking = true,
          },
      };
    }
    return {
        .prepared = std::make_unique<PreparedRevision>(state_),
    };
  }

 private:
  std::shared_ptr<AdmissionState> state_;
};

} // namespace

int main() {
  using namespace refusion::application;
  using namespace refusion::core;

  auto commands = create_application_host(ProjectSnapshot{
      .project_id = ProjectId{"prj_application_host"},
      .revision_id = RevisionId{10},
      .display_name = "Before",
  });

  const auto result = commands->submit(RenameProjectCommand{
      .envelope =
          CommandEnvelope{
              .command_id = CommandId{"cmd_application_host_1"},
              .expected_revision = RevisionId{10},
              .idempotency_key = IdempotencyKey{"idem_application_host_1"},
          },
      .requested_name = "After",
  });

  require(result.accepted());
  require(!result.diagnostic.blocking);
  require(result.diagnostic.code.empty());
  require(commands->active_snapshot().revision_id == RevisionId{11});
  require(commands->active_snapshot().display_name == "After");

  auto admission_commands = create_application_host(ProjectSnapshot{
      .project_id = ProjectId{"prj_atomic_admission"},
      .revision_id = RevisionId{40},
      .display_name = "Accepted",
  });
  auto admission_state = std::make_shared<AdmissionState>();
  admission_state->projection_observer = [&] {
    // This deliberately re-enters the read side exactly as a synchronous QML
    // binding does during a model reset. It must run after admission unlocks.
    admission_state->projection_observed_revision =
        admission_commands->active_snapshot().revision_id.value;
  };
  admission_commands->set_candidate_admission_port(
      std::make_shared<AdmissionPort>(admission_state));
  admission_state->reject = true;
  const auto runtime_rejected = admission_commands->submit(RenameProjectCommand{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_atomic_rejected"},
          .expected_revision = RevisionId{40},
          .idempotency_key = IdempotencyKey{"idem_atomic_rejected"},
      },
      .requested_name = "Must Not Publish",
  });
  require(!runtime_rejected.accepted());
  require(runtime_rejected.diagnostic.code == "RFX-TEST-RUNTIME-REJECTED");
  require(admission_state->prepared_revision == 41);
  require(!admission_state->engine_committed);
  require(!admission_state->projections_published);
  require(admission_commands->active_snapshot().revision_id == RevisionId{40});
  require(admission_commands->active_snapshot().display_name == "Accepted");

  admission_state->reject = false;
  const auto runtime_accepted = admission_commands->submit(RenameProjectCommand{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_atomic_accepted"},
          .expected_revision = RevisionId{40},
          .idempotency_key = IdempotencyKey{"idem_atomic_accepted"},
      },
      .requested_name = "Published",
  });
  require(runtime_accepted.accepted());
  require(admission_state->prepared_revision == 41);
  require(admission_state->engine_committed);
  require(admission_state->projections_published);
  require(admission_state->projection_observed_committed_state);
  require(admission_state->projection_observed_revision == 41);
  require(admission_commands->active_snapshot().revision_id == RevisionId{41});
  require(admission_commands->active_snapshot().display_name == "Published");

  auto semantic_commands = create_application_host(ProjectSnapshot{
      .project_id = ProjectId{"prj_application_semantic"},
      .revision_id = RevisionId{20},
      .display_name = "Semantic",
      .composition =
          CompositionSnapshot{
              .composition_id = CompositionId{"cmp_application_semantic"},
              .display_name = "Semantic",
              .canvas =
                  CanvasExtent{.width_pixels = 1920, .height_pixels = 1080},
              .frame_rate = RationalRate{.numerator = 30, .denominator = 1},
              .duration = 1'000'000'000,
              .layers = {shape_layer("lyr_one", 100.0),
                         shape_layer("lyr_two", 300.0)},
              .root_nodes = {LayerId{"lyr_one"}, LayerId{"lyr_two"}},
          },
  });
  const auto effect = semantic_commands->submit(AddEffectCommand{
      .envelope =
          CommandEnvelope{
              .command_id = CommandId{"cmd_application_effect"},
              .expected_revision = RevisionId{20},
              .idempotency_key = IdempotencyKey{"idem_application_effect"},
          },
      .layer_id = LayerId{"lyr_one"},
      .effect =
          LayerEffect{
              .effect_id = EffectId{"fx_application_glow"},
              .parameters = GlowEffect{.sigma = 8.0},
          },
  });
  require(effect.accepted());
  require(effect.active_snapshot.composition->layers.size() == 2);
  require(effect.active_snapshot.composition->root_nodes.size() == 2);

  const auto aligned = semantic_commands->submit(AlignNodesCommand{
      .envelope =
          CommandEnvelope{
              .command_id = CommandId{"cmd_application_align"},
              .expected_revision = RevisionId{21},
              .idempotency_key = IdempotencyKey{"idem_application_align"},
          },
      .subject = LayerId{"lyr_one"},
      .target = LayerId{"lyr_two"},
      .composition_time = 0,
      .horizontal = HorizontalAlignIntent::center,
      .bounds_basis = AlignmentBoundsBasis::geometry,
  });
  require(aligned.accepted());
  require(find_layer(*aligned.active_snapshot.composition,
                     LayerId{"lyr_one"})
              ->transform.position_x == 300.0);

  const auto grouped = semantic_commands->submit(GroupNodesCommand{
      .envelope =
          CommandEnvelope{
              .command_id = CommandId{"cmd_application_group"},
              .expected_revision = RevisionId{22},
              .idempotency_key = IdempotencyKey{"idem_application_group"},
          },
      .group_id = LayerGroupId{"grp_application"},
      .display_name = "Application Group",
      .nodes = {LayerId{"lyr_one"}, LayerId{"lyr_two"}},
  });
  require(grouped.accepted());
  require(grouped.active_snapshot.composition->groups.size() == 1);
  require(grouped.active_snapshot.composition->root_nodes ==
          std::vector<VisualNodeRef>{LayerGroupId{"grp_application"}});
}
