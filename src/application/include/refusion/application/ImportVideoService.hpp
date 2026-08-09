#pragma once

#include "refusion/application/MediaIndexingService.hpp"
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

struct MediaAssetMaterializationReceipt final {
  core::AssetId asset_id;
  std::string content_digest;
  std::uint64_t byte_size{0};
  std::string project_relative_original;

  friend bool operator==(const MediaAssetMaterializationReceipt&,
                         const MediaAssetMaterializationReceipt&) = default;
};

// A prepared copy is rollback-owned until retain() is called after the one
// accepted project revision has been published. Implementations journal the
// staging/final path and remove an unretained final on rollback or recovery.
class PreparedMediaAsset {
 public:
  virtual ~PreparedMediaAsset() = default;

  [[nodiscard]] virtual const MediaAssetMaterializationReceipt& receipt()
      const noexcept = 0;
  [[nodiscard]] virtual bool commit() noexcept = 0;
  virtual void retain() noexcept = 0;
};

// Configured for one already-open project workspace by the host adapter. It
// receives a path-free source lease and may perform filesystem I/O only; it
// has no project-revision authority.
class MediaImportWorkspacePort {
 public:
  virtual ~MediaImportWorkspacePort() = default;

  [[nodiscard]] virtual std::unique_ptr<PreparedMediaAsset> prepare_copy(
      const std::string& transaction_id,
      const MediaAssetMaterializationReceipt& expected,
      ImmutableCompressedSourceLease& source,
      const MediaCancellationToken* cancellation) = 0;
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
