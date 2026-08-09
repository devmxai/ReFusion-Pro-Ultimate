#include "refusion/core/MediaIndex.hpp"

#include "refusion/core/ContentDigest.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

using namespace refusion::core;

void require(const bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

[[nodiscard]] std::string digest(const char fill) {
  return "sha256:" + std::string(64, fill);
}

[[nodiscard]] MediaIndex sample_index() {
  MediaIndex index{
      .contract_version = 1,
      .source_digest = digest('a'),
      .source_byte_size = 20'000,
      .container_profile = MediaContainerProfile::iso_bmff_mp4,
  };
  index.streams.push_back(MediaStreamDescriptor{
      .stream_id = MediaStreamId{"stream_video"},
      .container_track_id = 1,
      .kind = MediaStreamKind::video,
      .codec = MediaCodec::h264_avc,
      .codec_configuration_digest = digest('b'),
      .time_base = MediaTimeBase{.numerator = 1, .denominator = 30'000},
      .start = 15'990,
      .duration = 6'000,
      .format = VideoStreamFormat{
          .coded_extent =
              CanvasExtent{.width_pixels = 640, .height_pixels = 360},
          .display_extent =
              CanvasExtent{.width_pixels = 640, .height_pixels = 360},
          .presentation_rate =
              RationalRate{.numerator = 30'000, .denominator = 1'001},
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
  index.streams.push_back(MediaStreamDescriptor{
      .stream_id = MediaStreamId{"stream_audio"},
      .container_track_id = 2,
      .kind = MediaStreamKind::audio,
      .codec = MediaCodec::aac_lc,
      .codec_configuration_digest = digest('c'),
      .time_base = MediaTimeBase{.numerator = 1, .denominator = 48'000},
      .start = 28'944,
      .duration = 4'096,
      .format = AudioStreamFormat{.sample_rate_hz = 48'000, .channels = 2},
  });
  const std::vector<std::uint8_t> video_configuration{1, 100, 0, 40};
  const std::vector<std::uint8_t> audio_configuration{0x11, 0x88};
  index.streams.at(0).codec_configuration_digest =
      sha256_content_digest(video_configuration);
  index.streams.at(1).codec_configuration_digest =
      sha256_content_digest(audio_configuration);
  index.codec_configurations = {
      MediaCodecConfiguration{
          .stream_id = MediaStreamId{"stream_video"},
          .sample_description_index = 1,
          .content_digest = index.streams.at(0).codec_configuration_digest,
          .bytes = video_configuration,
      },
      MediaCodecConfiguration{
          .stream_id = MediaStreamId{"stream_audio"},
          .sample_description_index = 1,
          .content_digest = index.streams.at(1).codec_configuration_digest,
          .bytes = audio_configuration,
      },
  };

  const SampleDependencyFlags independent{
      .leading = SampleDependencyValue::no,
      .depends_on_others = SampleDependencyValue::no,
      .is_depended_on = SampleDependencyValue::yes,
      .has_redundancy = SampleDependencyValue::no,
  };
  const SampleDependencyFlags dependent{
      .leading = SampleDependencyValue::no,
      .depends_on_others = SampleDependencyValue::yes,
      .is_depended_on = SampleDependencyValue::unknown,
      .has_redundancy = SampleDependencyValue::no,
  };
  index.samples_decode_order = {
      CompressedSample{
          .stream_id = MediaStreamId{"stream_video"},
          .sample_index = 0,
          .byte_offset = 1'000,
          .byte_size = 400,
          .presentation_timestamp = 15'990,
          .decode_timestamp = 13'990,
          .duration = 1'000,
          .time_base = MediaTimeBase{.numerator = 1, .denominator = 30'000},
          .sync_sample = true,
          .dependencies = independent,
          .sample_description_index = 1,
      },
      CompressedSample{
          .stream_id = MediaStreamId{"stream_audio"},
          .sample_index = 0,
          .byte_offset = 1'400,
          .byte_size = 200,
          .presentation_timestamp = 28'944,
          .decode_timestamp = 28'944,
          .duration = 1'024,
          .time_base = MediaTimeBase{.numerator = 1, .denominator = 48'000},
          .sync_sample = true,
          .dependencies = independent,
          .sample_description_index = 1,
      },
      CompressedSample{
          .stream_id = MediaStreamId{"stream_video"},
          .sample_index = 1,
          .byte_offset = 1'600,
          .byte_size = 350,
          .presentation_timestamp = 18'990,
          .decode_timestamp = 14'990,
          .duration = 1'000,
          .time_base = MediaTimeBase{.numerator = 1, .denominator = 30'000},
          .sync_sample = false,
          .dependencies = dependent,
          .sample_description_index = 1,
      },
      CompressedSample{
          .stream_id = MediaStreamId{"stream_audio"},
          .sample_index = 1,
          .byte_offset = 1'950,
          .byte_size = 200,
          .presentation_timestamp = 29'968,
          .decode_timestamp = 29'968,
          .duration = 1'024,
          .time_base = MediaTimeBase{.numerator = 1, .denominator = 48'000},
          .sync_sample = true,
          .dependencies = independent,
          .sample_description_index = 1,
      },
      CompressedSample{
          .stream_id = MediaStreamId{"stream_video"},
          .sample_index = 2,
          .byte_offset = 2'150,
          .byte_size = 300,
          .presentation_timestamp = 16'990,
          .decode_timestamp = 15'990,
          .duration = 2'000,
          .time_base = MediaTimeBase{.numerator = 1, .denominator = 30'000},
          .sync_sample = false,
          .dependencies = dependent,
          .sample_description_index = 1,
      },
      CompressedSample{
          .stream_id = MediaStreamId{"stream_audio"},
          .sample_index = 2,
          .byte_offset = 2'450,
          .byte_size = 180,
          .presentation_timestamp = 30'992,
          .decode_timestamp = 30'992,
          .duration = 1'024,
          .time_base = MediaTimeBase{.numerator = 1, .denominator = 48'000},
          .sync_sample = true,
          .dependencies = independent,
          .sample_description_index = 1,
      },
  };
  return index;
}

}  // namespace

int main() {
  using namespace refusion::core;

  const auto index = sample_index();
  const auto validation = validate_media_index(index);
  require(validation.valid, validation.code + ": " + validation.message);
  const auto canonical = canonical_media_index_bytes(index);
  require(canonical == canonical_media_index_bytes(index),
          "MediaIndex canonical bytes are not stable");
  const auto digest_value = media_index_digest(index);
  std::cout << "media_index_digest=" << digest_value << '\n';
  require(digest_value ==
              "sha256:c85d1ba60f7109f9447d455a19e9d8d3fd1df22fc57ae4083a358e3cb7585ab7",
          "MediaIndex differs from the AppleClang/MSVC canonical receipt");

  auto outside_source = index;
  outside_source.samples_decode_order.front().byte_offset = 19'900;
  outside_source.samples_decode_order.front().byte_size = 200;
  require(!validate_media_index(outside_source).valid,
          "sample outside immutable source bytes was admitted");

  auto missing_sample = index;
  missing_sample.samples_decode_order.back().sample_index = 3;
  require(!validate_media_index(missing_sample).valid,
          "non-contiguous per-stream sample index was admitted");

  auto overlapping = index;
  overlapping.samples_decode_order.at(1).byte_offset = 1'300;
  require(!validate_media_index(overlapping).valid,
          "overlapping compressed sample ranges were admitted");

  auto wrong_time_base = index;
  wrong_time_base.samples_decode_order.front().time_base.denominator = 90'000;
  require(!validate_media_index(wrong_time_base).valid,
          "sample/Stream time-base mismatch was admitted");

  auto presentation_outside_stream = index;
  presentation_outside_stream.samples_decode_order.front()
      .presentation_timestamp = 15'000;
  require(!validate_media_index(presentation_outside_stream).valid,
          "sample PTS outside Stream range was admitted");

  auto changed_configuration = index;
  changed_configuration.codec_configurations.front().bytes.front() ^= 0xffU;
  require(!validate_media_index(changed_configuration).valid,
          "codec configuration bytes differing from their digest were admitted");

  std::cout << "media index tests passed\n";
  return 0;
}
