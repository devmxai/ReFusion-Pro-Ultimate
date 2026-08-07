#include "refusion/core/ProjectAuthority.hpp"

#include <cassert>

int main() {
  using namespace refusion::core;

  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_stable"},
      .revision_id = RevisionId{7},
      .display_name = "Before",
  });

  const RenameProjectCommand accepted_command{
      .expected_revision = RevisionId{7},
      .requested_name = "After",
      .idempotency_key = "cmd-001",
  };
  const auto accepted = authority.apply(accepted_command);
  assert(accepted.accepted);
  assert(accepted.active_snapshot.project_id == ProjectId{"prj_stable"});
  assert(accepted.active_snapshot.revision_id == RevisionId{8});
  assert(accepted.active_snapshot.display_name == "After");

  const auto replayed = authority.apply(accepted_command);
  assert(replayed.accepted);
  assert(replayed.active_snapshot.revision_id == RevisionId{8});

  const auto stale = authority.apply(RenameProjectCommand{
      .expected_revision = RevisionId{7},
      .requested_name = "Stale overwrite",
      .idempotency_key = "cmd-002",
  });
  assert(!stale.accepted);
  assert(stale.diagnostic.code == "RFX-REV-409");
  assert(stale.active_snapshot.revision_id == RevisionId{8});
  assert(stale.active_snapshot.display_name == "After");

  const auto invalid = authority.apply(RenameProjectCommand{
      .expected_revision = RevisionId{8},
      .requested_name = "",
      .idempotency_key = "cmd-003",
  });
  assert(!invalid.accepted);
  assert(invalid.diagnostic.code == "RFX-SCHEMA-001");
  assert(authority.active_snapshot().display_name == "After");

  const auto reused = authority.apply(RenameProjectCommand{
      .expected_revision = RevisionId{8},
      .requested_name = "Different intent",
      .idempotency_key = "cmd-001",
  });
  assert(!reused.accepted);
  assert(reused.diagnostic.code == "RFX-CMD-002");
  assert(authority.active_snapshot().revision_id == RevisionId{8});
}

