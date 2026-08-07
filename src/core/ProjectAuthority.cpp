#include "refusion/core/ProjectAuthority.hpp"

#include <stdexcept>
#include <utility>

namespace refusion::core {

ProjectAuthority::ProjectAuthority(ProjectSnapshot initial_snapshot)
    : active_(std::move(initial_snapshot)) {
  if (active_.project_id.value.empty()) {
    throw std::invalid_argument("project ID must not be empty");
  }
  if (active_.revision_id.value == 0) {
    throw std::invalid_argument("initial revision must be non-zero");
  }
  if (active_.display_name.empty()) {
    throw std::invalid_argument("project name must not be empty");
  }
}

ProjectSnapshot ProjectAuthority::active_snapshot() const {
  std::scoped_lock lock(mutex_);
  return active_;
}

ApplyResult ProjectAuthority::rejected(std::string code, std::string message) const {
  return ApplyResult{
      .accepted = false,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{
          .code = std::move(code),
          .message = std::move(message),
          .blocking = true,
      },
  };
}

ApplyResult ProjectAuthority::apply(const RenameProjectCommand& command) {
  std::scoped_lock lock(mutex_);

  if (command.idempotency_key.empty()) {
    return rejected("RFX-CMD-001", "idempotency key is required");
  }

  if (const auto found = idempotency_ledger_.find(command.idempotency_key);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.command.expected_revision == command.expected_revision &&
        recorded.command.requested_name == command.requested_name) {
      return recorded.result;
    }
    return rejected("RFX-CMD-002", "idempotency key was reused for different intent");
  }

  if (command.expected_revision != active_.revision_id) {
    return rejected("RFX-REV-409", "expected revision does not match active revision");
  }

  if (command.requested_name.empty()) {
    return rejected("RFX-SCHEMA-001", "project name must not be empty");
  }

  active_ = ProjectSnapshot{
      .project_id = active_.project_id,
      .revision_id = RevisionId{active_.revision_id.value + 1},
      .display_name = command.requested_name,
  };

  ApplyResult result{
      .accepted = true,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(command.idempotency_key,
                              RecordedCommand{.command = command, .result = result});
  return result;
}

}  // namespace refusion::core

