#pragma once

#include "refusion/application/ImportVideoService.hpp"

#include <QString>

#include <memory>

struct QtCompressedFileSourceResult final {
  std::shared_ptr<
      refusion::application::ImmutableCompressedSourceLease> source;
  QString diagnostic;

  [[nodiscard]] bool succeeded() const noexcept { return source != nullptr; }
};

// Desktop file-portal adapter. The canonical host path remains private to this
// lease and never enters Project.rfx, MediaIndex or an Agent-facing receipt.
[[nodiscard]] QtCompressedFileSourceResult
open_immutable_compressed_file_source(const QString& selected_file) noexcept;

// Qt filesystem implementation of the shared import workspace port. It owns
// staging/journal/recovery mechanics only and has no revision authority.
class QtMediaImportWorkspace final
    : public refusion::application::MediaImportWorkspacePort {
 public:
  explicit QtMediaImportWorkspace(QString project_directory);

  [[nodiscard]] std::unique_ptr<
      refusion::application::PreparedMediaAsset>
  prepare_copy(
      const std::string& transaction_id,
      const refusion::application::MediaAssetMaterializationReceipt& expected,
      refusion::application::ImmutableCompressedSourceLease& source,
      const refusion::application::MediaCancellationToken* cancellation)
      override;

 private:
  QString project_directory_;
};

// Removes incomplete staging and unreferenced committed assets left by a
// process interruption. Assets referenced by the accepted snapshot are kept.
[[nodiscard]] bool recover_incomplete_media_imports(
    const QString& project_directory,
    const refusion::core::ProjectSnapshot& accepted_project,
    QString* diagnostic = nullptr) noexcept;
