#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace refusion::core {

struct ProjectId final {
  std::string value;

  friend bool operator==(const ProjectId&, const ProjectId&) = default;
};

struct RevisionId final {
  std::uint64_t value{0};

  friend bool operator==(const RevisionId&, const RevisionId&) = default;
};

struct CommandId final {
  std::string value;

  friend bool operator==(const CommandId&, const CommandId&) = default;
};

struct IdempotencyKey final {
  std::string value;

  friend bool operator==(const IdempotencyKey&, const IdempotencyKey&) = default;
};

struct ProjectSnapshot final {
  ProjectId project_id;
  RevisionId revision_id;
  std::string display_name;
};

struct CommandEnvelope final {
  CommandId command_id;
  RevisionId expected_revision;
  IdempotencyKey idempotency_key;

  friend bool operator==(const CommandEnvelope&, const CommandEnvelope&) = default;
};

struct RenameProjectCommand final {
  CommandEnvelope envelope;
  std::string requested_name;
};

struct Diagnostic final {
  std::string code;
  std::string message;
  bool blocking{false};
};

enum class ApplyStatus : std::uint8_t {
  rejected,
  accepted,
  replayed,
};

struct ApplyResult final {
  ApplyStatus status{ApplyStatus::rejected};
  CommandId command_id;
  RevisionId committed_revision;
  ProjectSnapshot active_snapshot;
  Diagnostic diagnostic;

  [[nodiscard]] bool accepted() const noexcept {
    return status == ApplyStatus::accepted || status == ApplyStatus::replayed;
  }

  [[nodiscard]] bool replayed() const noexcept {
    return status == ApplyStatus::replayed;
  }
};

class ProjectAuthority final {
 public:
  explicit ProjectAuthority(ProjectSnapshot initial_snapshot);

  [[nodiscard]] ProjectSnapshot active_snapshot() const;
  [[nodiscard]] ApplyResult apply(const RenameProjectCommand& command);

 private:
  struct RecordedCommand final {
    RenameProjectCommand command;
    RevisionId committed_revision;
  };

  [[nodiscard]] ApplyResult rejected(const CommandId& command_id,
                                     std::string code,
                                     std::string message) const;

  mutable std::mutex mutex_;
  ProjectSnapshot active_;
  std::unordered_map<std::string, RecordedCommand> idempotency_ledger_;
  std::unordered_map<std::string, std::string> command_id_index_;
};

}  // namespace refusion::core
