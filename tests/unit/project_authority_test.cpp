#include "refusion/core/ProjectAuthority.hpp"

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using refusion::core::ApplyResult;
using refusion::core::CommandEnvelope;
using refusion::core::CommandId;
using refusion::core::IdempotencyKey;
using refusion::core::ProjectAuthority;
using refusion::core::ProjectId;
using refusion::core::ProjectSnapshot;
using refusion::core::RenameProjectCommand;
using refusion::core::RevisionId;

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("project authority test requirement failed");
  }
}

[[nodiscard]] RenameProjectCommand rename_command(std::string command_id,
                                                  std::string idempotency_key,
                                                  const std::uint64_t revision,
                                                  std::string requested_name) {
  return RenameProjectCommand{
      .envelope = CommandEnvelope{
          .command_id = CommandId{std::move(command_id)},
          .expected_revision = RevisionId{revision},
          .idempotency_key = IdempotencyKey{std::move(idempotency_key)},
      },
      .requested_name = std::move(requested_name),
  };
}

void constructor_rejects_invalid_snapshot() {
  bool rejected_empty_id = false;
  try {
    static_cast<void>(ProjectAuthority(ProjectSnapshot{
        .project_id = ProjectId{" "},
        .revision_id = RevisionId{1},
        .display_name = "Valid",
    }));
  } catch (const std::invalid_argument&) {
    rejected_empty_id = true;
  }
  require(rejected_empty_id);

  bool rejected_zero_revision = false;
  try {
    static_cast<void>(ProjectAuthority(ProjectSnapshot{
        .project_id = ProjectId{"prj_valid"},
        .revision_id = RevisionId{0},
        .display_name = "Valid",
    }));
  } catch (const std::invalid_argument&) {
    rejected_zero_revision = true;
  }
  require(rejected_zero_revision);

  bool rejected_blank_name = false;
  try {
    static_cast<void>(ProjectAuthority(ProjectSnapshot{
        .project_id = ProjectId{"prj_valid"},
        .revision_id = RevisionId{1},
        .display_name = "\t",
    }));
  } catch (const std::invalid_argument&) {
    rejected_blank_name = true;
  }
  require(rejected_blank_name);
}

void command_contract_is_deterministic_and_preserves_lkg() {
  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_stable"},
      .revision_id = RevisionId{7},
      .display_name = "Before",
  });

  const auto first_command =
      rename_command("cmd_001", "idem_001", 7, "After");
  const auto accepted = authority.apply(first_command);
  require(accepted.accepted());
  require(!accepted.replayed());
  require(accepted.command_id == CommandId{"cmd_001"});
  require(accepted.committed_revision == RevisionId{8});
  require(accepted.active_snapshot.project_id == ProjectId{"prj_stable"});
  require(accepted.active_snapshot.revision_id == RevisionId{8});
  require(accepted.active_snapshot.display_name == "After");

  const auto immediate_replay = authority.apply(first_command);
  require(immediate_replay.accepted());
  require(immediate_replay.replayed());
  require(immediate_replay.committed_revision == RevisionId{8});
  require(immediate_replay.active_snapshot.revision_id == RevisionId{8});

  const auto second = authority.apply(
      rename_command("cmd_002", "idem_002", 8, "Latest accepted"));
  require(second.accepted());
  require(second.committed_revision == RevisionId{9});

  const auto late_replay = authority.apply(first_command);
  require(late_replay.accepted());
  require(late_replay.replayed());
  require(late_replay.committed_revision == RevisionId{8});
  require(late_replay.active_snapshot.revision_id == RevisionId{9});
  require(late_replay.active_snapshot.display_name == "Latest accepted");

  const auto stale =
      authority.apply(rename_command("cmd_003", "idem_003", 7, "Stale overwrite"));
  require(!stale.accepted());
  require(stale.diagnostic.code == "RFX-REV-409");
  require(stale.active_snapshot.revision_id == RevisionId{9});
  require(stale.active_snapshot.display_name == "Latest accepted");

  const auto invalid =
      authority.apply(rename_command("cmd_004", "idem_004", 9, "  \t"));
  require(!invalid.accepted());
  require(invalid.diagnostic.code == "RFX-SCHEMA-001");
  require(authority.active_snapshot().display_name == "Latest accepted");

  const auto reused_key = authority.apply(
      rename_command("cmd_other", "idem_001", 9, "Different intent"));
  require(!reused_key.accepted());
  require(reused_key.diagnostic.code == "RFX-CMD-002");

  const auto reused_command_id = authority.apply(
      rename_command("cmd_002", "idem_other", 9, "Different identity"));
  require(!reused_command_id.accepted());
  require(reused_command_id.diagnostic.code == "RFX-CMD-003");

  const auto missing_command_id =
      authority.apply(rename_command("", "idem_005", 9, "Ignored"));
  require(!missing_command_id.accepted());
  require(missing_command_id.diagnostic.code == "RFX-CMD-000");

  const auto missing_idempotency_key =
      authority.apply(rename_command("cmd_006", " ", 9, "Ignored"));
  require(!missing_idempotency_key.accepted());
  require(missing_idempotency_key.diagnostic.code == "RFX-CMD-001");

  const auto final_snapshot = authority.active_snapshot();
  require(final_snapshot.revision_id == RevisionId{9});
  require(final_snapshot.display_name == "Latest accepted");
}

void revision_overflow_fails_closed() {
  const auto maximum_revision = std::numeric_limits<std::uint64_t>::max();
  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_overflow"},
      .revision_id = RevisionId{maximum_revision},
      .display_name = "Last known good",
  });

  const auto result = authority.apply(
      rename_command("cmd_overflow", "idem_overflow", maximum_revision, "Never applied"));
  require(!result.accepted());
  require(result.diagnostic.code == "RFX-REV-OVERFLOW");
  require(result.active_snapshot.revision_id == RevisionId{maximum_revision});
  require(result.active_snapshot.display_name == "Last known good");
}

void concurrent_commands_have_one_revision_winner() {
  constexpr std::size_t contender_count = 12;
  ProjectAuthority authority(ProjectSnapshot{
      .project_id = ProjectId{"prj_concurrent"},
      .revision_id = RevisionId{100},
      .display_name = "Before race",
  });

  std::mutex start_mutex;
  std::condition_variable start_condition;
  std::size_t ready_count = 0;
  bool start = false;
  std::vector<ApplyResult> results(contender_count);
  std::vector<std::thread> workers;
  workers.reserve(contender_count);

  for (std::size_t index = 0; index < contender_count; ++index) {
    workers.emplace_back([&, index] {
      {
        std::unique_lock lock(start_mutex);
        ++ready_count;
        start_condition.notify_all();
        start_condition.wait(lock, [&start] { return start; });
      }

      const auto suffix = std::to_string(index);
      results[index] = authority.apply(rename_command(
          "cmd_race_" + suffix, "idem_race_" + suffix, 100,
          "Concurrent winner " + suffix));
    });
  }

  {
    std::unique_lock lock(start_mutex);
    start_condition.wait(lock, [&ready_count] { return ready_count == contender_count; });
    start = true;
  }
  start_condition.notify_all();

  for (auto& worker : workers) {
    worker.join();
  }

  const auto accepted_count = std::count_if(
      results.begin(), results.end(), [](const ApplyResult& result) {
        return result.accepted() && !result.replayed();
      });
  const auto stale_count = std::count_if(
      results.begin(), results.end(), [](const ApplyResult& result) {
        return !result.accepted() && result.diagnostic.code == "RFX-REV-409";
      });

  require(accepted_count == 1);
  require(stale_count == contender_count - 1);
  require(authority.active_snapshot().revision_id == RevisionId{101});
}

}  // namespace

int main() {
  constructor_rejects_invalid_snapshot();
  command_contract_is_deterministic_and_preserves_lkg();
  revision_overflow_fails_closed();
  concurrent_commands_have_one_revision_winner();
}
