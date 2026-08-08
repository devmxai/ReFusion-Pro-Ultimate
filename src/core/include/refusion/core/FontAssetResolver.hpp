#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace refusion::core {

struct FontAssetRequest final {
  std::string asset_id;
  std::string expected_content_digest;
  std::uint32_t face_index{0};
};

struct FontAssetBlob final {
  std::shared_ptr<const std::vector<std::uint8_t>> bytes;
  std::string verified_content_digest;
  std::uint32_t face_index{0};
};

struct FontAssetResolution final {
  std::optional<FontAssetBlob> asset;
  std::string diagnostic_code;
  std::string diagnostic_message;

  [[nodiscard]] bool succeeded() const noexcept {
    return asset.has_value() && diagnostic_code.empty();
  }
};

// Asset identity and bytes are engine values. Filesystem paths, Qt types and
// platform font managers stay behind implementations of this port.
class FontAssetResolverPort {
 public:
  virtual ~FontAssetResolverPort() = default;

  [[nodiscard]] virtual FontAssetResolution resolve_font_asset(
      const FontAssetRequest& request) = 0;
};

}  // namespace refusion::core
