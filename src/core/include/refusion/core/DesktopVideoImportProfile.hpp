#pragma once

#include <cstdint>

namespace refusion::core::desktop_video_import {

// Portable intake bounds shared by every Desktop adapter. Native decoders may
// fail closed for an unavailable device, but may not redefine these semantics.
inline constexpr std::uint8_t maximum_h264_level_idc = 52;
inline constexpr std::uint32_t maximum_coded_dimension = 3'840;
inline constexpr std::uint64_t maximum_coded_pixels = 3'840ULL * 2'160ULL;
inline constexpr std::uint64_t maximum_bitrate_bits_per_second = 50'000'000ULL;
inline constexpr std::uint32_t maximum_presentation_frames_per_second = 60;

[[nodiscard]] constexpr bool admitted_h264_profile_idc(
    const std::uint8_t profile_idc) noexcept {
  return profile_idc == 66 || profile_idc == 77 || profile_idc == 100;
}

}  // namespace refusion::core::desktop_video_import
