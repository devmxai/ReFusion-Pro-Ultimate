#pragma once

#include <cstdint>
#include <vector>

#include "refusion/runtime/media/HardwareVideoDecode.hpp"

namespace refusion_test {

[[nodiscard]] inline refusion::runtime::media::CompressedSampleDescriptor
long_gop_sample(const std::uint64_t access_unit,
                const std::uint64_t source_frame, const std::int64_t pts,
                const std::int64_t dts, const std::int64_t duration,
                const bool sync) {
  return {
      .access_unit_index = access_unit,
      .source_frame_index = source_frame,
      .timing =
          {
              .presentation_time = {.value = pts, .timescale = 30'000},
              .duration = {.value = duration, .timescale = 30'000},
          },
      .decode_time = {.value = dts, .timescale = 30'000},
      .sync_sample = sync,
  };
}

// Mirrors fixture.json exactly. The vector order is compressed decode order;
// source_frame and PTS identity are presentation order.
[[nodiscard]] inline std::vector<
    refusion::runtime::media::CompressedSampleDescriptor>
long_gop_samples() {
  return {
      long_gop_sample(0, 0, 90'000, 89'000, 1'000, true),
      long_gop_sample(1, 4, 94'000, 90'000, 1'000, false),
      long_gop_sample(2, 2, 92'000, 91'000, 1, false),
      long_gop_sample(3, 1, 91'001, 91'001, 1'999, false),
      long_gop_sample(4, 3, 93'000, 93'000, 1'000, false),
      long_gop_sample(5, 8, 98'000, 94'000, 1'000, false),
      long_gop_sample(6, 6, 96'000, 95'000, 1, false),
      long_gop_sample(7, 5, 95'001, 95'001, 1'999, false),
      long_gop_sample(8, 7, 97'000, 97'000, 1'000, false),
      long_gop_sample(9, 10, 100'000, 98'000, 1'000, false),
      long_gop_sample(10, 9, 99'000, 99'000, 1'000, false),
      long_gop_sample(11, 11, 101'000, 100'000, 1'000, true),
      long_gop_sample(12, 13, 103'000, 101'000, 1'000, false),
      long_gop_sample(13, 12, 102'000, 102'000, 1'000, false),
      long_gop_sample(14, 15, 105'000, 103'000, 1'000, false),
      long_gop_sample(15, 14, 104'000, 104'000, 1'000, false),
  };
}

}  // namespace refusion_test
