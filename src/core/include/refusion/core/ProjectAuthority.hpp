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

struct ProjectSnapshot final {
  ProjectId project_id;
  RevisionId revision_id;
  std::string display_name;
};

struct RenameProjectCommand final {
  RevisionId expected_revision;
  std::string requested_name;
  std::string idempotency_key;
};

struct Diagnostic final {
  std::string code;
  std::string message;
  bool blocking{true};
};

struct ApplyResult final {
  bool accepted{false};
  ProjectSnapshot active_snapshot;
  Diagnostic diagnostic;
};

class ProjectAuthority final {
 public:
  explicit ProjectAuthority(ProjectSnapshot initial_snapshot);

  [[nodiscard]] ProjectSnapshot active_snapshot() const;
  [[nodiscard]] ApplyResult apply(const RenameProjectCommand& command);

 private:
  struct RecordedCommand final {
    RenameProjectCommand command;
    ApplyResult result;
  };

  [[nodiscard]] ApplyResult rejected(std::string code, std::string message) const;

  mutable std::mutex mutex_;
  ProjectSnapshot active_;
  std::unordered_map<std::string, RecordedCommand> idempotency_ledger_;
};

}  // namespace refusion::core

