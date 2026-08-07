#include "refusion/application/ProjectCommandService.hpp"

#include <iostream>

int main() {
  using namespace refusion::application;
  using namespace refusion::core;

  auto commands = create_application_host(ProjectSnapshot{
      .project_id = ProjectId{"prj_01JREFUSIONFOUNDATION"},
      .revision_id = RevisionId{1},
      .display_name = "Untitled ReFusion Project",
  });

  const auto result = commands->submit(RenameProjectCommand{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_foundation_cli_demo_1"},
          .expected_revision = RevisionId{1},
          .idempotency_key = IdempotencyKey{"foundation-cli-demo-1"},
      },
      .requested_name = "ReFusion Foundation",
  });

  std::cout << "accepted=" << (result.accepted() ? "true" : "false")
            << " revision=" << result.active_snapshot.revision_id.value
            << " command_id=" << result.command_id.value
            << " project_id=" << result.active_snapshot.project_id.value
            << " name=\"" << result.active_snapshot.display_name << "\"\n";
  return result.accepted() ? 0 : 1;
}
