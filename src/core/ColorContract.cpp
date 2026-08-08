#include "refusion/core/ColorContract.hpp"

#include "refusion/core/ContentDigest.hpp"

#include <span>

namespace refusion::core {
namespace {

constexpr VisualColorContract desktop_v1_sdr{
    .profile_id = "refusion.color.desktop-v1-sdr.v1",
    .schema_version = 1,
    .authored_encoding = AuthoredColorEncoding::srgb_unorm8,
    .primaries = ColorPrimaries::rec709_d65,
    .transfer = ColorTransferFunction::srgb,
    .project_alpha = ProjectAlphaModel::straight,
    .compositing_alpha = CompositingAlphaModel::premultiplied,
    .blend_filter_working_space = BlendFilterWorkingSpace::srgb_encoded,
    .gradient_interpolation =
        GradientInterpolationPolicy::srgb_straight,
    .filter_edge = FilterEdgePolicy::transparent_decal,
    .target_format = VisualTargetFormat::bgra8_unorm,
    .output_transfer = OutputTransferFunction::srgb,
};

constexpr std::string_view canonical_receipt =
    "profile_id=refusion.color.desktop-v1-sdr.v1\n"
    "schema_version=1\n"
    "authored_encoding=srgb-unorm8\n"
    "primaries=rec709-d65\n"
    "transfer=srgb\n"
    "project_alpha=straight\n"
    "compositing_alpha=premultiplied\n"
    "blend_filter_working_space=srgb-encoded\n"
    "gradient_interpolation=srgb-straight\n"
    "filter_edge=transparent-decal\n"
    "target_format=bgra8-unorm\n"
    "output_transfer=srgb\n";

}  // namespace

const VisualColorContract& desktop_v1_sdr_color_contract() noexcept {
  return desktop_v1_sdr;
}

std::string_view desktop_v1_sdr_color_contract_canonical_bytes() noexcept {
  return canonical_receipt;
}

const std::string& desktop_v1_sdr_color_contract_digest() {
  static const std::string digest = [] {
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(
        canonical_receipt.data());
    return sha256_content_digest(
        std::span<const std::uint8_t>(bytes, canonical_receipt.size()));
  }();
  return digest;
}

bool is_desktop_v1_sdr_color_contract(
    const VisualColorContract& contract) noexcept {
  return contract == desktop_v1_sdr;
}

}  // namespace refusion::core
