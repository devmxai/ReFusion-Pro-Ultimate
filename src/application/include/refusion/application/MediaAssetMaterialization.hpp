#pragma once

#include "refusion/application/MediaIndexingService.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace refusion::application {

struct MediaAssetMaterializationReceipt final {
  core::AssetId asset_id;
  std::string content_digest;
  std::uint64_t byte_size{0};
  std::string project_relative_original;

  friend bool operator==(const MediaAssetMaterializationReceipt&,
                         const MediaAssetMaterializationReceipt&) = default;
};

// A prepared copy is rollback-owned until retain() is called after its
// corresponding workflow has passed every authority/identity check.
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

}  // namespace refusion::application
