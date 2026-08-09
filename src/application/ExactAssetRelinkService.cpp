#include "refusion/application/ExactAssetRelinkService.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <utility>

namespace refusion::application {
namespace {

[[nodiscard]] RelinkExactAssetResult rejected(
    const core::RevisionId revision, const core::AssetId asset_id,
    std::string code, std::string diagnostic) {
  return RelinkExactAssetResult{
      .status = RelinkExactAssetStatus::rejected,
      .active_revision = revision,
      .asset_id = asset_id,
      .code = std::move(code),
      .diagnostic = std::move(diagnostic),
  };
}

[[nodiscard]] RelinkExactAssetResult cancelled(
    const core::RevisionId revision, const core::AssetId asset_id,
    std::string diagnostic) {
  return RelinkExactAssetResult{
      .status = RelinkExactAssetStatus::cancelled,
      .active_revision = revision,
      .asset_id = asset_id,
      .code = "RFX-MEDIA-RELINK-CANCELLED",
      .diagnostic = std::move(diagnostic),
  };
}

[[nodiscard]] const core::AssetRecord* find_asset(
    const core::ProjectSnapshot& project, const core::AssetId& asset_id) {
  const auto asset =
      std::find_if(project.assets.begin(), project.assets.end(),
                   [&](const core::AssetRecord& candidate) {
                     return candidate.asset_id == asset_id;
                   });
  return asset == project.assets.end() ? nullptr : &*asset;
}

[[nodiscard]] bool cancelled(
    const std::shared_ptr<const MediaCancellationToken>& token) noexcept {
  return token && token->cancelled();
}

}  // namespace

ExactAssetRelinkService::ExactAssetRelinkService(
    ProjectRevisionService& revisions,
    MediaImportWorkspacePort& workspace) noexcept
    : revisions_(revisions), workspace_(workspace) {}

RelinkExactAssetResult ExactAssetRelinkService::execute(
    RelinkExactAssetIntent intent) {
  auto active = revisions_.active_snapshot();
  if (intent.envelope.expected_revision != active.revision_id) {
    return rejected(active.revision_id, intent.asset_id,
                    "RFX-MEDIA-RELINK-STALE-REVISION",
                    "RelinkExactAsset expected a different active Revision");
  }
  if (!intent.source) {
    return rejected(active.revision_id, intent.asset_id,
                    "RFX-MEDIA-RELINK-SOURCE-INVALID",
                    "RelinkExactAsset requires an immutable source lease");
  }
  if (cancelled(intent.cancellation)) {
    return cancelled(active.revision_id, intent.asset_id,
                     "RelinkExactAsset was cancelled before validation");
  }
  const auto* asset = find_asset(active, intent.asset_id);
  if (asset == nullptr) {
    return rejected(active.revision_id, intent.asset_id,
                    "RFX-MEDIA-RELINK-ASSET-NOT-FOUND",
                    "the requested AssetId is absent from accepted project truth");
  }
  if (asset->content_digest != intent.source->content_digest() ||
      asset->byte_size != intent.source->byte_size()) {
    return rejected(active.revision_id, intent.asset_id,
                    "RFX-MEDIA-RELINK-IDENTITY-MISMATCH",
                    "selected bytes do not match the accepted Asset digest and size");
  }

  const MediaAssetMaterializationReceipt expected{
      .asset_id = asset->asset_id,
      .content_digest = asset->content_digest,
      .byte_size = asset->byte_size,
      .project_relative_original = asset->project_relative_original,
  };
  std::unique_ptr<PreparedMediaAsset> prepared;
  try {
    prepared = workspace_.prepare_copy(
        intent.envelope.idempotency_key.value, expected, *intent.source,
        intent.cancellation.get());
  } catch (const std::exception& error) {
    return rejected(active.revision_id, intent.asset_id,
                    "RFX-MEDIA-RELINK-STAGE-EXCEPTION", error.what());
  } catch (...) {
    return rejected(active.revision_id, intent.asset_id,
                    "RFX-MEDIA-RELINK-STAGE-EXCEPTION",
                    "exact asset staging failed with an unknown exception");
  }
  if (!prepared || prepared->receipt() != expected) {
    return rejected(active.revision_id, intent.asset_id,
                    "RFX-MEDIA-RELINK-STAGE-IDENTITY",
                    "workspace did not prepare the exact accepted Asset receipt");
  }
  if (cancelled(intent.cancellation)) {
    return cancelled(active.revision_id, intent.asset_id,
                     "RelinkExactAsset was cancelled before asset commit");
  }
  if (!prepared->commit()) {
    return rejected(active.revision_id, intent.asset_id,
                    "RFX-MEDIA-RELINK-ASSET-COMMIT",
                    "workspace could not atomically restore the exact Asset");
  }
  if (cancelled(intent.cancellation)) {
    return cancelled(active.revision_id, intent.asset_id,
                     "RelinkExactAsset was cancelled before final identity check");
  }

  active = revisions_.active_snapshot();
  const auto* current_asset = find_asset(active, intent.asset_id);
  if (active.revision_id != intent.envelope.expected_revision ||
      current_asset == nullptr ||
      current_asset->content_digest != expected.content_digest ||
      current_asset->byte_size != expected.byte_size ||
      current_asset->project_relative_original !=
          expected.project_relative_original) {
    return rejected(active.revision_id, intent.asset_id,
                    "RFX-MEDIA-RELINK-STALE-AFTER-COPY",
                    "accepted Asset truth changed while exact bytes were staged");
  }

  prepared->retain();
  return RelinkExactAssetResult{
      .status = RelinkExactAssetStatus::restored,
      .active_revision = active.revision_id,
      .asset_id = expected.asset_id,
      .code = "RFX-MEDIA-RELINK-EXACT-RESTORED",
      .diagnostic = "byte-identical project Asset is available at its canonical location",
  };
}

}  // namespace refusion::application
