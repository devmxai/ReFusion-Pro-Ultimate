#include "refusion/runtime/media/MediaIndexDecodeProjection.hpp"

#include "refusion/core/ContentDigest.hpp"

#include <cstdint>
#include <source_location>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace refusion::core;
using namespace refusion::runtime::media;

void require(const bool condition,
             const std::source_location location =
                 std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error("MediaIndex decode projection failure at line " +
                             std::to_string(location.line()));
  }
}

[[nodiscard]] MediaIndex b_frame_index() {
  const std::vector<std::uint8_t> configuration{1, 100, 0, 40};
  const auto configuration_digest = sha256_content_digest(configuration);
  MediaIndex index{
      .contract_version = 1,
      .source_digest = sha256_content_digest(
          std::vector<std::uint8_t>(64, 0x5a)),
      .source_byte_size = 64,
      .container_profile = MediaContainerProfile::iso_bmff_mp4,
  };
  index.streams.push_back(MediaStreamDescriptor{
      .stream_id = MediaStreamId{"stream_video"},
      .container_track_id = 1,
      .kind = MediaStreamKind::video,
      .codec = MediaCodec::h264_avc,
      .codec_configuration_digest = configuration_digest,
      .time_base = {.numerator = 1, .denominator = 30'000},
      .start = 90'000,
      .duration = 3'000,
      .format = VideoStreamFormat{
          .coded_extent = {.width_pixels = 640, .height_pixels = 360},
          .display_extent = {.width_pixels = 640, .height_pixels = 360},
          .presentation_rate = {.numerator = 30, .denominator = 1},
          .bit_depth = 8,
          .chroma_subsampling_x = 2,
          .chroma_subsampling_y = 2,
          .color_range = MediaColorRange::video,
          .color_primaries = "bt709",
          .color_transfer = "bt709",
          .color_matrix = "bt709",
          .orientation_degrees = 0,
          .sample_aspect_numerator = 1,
          .sample_aspect_denominator = 1,
      },
  });
  index.codec_configurations.push_back(MediaCodecConfiguration{
      .stream_id = MediaStreamId{"stream_video"},
      .sample_description_index = 1,
      .content_digest = configuration_digest,
      .bytes = configuration,
  });
  const auto add_sample = [&index](const std::uint64_t decode_order,
                                   const std::int64_t pts,
                                   const std::int64_t dts,
                                   const std::uint64_t byte_offset,
                                   const bool sync) {
    index.samples_decode_order.push_back(CompressedSample{
        .stream_id = MediaStreamId{"stream_video"},
        .sample_index = decode_order,
        .byte_offset = byte_offset,
        .byte_size = 10,
        .presentation_timestamp = pts,
        .decode_timestamp = dts,
        .duration = 1'000,
        .time_base = {.numerator = 1, .denominator = 30'000},
        .sync_sample = sync,
        .sample_description_index = 1,
    });
  };
  add_sample(0, 90'000, 89'000, 0, true);
  add_sample(1, 92'000, 90'000, 10, false);
  add_sample(2, 91'000, 91'000, 20, false);
  return index;
}

}  // namespace

int main() {
  auto index = b_frame_index();
  const auto result = project_media_index_for_hardware_decode(
      index, MediaStreamId{"stream_video"});
  require(result.succeeded());
  const auto& projection = *result.projection;
  require(projection.source_digest == index.source_digest);
  require(projection.expected_profile.coded_width == 640 &&
          projection.expected_profile.coded_height == 360);
  require(projection.codec_configuration ==
          index.codec_configurations.front().bytes);
  require(projection.samples_decode_order.size() == 3);
  require(projection.samples_decode_order[0].source_frame_index == 0);
  require(projection.samples_decode_order[1].source_frame_index == 2);
  require(projection.samples_decode_order[2].source_frame_index == 1);
  require(projection.samples_decode_order[1].source_byte_offset == 10 &&
          projection.samples_decode_order[1].source_byte_size == 10);
  require(HardwareDecodeSequenceRequest{
              .source_path = "opaque-test-token",
              .expected_profile = projection.expected_profile,
              .samples = projection.samples_decode_order,
          }
              .valid());

  index.samples_decode_order[1].presentation_timestamp = 91'000;
  const auto duplicate = project_media_index_for_hardware_decode(
      index, MediaStreamId{"stream_video"});
  require(duplicate.state ==
              MediaIndexDecodeProjectionState::ambiguous_presentation_order &&
          !duplicate.projection.has_value());

  const auto missing = project_media_index_for_hardware_decode(
      b_frame_index(), MediaStreamId{"stream_missing"});
  require(missing.state == MediaIndexDecodeProjectionState::stream_missing);
}
