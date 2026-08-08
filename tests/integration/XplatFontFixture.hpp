#pragma once

#include "refusion/core/ContentDigest.hpp"
#include "refusion/core/FontAssetResolver.hpp"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace refusion::tests {

inline constexpr const char* kXplatFontAssetId =
    "font_noto_sans_arabic_regular";
inline constexpr const char* kXplatFontDigest =
    "sha256:7ed3fe069312aceac454f17cf613a30f95271d6ed7ce58005ed4d016bd3823d7";

class XplatFontFixtureResolver final : public core::FontAssetResolverPort {
 public:
  [[nodiscard]] core::FontAssetResolution resolve_font_asset(
      const core::FontAssetRequest& request) override {
    if (request.asset_id != kXplatFontAssetId ||
        request.expected_content_digest != kXplatFontDigest ||
        request.face_index != 0) {
      return {
          .diagnostic_code = "RFX-XPLAT-FONT-IDENTITY-001",
          .diagnostic_message =
              "qualification requested a font outside the pinned fixture",
      };
    }
    std::ifstream input(REFUSION_NOTO_SANS_ARABIC_PATH, std::ios::binary);
    if (!input) {
      return {
          .diagnostic_code = "RFX-XPLAT-FONT-MISSING-001",
          .diagnostic_message = "the pinned qualification font is missing",
      };
    }
    auto bytes = std::make_shared<std::vector<std::uint8_t>>(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
    const auto digest = core::sha256_content_digest(*bytes);
    if (digest != kXplatFontDigest) {
      return {
          .diagnostic_code = "RFX-XPLAT-FONT-DIGEST-001",
          .diagnostic_message = "the pinned qualification font bytes changed",
      };
    }
    return {
        .asset = core::FontAssetBlob{
            .bytes = std::move(bytes),
            .verified_content_digest = digest,
            .face_index = 0,
        },
    };
  }
};

[[nodiscard]] inline std::shared_ptr<core::FontAssetResolverPort>
make_xplat_font_fixture_resolver() {
  return std::make_shared<XplatFontFixtureResolver>();
}

}  // namespace refusion::tests
