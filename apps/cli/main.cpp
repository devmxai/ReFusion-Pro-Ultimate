#include "refusion/core/ProjectAuthority.hpp"

#include <iostream>

int main() {
  using namespace refusion::core;

  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_01JREFUSIONFOUNDATION"},
      .revision_id = RevisionId{1},
      .display_name = "Untitled ReFusion Project",
  });

  const auto result = authority.apply(RenameProjectCommand{
      .expected_revision = RevisionId{1},
      .requested_name = "ReFusion Foundation",
      .idempotency_key = "foundation-cli-demo-1",
  });

  std::cout << "accepted=" << (result.accepted ? "true" : "false")
            << " revision=" << result.active_snapshot.revision_id.value
            << " project_id=" << result.active_snapshot.project_id.value
            << " name=\"" << result.active_snapshot.display_name << "\"\n";
  return result.accepted ? 0 : 1;
}

