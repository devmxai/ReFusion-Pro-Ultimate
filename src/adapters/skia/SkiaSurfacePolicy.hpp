#pragma once

#include "include/core/SkImageInfo.h"
#include "include/core/SkSurfaceProps.h"

#include <cstdint>

namespace refusion::adapters::skia {

// One surface policy is used by every Skia backend. Unknown pixel geometry
// prevents platform subpixel assumptions from entering project Canvas text;
// final quantization uses the shared presentation shader, not backend dithering.
[[nodiscard]] const SkSurfaceProps& visual_surface_props() noexcept;

// Desktop-v1 keeps its accepted sRGB-encoded compositing semantics while the
// intermediate uses floating-point channels to avoid early 8-bit quantization.
[[nodiscard]] SkImageInfo composition_surface_info(
    std::uint32_t width_pixels, std::uint32_t height_pixels);

}  // namespace refusion::adapters::skia
