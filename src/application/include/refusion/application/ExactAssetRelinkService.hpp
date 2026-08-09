#pragma once

#include "refusion/application/MediaAssetMaterialization.hpp"
#include "refusion/application/ProjectCommandService.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace refusion::application {

struct RelinkExactAssetIntent final {
  core::CommandEnvelope envelope;
  core::AssetId asset_id;
  std::shared_ptr<ImmutableCompressedSourceLease> source;
  std::shared_ptr<const MediaCancellationToken> cancellation;
};

enum class RelinkExactAssetStatus : std::uint8_t {
  rejected,
  cancelled,
  restored,
};

struct RelinkExactAssetResult final {
  RelinkExactAssetStatus status{RelinkExactAssetStatus::rejected};
  core::RevisionId active_revision;
  core::AssetId asset_id;
  std::string code;
  std::string diagnostic;

  [[nodiscard]] bool succeeded() const noexcept {
    return status == RelinkExactAssetStatus::restored;
  }
};

// Restores only the exact bytes already named by accepted project truth. It
// never mutates Clip/MediaSource semantics or manufactures a project Revision.
// Different bytes require the future ReplaceMediaSource workflow.
class ExactAssetRelinkService final {
 public:
  ExactAssetRelinkService(ProjectRevisionService& revisions,
                          MediaImportWorkspacePort& workspace) noexcept;

  [[nodiscard]] RelinkExactAssetResult execute(
      RelinkExactAssetIntent intent);

 private:
  ProjectRevisionService& revisions_;
  MediaImportWorkspacePort& workspace_;
};

}  // namespace refusion::application
