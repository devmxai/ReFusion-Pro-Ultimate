#include "refusion/application/ProjectCommandService.hpp"

#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("application host test requirement failed");
  }
}

}  // namespace

int main() {
  using namespace refusion::application;
  using namespace refusion::core;

  auto commands = create_application_host(ProjectSnapshot{
      .project_id = ProjectId{"prj_application_host"},
      .revision_id = RevisionId{10},
      .display_name = "Before",
  });

  const auto result = commands->submit(RenameProjectCommand{
      .envelope = CommandEnvelope{
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
}
