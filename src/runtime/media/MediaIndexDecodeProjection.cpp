#include "refusion/runtime/media/MediaIndexDecodeProjection.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

namespace refusion::runtime::media {
namespace {

[[nodiscard]] MediaIndexDecodeProjectionResult rejected(
    const MediaIndexDecodeProjectionState state, std::string code,
    std::string diagnostic) {
  return {
      .state = state,
      .projection = std::nullopt,
      .code = std::move(code),
      .diagnostic = std::move(diagnostic),
  };
}

[[nodiscard]] bool checked_time_value(const std::int64_t ticks,
                                      const std::int64_t numerator,
                                      std::int64_t& value) noexcept {
  if (ticks > 0 && numerator >
                       std::numeric_limits<std::int64_t>::max() / ticks) {
    return false;
  }
  if (ticks < 0 && ticks <
                       std::numeric_limits<std::int64_t>::min() / numerator) {
    return false;
  }
  value = ticks * numerator;
  return true;
}

[[nodiscard]] bool checked_duration_value(const std::uint64_t ticks,
                                          const std::int64_t numerator,
                                          std::int64_t& value) noexcept {
  if (ticks > static_cast<std::uint64_t>(
                  std::numeric_limits<std::int64_t>::max() / numerator)) {
    return false;
  }
  value = static_cast<std::int64_t>(ticks) * numerator;
  return true;
}

}  // namespace

bool MediaIndexVideoDecodeProjection::valid() const noexcept {
  if (source_digest.empty() || source_byte_size == 0 || stream_id.value.empty() ||
      !expected_profile.valid() || codec_configuration_digest.empty() ||
      codec_configuration.empty() || samples_decode_order.empty()) {
    return false;
  }
  return std::all_of(samples_decode_order.begin(), samples_decode_order.end(),
                     [this](const auto& sample) {
                       return sample.valid() && sample.source_byte_size > 0 &&
                              sample.source_byte_offset <= source_byte_size &&
                              sample.source_byte_size <=
                                  source_byte_size - sample.source_byte_offset &&
                              sample.sample_description_index == 1;
                     });
}

bool MediaIndexDecodeProjectionResult::succeeded() const noexcept {
  return state == MediaIndexDecodeProjectionState::ready && projection &&
         projection->valid();
}

MediaIndexDecodeProjectionResult project_media_index_for_hardware_decode(
    const core::MediaIndex& index,
    const core::MediaStreamId& video_stream_id) {
  const auto validation = core::validate_media_index(index);
  if (!validation.valid) {
    return rejected(MediaIndexDecodeProjectionState::invalid_index,
                    "RFX-MEDIA-DECODE-INDEX-INVALID",
                    validation.code + ": " + validation.message);
  }

  const auto stream_iterator = std::find_if(
      index.streams.begin(), index.streams.end(),
      [&video_stream_id](const core::MediaStreamDescriptor& stream) {
        return stream.stream_id == video_stream_id;
      });
  if (stream_iterator == index.streams.end()) {
    return rejected(MediaIndexDecodeProjectionState::stream_missing,
                    "RFX-MEDIA-DECODE-STREAM-MISSING",
                    "selected Video Stream is absent from MediaIndex");
  }
  if (stream_iterator->kind != core::MediaStreamKind::video ||
      !std::holds_alternative<core::VideoStreamFormat>(
          stream_iterator->format)) {
    return rejected(MediaIndexDecodeProjectionState::stream_not_video,
                    "RFX-MEDIA-DECODE-STREAM-NOT-VIDEO",
                    "selected MediaIndex Stream is not Video");
  }

  const auto& format = std::get<core::VideoStreamFormat>(stream_iterator->format);
  if (stream_iterator->codec != core::MediaCodec::h264_avc ||
      format.bit_depth != 8 || format.chroma_subsampling_x != 2 ||
      format.chroma_subsampling_y != 2 ||
      format.color_range != core::MediaColorRange::video ||
      format.color_primaries != "bt709" || format.color_transfer != "bt709" ||
      format.color_matrix != "bt709") {
    return rejected(MediaIndexDecodeProjectionState::unsupported_profile,
                    "RFX-MEDIA-DECODE-PROFILE-UNSUPPORTED",
                    "selected Video Stream is outside the strict AVC/NV12/Rec.709 profile");
  }
  if (stream_iterator->time_base.denominator >
      std::numeric_limits<std::int32_t>::max()) {
    return rejected(MediaIndexDecodeProjectionState::timing_not_representable,
                    "RFX-MEDIA-DECODE-TIME-UNREPRESENTABLE",
                    "MediaIndex time base exceeds the exact decoder time domain");
  }

  const auto configuration_iterator = std::find_if(
      index.codec_configurations.begin(), index.codec_configurations.end(),
      [&video_stream_id](const core::MediaCodecConfiguration& configuration) {
        return configuration.stream_id == video_stream_id &&
               configuration.sample_description_index == 1;
      });
  if (configuration_iterator == index.codec_configurations.end()) {
    return rejected(MediaIndexDecodeProjectionState::invalid_index,
                    "RFX-MEDIA-DECODE-CONFIGURATION-MISSING",
                    "selected Video Stream has no codec configuration");
  }

  std::vector<const core::CompressedSample*> video_samples;
  for (const auto& sample : index.samples_decode_order) {
    if (sample.stream_id == video_stream_id) video_samples.push_back(&sample);
  }
  if (video_samples.empty()) {
    return rejected(MediaIndexDecodeProjectionState::invalid_index,
                    "RFX-MEDIA-DECODE-SAMPLES-MISSING",
                    "selected Video Stream has no compressed samples");
  }

  auto presentation_order = video_samples;
  std::sort(presentation_order.begin(), presentation_order.end(),
            [](const auto* left, const auto* right) {
              if (left->presentation_timestamp != right->presentation_timestamp) {
                return left->presentation_timestamp < right->presentation_timestamp;
              }
              return left->sample_index < right->sample_index;
            });
  std::unordered_map<std::uint64_t, std::uint64_t> source_frame_indices;
  source_frame_indices.reserve(presentation_order.size());
  for (std::size_t offset = 0; offset < presentation_order.size(); ++offset) {
    if (presentation_order[offset]->discard_sample ||
        (offset > 0 &&
         presentation_order[offset - 1]->presentation_timestamp ==
             presentation_order[offset]->presentation_timestamp)) {
      return rejected(
          MediaIndexDecodeProjectionState::ambiguous_presentation_order,
          "RFX-MEDIA-DECODE-PRESENTATION-AMBIGUOUS",
          "Video samples require unique PTS and no discard-only access units in the first profile");
    }
    source_frame_indices.emplace(presentation_order[offset]->sample_index,
                                 static_cast<std::uint64_t>(offset));
  }

  MediaIndexVideoDecodeProjection projection{
      .source_digest = index.source_digest,
      .source_byte_size = index.source_byte_size,
      .stream_id = video_stream_id,
      .expected_profile =
          {
              .codec = VideoCodec::h264_avc,
              .pixel_format = VideoPixelFormat::nv12_8bit_video_range,
              .coded_width = format.coded_extent.width_pixels,
              .coded_height = format.coded_extent.height_pixels,
              .color =
                  {
                      .primaries = ColorPrimaries::bt709,
                      .transfer = TransferFunction::bt709,
                      .matrix = MatrixCoefficients::bt709,
                      .full_range = false,
                  },
          },
      .codec_configuration_digest = configuration_iterator->content_digest,
      .codec_configuration = configuration_iterator->bytes,
  };
  projection.samples_decode_order.reserve(video_samples.size());
  for (const auto* sample : video_samples) {
    std::int64_t presentation_value = 0;
    std::int64_t decode_value = 0;
    std::int64_t duration_value = 0;
    if (!checked_time_value(sample->presentation_timestamp,
                            sample->time_base.numerator, presentation_value) ||
        !checked_time_value(sample->decode_timestamp,
                            sample->time_base.numerator, decode_value) ||
        !checked_duration_value(sample->duration,
                                sample->time_base.numerator, duration_value)) {
      return rejected(MediaIndexDecodeProjectionState::timing_not_representable,
                      "RFX-MEDIA-DECODE-TIME-UNREPRESENTABLE",
                      "compressed sample timing overflows the exact decoder time domain");
    }
    projection.samples_decode_order.push_back(CompressedSampleDescriptor{
        .access_unit_index = sample->sample_index,
        .source_frame_index = source_frame_indices.at(sample->sample_index),
        .timing =
            {
                .presentation_time =
                    {.value = presentation_value,
                     .timescale = static_cast<std::int32_t>(
                         sample->time_base.denominator)},
                .duration =
                    {.value = duration_value,
                     .timescale = static_cast<std::int32_t>(
                         sample->time_base.denominator)},
            },
        .decode_time =
            {.value = decode_value,
             .timescale = static_cast<std::int32_t>(
                 sample->time_base.denominator)},
        .sync_sample = sample->sync_sample,
        .source_byte_offset = sample->byte_offset,
        .source_byte_size = sample->byte_size,
        .sample_description_index = sample->sample_description_index,
    });
  }

  if (!projection.valid()) {
    return rejected(MediaIndexDecodeProjectionState::invalid_index,
                    "RFX-MEDIA-DECODE-PROJECTION-INVALID",
                    "MediaIndex produced an invalid hardware decode projection");
  }
  return {
      .state = MediaIndexDecodeProjectionState::ready,
      .projection = std::move(projection),
      .code = "RFX-MEDIA-DECODE-PROJECTION-READY",
      .diagnostic =
          "shared MediaIndex projected into exact decode-order scheduler input",
  };
}

}  // namespace refusion::runtime::media
