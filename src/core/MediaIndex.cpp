#include "refusion/core/MediaIndex.hpp"

#include "refusion/core/CanonicalText.hpp"
#include "refusion/core/ContentDigest.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace refusion::core {
namespace {

[[nodiscard]] CompositionValidation rejected(std::string code,
                                             std::string message) {
  return CompositionValidation{
      .valid = false,
      .code = std::move(code),
      .message = std::move(message),
  };
}

[[nodiscard]] bool sha256_digest(const std::string_view value) noexcept {
  if (!value.starts_with("sha256:") || value.size() != 71) return false;
  return std::all_of(value.begin() + 7, value.end(), [](const char character) {
    return (character >= '0' && character <= '9') ||
           (character >= 'a' && character <= 'f');
  });
}

[[nodiscard]] bool dependency_value_valid(
    const SampleDependencyValue value) noexcept {
  switch (value) {
    case SampleDependencyValue::unknown:
    case SampleDependencyValue::no:
    case SampleDependencyValue::yes:
      return true;
  }
  return false;
}

[[nodiscard]] std::string_view container_name(
    const MediaContainerProfile profile) {
  switch (profile) {
    case MediaContainerProfile::iso_bmff_mp4: return "iso_bmff_mp4";
    case MediaContainerProfile::quicktime_mov: return "quicktime_mov";
  }
  throw std::invalid_argument("unknown media container profile");
}

[[nodiscard]] std::string_view stream_kind_name(const MediaStreamKind kind) {
  switch (kind) {
    case MediaStreamKind::video: return "video";
    case MediaStreamKind::audio: return "audio";
  }
  throw std::invalid_argument("unknown media stream kind");
}

[[nodiscard]] std::string_view codec_name(const MediaCodec codec) {
  switch (codec) {
    case MediaCodec::h264_avc: return "h264_avc";
    case MediaCodec::aac_lc: return "aac_lc";
  }
  throw std::invalid_argument("unknown media codec");
}

[[nodiscard]] std::string_view dependency_name(
    const SampleDependencyValue value) {
  switch (value) {
    case SampleDependencyValue::unknown: return "unknown";
    case SampleDependencyValue::no: return "no";
    case SampleDependencyValue::yes: return "yes";
  }
  throw std::invalid_argument("unknown sample dependency value");
}

[[nodiscard]] std::string_view notice_name(
    const MediaIndexNoticeKind kind) {
  switch (kind) {
    case MediaIndexNoticeKind::bt709_transfer_defaulted:
      return "bt709_transfer_defaulted";
  }
  throw std::invalid_argument("unknown MediaIndex notice kind");
}

template <typename Integer>
void append_integer(std::string& output, const Integer value) {
  char buffer[32]{};
  const auto [end, error] =
      std::to_chars(std::begin(buffer), std::end(buffer), value);
  if (error != std::errc{}) {
    throw std::runtime_error("canonical MediaIndex integer conversion failed");
  }
  output.append(buffer, end);
}

void append_token(std::string& output, const std::string_view value) {
  append_integer(output, value.size());
  output.push_back(':');
  output.append(value);
}

void append_hex_bytes(std::string& output,
                      const std::span<const std::uint8_t> bytes) {
  constexpr std::string_view kHex = "0123456789abcdef";
  output.reserve(output.size() + bytes.size() * 2);
  for (const auto byte : bytes) {
    output.push_back(kHex[(byte >> 4U) & 0x0fU]);
    output.push_back(kHex[byte & 0x0fU]);
  }
}

}  // namespace

CompositionValidation validate_media_index(const MediaIndex& index) {
  constexpr std::uint64_t kMaximumSourceBytes = 8ULL * 1024ULL * 1024ULL *
                                                 1024ULL;
  if (index.contract_version != 1 || !sha256_digest(index.source_digest) ||
      index.source_byte_size == 0 ||
      index.source_byte_size > kMaximumSourceBytes) {
    return rejected("RFX-MEDIA-INDEX-001",
                    "MediaIndex source identity or contract version is invalid");
  }
  switch (index.container_profile) {
    case MediaContainerProfile::iso_bmff_mp4:
    case MediaContainerProfile::quicktime_mov:
      break;
    default:
      return rejected("RFX-MEDIA-INDEX-002",
                      "MediaIndex container profile is invalid");
  }
  if (index.streams.empty() || index.streams.size() > 32 ||
      index.codec_configurations.size() != index.streams.size() ||
      index.samples_decode_order.empty()) {
    return rejected("RFX-MEDIA-INDEX-003",
                    "MediaIndex streams or samples exceed admitted bounds");
  }

  std::unordered_set<std::string> stream_ids;
  std::unordered_set<std::uint32_t> track_ids;
  std::unordered_map<std::string, const MediaStreamDescriptor*> streams;
  for (const auto& stream : index.streams) {
    const auto validation = validate_media_stream_descriptor(stream);
    if (!validation.valid) return validation;
    if (!stream_ids.emplace(stream.stream_id.value).second ||
        !track_ids.emplace(stream.container_track_id).second) {
      return rejected("RFX-MEDIA-INDEX-004",
                      "MediaIndex Stream and track IDs must be unique");
    }
    streams.emplace(stream.stream_id.value, &stream);
  }
  std::unordered_set<std::string> notice_keys;
  for (const auto& notice : index.notices) {
    const auto stream = streams.find(notice.stream_id.value);
    const auto key = notice.stream_id.value + ":" +
                     std::string{notice_name(notice.kind)};
    if (stream == streams.end() ||
        stream->second->kind != MediaStreamKind::video ||
        !notice_keys.emplace(key).second) {
      return rejected(
          "RFX-MEDIA-INDEX-011",
          "MediaIndex notices must be unique and reference a Video stream");
    }
  }
  constexpr std::size_t kMaximumCodecConfigurationBytes = 1024U * 1024U;
  for (std::size_t offset = 0; offset < index.codec_configurations.size();
       ++offset) {
    const auto& configuration = index.codec_configurations[offset];
    const auto& stream = index.streams[offset];
    if (configuration.stream_id != stream.stream_id ||
        configuration.sample_description_index != 1 ||
        configuration.bytes.empty() ||
        configuration.bytes.size() > kMaximumCodecConfigurationBytes ||
        configuration.content_digest != stream.codec_configuration_digest ||
        configuration.content_digest !=
            sha256_content_digest(configuration.bytes)) {
      return rejected(
          "RFX-MEDIA-INDEX-010",
          "codec configuration bytes, identity or digest are invalid");
    }
  }

  struct SampleCursor final {
    std::uint64_t next_index{0};
    std::int64_t previous_dts{0};
    bool has_previous{false};
  };
  std::unordered_map<std::string, SampleCursor> cursors;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> byte_ranges;
  byte_ranges.reserve(index.samples_decode_order.size());
  for (const auto& sample : index.samples_decode_order) {
    const auto stream = streams.find(sample.stream_id.value);
    const auto stream_end =
        stream == streams.end()
            ? 0
            : stream->second->start +
                  static_cast<std::int64_t>(stream->second->duration);
    if (stream == streams.end() || sample.byte_size == 0 ||
        sample.duration == 0 || !sample.time_base.valid() ||
        sample.time_base != stream->second->time_base ||
        sample.sample_description_index != 1 ||
        !dependency_value_valid(sample.dependencies.leading) ||
        !dependency_value_valid(sample.dependencies.depends_on_others) ||
        !dependency_value_valid(sample.dependencies.is_depended_on) ||
        !dependency_value_valid(sample.dependencies.has_redundancy) ||
        sample.byte_offset > index.source_byte_size ||
        sample.byte_size > index.source_byte_size - sample.byte_offset ||
        sample.duration > static_cast<std::uint64_t>(
                              std::numeric_limits<std::int64_t>::max()) ||
        sample.presentation_timestamp >
            std::numeric_limits<std::int64_t>::max() -
                static_cast<std::int64_t>(sample.duration) ||
        sample.decode_timestamp > std::numeric_limits<std::int64_t>::max() -
                                      static_cast<std::int64_t>(sample.duration)) {
      return rejected("RFX-MEDIA-INDEX-005",
                      "compressed sample timing, dependency or byte range is invalid");
    }
    const auto presentation_end =
        sample.presentation_timestamp +
        static_cast<std::int64_t>(sample.duration);
    const bool inside_presentation =
        sample.presentation_timestamp >= stream->second->start &&
        presentation_end <= stream_end;
    const bool discard_outside_presentation =
        sample.discard_sample &&
        (presentation_end <= stream->second->start ||
         sample.presentation_timestamp >= stream_end);
    if (!inside_presentation && !discard_outside_presentation) {
      return rejected("RFX-MEDIA-INDEX-009",
                      "sample PTS range is not admitted by its Stream range");
    }
    auto& cursor = cursors[sample.stream_id.value];
    if (sample.sample_index != cursor.next_index ||
        (cursor.has_previous &&
         sample.decode_timestamp <= cursor.previous_dts)) {
      return rejected("RFX-MEDIA-INDEX-006",
                      "per-stream sample indices and DTS must increase exactly");
    }
    ++cursor.next_index;
    cursor.previous_dts = sample.decode_timestamp;
    cursor.has_previous = true;
    byte_ranges.emplace_back(
        sample.byte_offset,
        sample.byte_offset + static_cast<std::uint64_t>(sample.byte_size));
  }
  if (cursors.size() != index.streams.size()) {
    return rejected("RFX-MEDIA-INDEX-007",
                    "every selected MediaIndex stream requires samples");
  }
  std::sort(byte_ranges.begin(), byte_ranges.end());
  for (std::size_t index = 1; index < byte_ranges.size(); ++index) {
    if (byte_ranges[index].first < byte_ranges[index - 1].second) {
      return rejected("RFX-MEDIA-INDEX-008",
                      "compressed sample byte ranges must not overlap");
    }
  }
  return CompositionValidation{.valid = true};
}

std::string canonical_media_index_bytes(const MediaIndex& index) {
  const auto validation = validate_media_index(index);
  if (!validation.valid) {
    throw std::invalid_argument(validation.code + ": " + validation.message);
  }
  std::string output = "refusion-media-index-v1\nsource ";
  append_token(output, index.source_digest);
  output.push_back(' ');
  append_integer(output, index.source_byte_size);
  output.push_back(' ');
  append_token(output, container_name(index.container_profile));
  output.push_back('\n');

  for (const auto& stream : index.streams) {
    output += "stream ";
    append_token(output, stream.stream_id.value);
    output.push_back(' ');
    append_integer(output, stream.container_track_id);
    output.push_back(' ');
    append_token(output, stream_kind_name(stream.kind));
    output.push_back(' ');
    append_token(output, codec_name(stream.codec));
    output.push_back(' ');
    append_token(output, stream.codec_configuration_digest);
    output.push_back(' ');
    append_integer(output, stream.time_base.numerator);
    output.push_back('/');
    append_integer(output, stream.time_base.denominator);
    output.push_back(' ');
    append_integer(output, stream.start);
    output.push_back(' ');
    append_integer(output, stream.duration);
    if (const auto* video = std::get_if<VideoStreamFormat>(&stream.format)) {
      output += " video ";
      append_integer(output, video->coded_extent.width_pixels);
      output.push_back('x');
      append_integer(output, video->coded_extent.height_pixels);
      output.push_back(' ');
      append_integer(output, video->display_extent.width_pixels);
      output.push_back('x');
      append_integer(output, video->display_extent.height_pixels);
      output.push_back(' ');
      append_integer(output, video->presentation_rate.numerator);
      output.push_back('/');
      append_integer(output, video->presentation_rate.denominator);
      output.push_back(' ');
      append_integer(output, static_cast<unsigned>(video->bit_depth));
      output.push_back(' ');
      append_integer(output,
                     static_cast<unsigned>(video->chroma_subsampling_x));
      output.push_back('/');
      append_integer(output,
                     static_cast<unsigned>(video->chroma_subsampling_y));
      output.push_back(' ');
      append_token(output, video->color_primaries);
      output.push_back(' ');
      append_token(output, video->color_transfer);
      output.push_back(' ');
      append_token(output, video->color_matrix);
      output.push_back(' ');
      append_integer(output, video->orientation_degrees);
      output.push_back(' ');
      append_integer(output, video->sample_aspect_numerator);
      output.push_back('/');
      append_integer(output, video->sample_aspect_denominator);
    } else {
      const auto& audio = std::get<AudioStreamFormat>(stream.format);
      output += " audio ";
      append_integer(output, audio.sample_rate_hz);
      output.push_back(' ');
      append_integer(output, static_cast<unsigned>(audio.channels));
    }
    output.push_back('\n');
  }

  for (const auto& notice : index.notices) {
    output += "notice ";
    append_token(output, notice.stream_id.value);
    output.push_back(' ');
    append_token(output, notice_name(notice.kind));
    output.push_back('\n');
  }

  for (const auto& configuration : index.codec_configurations) {
    output += "configuration ";
    append_token(output, configuration.stream_id.value);
    output.push_back(' ');
    append_integer(output, configuration.sample_description_index);
    output.push_back(' ');
    append_token(output, configuration.content_digest);
    output.push_back(' ');
    append_integer(output, configuration.bytes.size());
    output.push_back(':');
    append_hex_bytes(output, configuration.bytes);
    output.push_back('\n');
  }

  for (const auto& sample : index.samples_decode_order) {
    output += "sample ";
    append_token(output, sample.stream_id.value);
    output.push_back(' ');
    append_integer(output, sample.sample_index);
    output.push_back(' ');
    append_integer(output, sample.byte_offset);
    output.push_back(' ');
    append_integer(output, sample.byte_size);
    output.push_back(' ');
    append_integer(output, sample.presentation_timestamp);
    output.push_back(' ');
    append_integer(output, sample.decode_timestamp);
    output.push_back(' ');
    append_integer(output, sample.duration);
    output.push_back(' ');
    append_integer(output, sample.time_base.numerator);
    output.push_back('/');
    append_integer(output, sample.time_base.denominator);
    output.push_back(' ');
    output.push_back(sample.sync_sample ? '1' : '0');
    output.push_back(' ');
    output.push_back(sample.discard_sample ? '1' : '0');
    output.push_back(' ');
    append_token(output, dependency_name(sample.dependencies.leading));
    output.push_back(' ');
    append_token(output,
                 dependency_name(sample.dependencies.depends_on_others));
    output.push_back(' ');
    append_token(output, dependency_name(sample.dependencies.is_depended_on));
    output.push_back(' ');
    append_token(output, dependency_name(sample.dependencies.has_redundancy));
    output.push_back(' ');
    append_integer(output, sample.sample_description_index);
    output.push_back('\n');
  }
  return output;
}

std::string media_index_digest(const MediaIndex& index) {
  const auto canonical = canonical_media_index_bytes(index);
  const auto* data = reinterpret_cast<const std::uint8_t*>(canonical.data());
  return sha256_content_digest(
      std::span<const std::uint8_t>{data, canonical.size()});
}

}  // namespace refusion::core
