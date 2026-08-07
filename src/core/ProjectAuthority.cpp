#include "refusion/core/ProjectAuthority.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <utility>

namespace refusion::core {
namespace {

[[nodiscard]] bool is_blank(const std::string& value) {
  return value.empty() ||
         std::all_of(value.begin(), value.end(), [](const unsigned char character) {
           return std::isspace(character) != 0;
         });
}

}  // namespace

ProjectAuthority::ProjectAuthority(ProjectSnapshot initial_snapshot)
    : active_(std::move(initial_snapshot)) {
  if (is_blank(active_.project_id.value)) {
    throw std::invalid_argument("project ID must not be empty");
  }
  if (active_.revision_id.value == 0) {
    throw std::invalid_argument("initial revision must be non-zero");
  }
  if (is_blank(active_.display_name)) {
    throw std::invalid_argument("project name must not be empty");
  }
  if (active_.composition) {
    const auto validation = validate_composition(*active_.composition);
    if (!validation.valid) {
      throw std::invalid_argument(validation.code + ": " + validation.message);
    }
  }
}

ProjectSnapshot ProjectAuthority::active_snapshot() const {
  std::scoped_lock lock(mutex_);
  return active_;
}

ApplyResult ProjectAuthority::rejected(const CommandId& command_id,
                                       std::string code,
                                       std::string message) const {
  return ApplyResult{
      .status = ApplyStatus::rejected,
      .command_id = command_id,
      .committed_revision = RevisionId{},
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

  if (is_blank(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-000", "command ID is required");
  }

  if (is_blank(command.envelope.idempotency_key.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-001",
                    "idempotency key is required");
  }

  if (const auto found =
          idempotency_ledger_.find(command.envelope.idempotency_key.value);
      found != idempotency_ledger_.end()) {
    const auto& recorded = found->second;
    if (recorded.command.envelope == command.envelope &&
        recorded.command.requested_name == command.requested_name) {
      return ApplyResult{
          .status = ApplyStatus::replayed,
          .command_id = command.envelope.command_id,
          .committed_revision = recorded.committed_revision,
          .active_snapshot = active_,
          .diagnostic = Diagnostic{},
      };
    }
    return rejected(command.envelope.command_id, "RFX-CMD-002",
                    "idempotency key was reused for different intent");
  }

  if (command_id_index_.contains(command.envelope.command_id.value)) {
    return rejected(command.envelope.command_id, "RFX-CMD-003",
                    "command ID was reused with a different idempotency key");
  }

  if (command.envelope.expected_revision != active_.revision_id) {
    return rejected(command.envelope.command_id, "RFX-REV-409",
                    "expected revision does not match active revision");
  }

  if (is_blank(command.requested_name)) {
    return rejected(command.envelope.command_id, "RFX-SCHEMA-001",
                    "project name must not be empty");
  }

  if (active_.revision_id.value == std::numeric_limits<std::uint64_t>::max()) {
    return rejected(command.envelope.command_id, "RFX-REV-OVERFLOW",
                    "active revision cannot be incremented");
  }

  active_ = ProjectSnapshot{
      .project_id = active_.project_id,
      .revision_id = RevisionId{active_.revision_id.value + 1},
      .display_name = command.requested_name,
      .composition = active_.composition,
  };

  ApplyResult result{
      .status = ApplyStatus::accepted,
      .command_id = command.envelope.command_id,
      .committed_revision = active_.revision_id,
      .active_snapshot = active_,
      .diagnostic = Diagnostic{},
  };
  idempotency_ledger_.emplace(
      command.envelope.idempotency_key.value,
      RecordedCommand{.command = command, .committed_revision = active_.revision_id});
  command_id_index_.emplace(command.envelope.command_id.value,
                            command.envelope.idempotency_key.value);
  return result;
}

}  // namespace refusion::core
