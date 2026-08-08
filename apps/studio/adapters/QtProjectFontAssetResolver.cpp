#include "adapters/QtProjectFontAssetResolver.hpp"

#include "refusion/core/ContentDigest.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] bool portable_asset_id(const std::string& value) noexcept {
  if (value.empty()) {
    return false;
  }
  for (const unsigned char byte : value) {
    const bool letter = (byte >= 'A' && byte <= 'Z') ||
                        (byte >= 'a' && byte <= 'z');
    const bool digit = byte >= '0' && byte <= '9';
    if (!letter && !digit && byte != '_' && byte != '-') {
      return false;
    }
  }
  return true;
}

[[nodiscard]] refusion::core::FontAssetResolution rejected(
    std::string code,
    std::string message) {
  return {.diagnostic_code = std::move(code),
          .diagnostic_message = std::move(message)};
}

}  // namespace

QtProjectFontAssetResolver::QtProjectFontAssetResolver(
    QString project_directory)
    : project_directory_(QFileInfo(std::move(project_directory))
                             .canonicalFilePath()),
      font_root_(project_directory_ + QStringLiteral("/Assets/Fonts")) {}

refusion::core::FontAssetResolution
QtProjectFontAssetResolver::resolve_font_asset(
    const refusion::core::FontAssetRequest& request) {
  if (project_directory_.isEmpty() || !portable_asset_id(request.asset_id)) {
    return rejected("RFX-FONT-ASSET-ID-001",
                    "Font AssetId is not a portable project identifier");
  }
  const auto path =
      font_root_ + QLatin1Char('/') +
      QString::fromStdString(request.asset_id) + QStringLiteral("/font.ttf");
  const QFileInfo file_info(path);
  const auto canonical_path = file_info.canonicalFilePath();
  const auto canonical_root = QFileInfo(font_root_).canonicalFilePath();
  if (canonical_root.isEmpty() || canonical_path.isEmpty() ||
      !canonical_path.startsWith(canonical_root + QLatin1Char('/'))) {
    return rejected("RFX-FONT-ASSET-MISSING-001",
                    "Packaged Font asset is missing from Assets/Fonts");
  }

  QFile input(canonical_path);
  if (!input.open(QIODevice::ReadOnly)) {
    return rejected("RFX-FONT-ASSET-READ-001",
                    "Packaged Font asset could not be opened");
  }
  const auto payload = input.readAll();
  auto bytes = std::make_shared<std::vector<std::uint8_t>>(
      reinterpret_cast<const std::uint8_t*>(payload.constData()),
      reinterpret_cast<const std::uint8_t*>(payload.constData()) +
          payload.size());
  const auto actual_digest = refusion::core::sha256_content_digest(*bytes);
  if (actual_digest != request.expected_content_digest) {
    return rejected("RFX-FONT-ASSET-DIGEST-001",
                    "Packaged Font bytes differ from Project.rfx identity");
  }
  return {.asset = refusion::core::FontAssetBlob{
              .bytes = std::move(bytes),
              .verified_content_digest = actual_digest,
              .face_index = request.face_index,
          }};
}
