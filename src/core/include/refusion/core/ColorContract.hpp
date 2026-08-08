#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace refusion::core {

enum class AuthoredColorEncoding : std::uint8_t {
  srgb_unorm8 = 1,
};

enum class ColorPrimaries : std::uint8_t {
  rec709_d65 = 1,
};

enum class ColorTransferFunction : std::uint8_t {
  srgb = 1,
};

enum class ProjectAlphaModel : std::uint8_t {
  straight = 1,
};

enum class CompositingAlphaModel : std::uint8_t {
  premultiplied = 1,
};

enum class BlendFilterWorkingSpace : std::uint8_t {
  srgb_encoded = 1,
};

enum class GradientInterpolationPolicy : std::uint8_t {
  srgb_straight = 1,
};

enum class FilterEdgePolicy : std::uint8_t {
  transparent_decal = 1,
};

enum class VisualTargetFormat : std::uint8_t {
  bgra8_unorm = 1,
};

enum class OutputTransferFunction : std::uint8_t {
  srgb = 1,
};

// Portable visual color meaning. This value contains no Skia, Metal, D3D or
// Vulkan object and is carried into every accepted VisualRenderPlan.
struct VisualColorContract final {
  std::string_view profile_id;
  std::uint32_t schema_version{0};
  AuthoredColorEncoding authored_encoding{};
  ColorPrimaries primaries{};
  ColorTransferFunction transfer{};
  ProjectAlphaModel project_alpha{};
  CompositingAlphaModel compositing_alpha{};
  BlendFilterWorkingSpace blend_filter_working_space{};
  GradientInterpolationPolicy gradient_interpolation{};
  FilterEdgePolicy filter_edge{};
  VisualTargetFormat target_format{};
  OutputTransferFunction output_transfer{};

  friend constexpr bool operator==(const VisualColorContract&,
                                   const VisualColorContract&) = default;
};

[[nodiscard]] const VisualColorContract&
desktop_v1_sdr_color_contract() noexcept;

// Canonical ASCII bytes and their SHA-256 bind the contract to project/
// RenderPlan/toolchain receipts. Backends may not replace individual fields
// with API defaults while retaining this identity.
[[nodiscard]] std::string_view
desktop_v1_sdr_color_contract_canonical_bytes() noexcept;

[[nodiscard]] const std::string&
desktop_v1_sdr_color_contract_digest();

[[nodiscard]] bool is_desktop_v1_sdr_color_contract(
    const VisualColorContract& contract) noexcept;

}  // namespace refusion::core
