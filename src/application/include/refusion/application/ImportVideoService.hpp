#pragma once

#include "refusion/application/MediaAssetMaterialization.hpp"
#include "refusion/application/ProjectCommandService.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace refusion::application {

enum class ImportVideoStage : std::uint8_t {
  validating,
  indexing,
  staging_asset,
  preparing_revision,
  committing_asset,
  publishing_revision,
  completed,
};

class ImportVideoProgressPort {
 public:
  virtual ~ImportVideoProgressPort() = default;
  virtual void report(ImportVideoStage stage) noexcept = 0;
};

struct ImportVideoIntent final {
  core::CommandEnvelope envelope;
  std::shared_ptr<ImmutableCompressedSourceLease> source;
  std::shared_ptr<const MediaCancellationToken> cancellation;
  std::string original_display_name;
  core::ProjectTimeNs timeline_start{0};
};

enum class ImportVideoStatus : std::uint8_t {
  rejected,
  cancelled,
  accepted,
  replayed,
};

struct ImportVideoResult final {
  ImportVideoStatus status{ImportVideoStatus::rejected};
  core::RevisionId active_revision;
  core::AssetId asset_id;
  core::MediaSourceId media_source_id;
  core::LinkedImportId linked_import_id;
  std::string code;
  std::string diagnostic;

  [[nodiscard]] bool succeeded() const noexcept {
    return status == ImportVideoStatus::accepted ||
           status == ImportVideoStatus::replayed;
  }
};

// Shared atomic import orchestrator. Container parsing is delegated to the
// common indexing service, bytes to a filesystem adapter, and publication to
// the one ProjectCommandService. It never parses paths, decodes pixels or
// publishes partial project state.
class ImportVideoService final {
 public:
  ImportVideoService(ProjectRevisionService& commands,
                     MediaIndexingService& indexing,
                     MediaImportWorkspacePort& workspace,
                     ImportVideoProgressPort* progress = nullptr) noexcept;

  [[nodiscard]] ImportVideoResult execute(ImportVideoIntent intent);

 private:
  ProjectRevisionService& commands_;
  MediaIndexingService& indexing_;
  MediaImportWorkspacePort& workspace_;
  ImportVideoProgressPort* progress_{nullptr};
};

}  // namespace refusion::application
