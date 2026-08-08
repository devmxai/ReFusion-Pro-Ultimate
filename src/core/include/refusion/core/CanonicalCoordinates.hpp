#pragma once

#include "refusion/core/ProjectDocument.hpp"

namespace refusion::core {

// Derived/UI-authored pixel coordinates enter accepted project truth on this
// binary-exact grid. Explicit Project.rfx literals are preserved as authored;
// the grid prevents platform math from committing infinitesimally different
// alignment/measurement results.
inline constexpr double kAuthoredSubpixelsPerPixel = 1024.0;
inline constexpr double kAuthoredPixelQuantum =
    1.0 / kAuthoredSubpixelsPerPixel;

[[nodiscard]] double quantize_authored_pixel(double value) noexcept;
[[nodiscard]] bool is_quantized_authored_pixel(double value) noexcept;

[[nodiscard]] Transform2D quantize_transform_pixels(
    const Transform2D& transform) noexcept;

}  // namespace refusion::core
