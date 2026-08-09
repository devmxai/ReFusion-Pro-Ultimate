#include "SkiaSurfacePolicy.hpp"

#include "include/core/SkColorSpace.h"

#include <limits>
#include <stdexcept>

namespace refusion::adapters::skia {

const SkSurfaceProps& visual_surface_props() noexcept {
  static const SkSurfaceProps properties{
      SkSurfaceProps::kUseDeviceIndependentFonts_Flag,
      kUnknown_SkPixelGeometry};
  return properties;
}

SkImageInfo composition_surface_info(const std::uint32_t width_pixels,
                                     const std::uint32_t height_pixels) {
  constexpr auto kMaximumDimension =
      static_cast<std::uint32_t>(std::numeric_limits<int>::max());
  if (width_pixels == 0 || height_pixels == 0 ||
      width_pixels > kMaximumDimension || height_pixels > kMaximumDimension) {
    throw std::invalid_argument(
        "RFX-CANVAS-SURFACE-002: invalid Composition surface dimensions");
  }
  return SkImageInfo::Make(
      static_cast<int>(width_pixels), static_cast<int>(height_pixels),
      kRGBA_F16_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
}

}  // namespace refusion::adapters::skia
