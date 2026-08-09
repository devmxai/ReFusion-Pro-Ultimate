#include "refusion/adapters/media/FfmpegMediaDemuxer.hpp"

#include "refusion/core/ContentDigest.hpp"
#include "refusion/core/DesktopVideoImportProfile.hpp"

extern "C" {
#include <libavcodec/codec_par.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libavutil/display.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <libavutil/pixfmt.h>
}

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#if LIBAVFORMAT_VERSION_MAJOR != 62
#error "ReFusion requires the pinned FFmpeg n8.0.x libavformat ABI"
#endif

namespace refusion::adapters::media {
namespace {

using namespace application;
using namespace core;

constexpr std::size_t kAvioBufferBytes = 64U * 1024U;
constexpr std::uint64_t kMaximumSourceBytes =
    8ULL * 1024ULL * 1024ULL * 1024ULL;

[[nodiscard]] bool valid_sha256_digest(const std::string_view value) noexcept {
  return value.size() == 71 && value.starts_with("sha256:") &&
         std::all_of(value.begin() + 7, value.end(), [](const char character) {
           return (character >= '0' && character <= '9') ||
                  (character >= 'a' && character <= 'f');
         });
}

[[nodiscard]] int compare_unsigned_fractions(
    std::uint64_t left_numerator, std::uint64_t left_denominator,
    std::uint64_t right_numerator, std::uint64_t right_denominator) noexcept {
  bool reverse = false;
  for (;;) {
    const auto left_quotient = left_numerator / left_denominator;
    const auto right_quotient = right_numerator / right_denominator;
    if (left_quotient != right_quotient) {
      const int result = left_quotient < right_quotient ? -1 : 1;
      return reverse ? -result : result;
    }
    const auto left_remainder = left_numerator % left_denominator;
    const auto right_remainder = right_numerator % right_denominator;
    if (left_remainder == 0 || right_remainder == 0) {
      if (left_remainder == right_remainder) return 0;
      const int result = left_remainder == 0 ? -1 : 1;
      return reverse ? -result : result;
    }
    left_numerator = left_denominator;
    left_denominator = left_remainder;
    right_numerator = right_denominator;
    right_denominator = right_remainder;
    reverse = !reverse;
  }
}

[[nodiscard]] bool within_ten_minutes(const std::uint64_t ticks,
                                      const AVRational time_base) noexcept {
  return time_base.num > 0 && time_base.den > 0 &&
         compare_unsigned_fractions(
             ticks, static_cast<std::uint64_t>(time_base.den), 600,
             static_cast<std::uint64_t>(time_base.num)) <= 0;
}

struct SourceCursor final {
  ImmutableCompressedSourceLease* source{nullptr};
  std::uint64_t offset{0};
  bool failed{false};
};

int read_packet(void* opaque, std::uint8_t* buffer, const int buffer_size) {
  auto& cursor = *static_cast<SourceCursor*>(opaque);
  if (cursor.failed || cursor.source == nullptr || buffer_size <= 0) {
    return AVERROR(EIO);
  }
  if (cursor.offset >= cursor.source->byte_size()) return AVERROR_EOF;
  const auto available = cursor.source->byte_size() - cursor.offset;
  const auto requested = static_cast<std::size_t>(std::min<std::uint64_t>(
      available, static_cast<std::uint64_t>(buffer_size)));
  const auto result = cursor.source->read_at(
      cursor.offset, std::span<std::uint8_t>{buffer, requested});
  if (result.state == CompressedSourceReadState::end_of_source) {
    return AVERROR_EOF;
  }
  if (result.state != CompressedSourceReadState::read ||
      result.bytes_read == 0 || result.bytes_read > requested ||
      result.bytes_read > static_cast<std::size_t>(
                              std::numeric_limits<int>::max())) {
    cursor.failed = true;
    return AVERROR(EIO);
  }
  cursor.offset += result.bytes_read;
  return static_cast<int>(result.bytes_read);
}

std::int64_t seek_source(void* opaque, const std::int64_t offset,
                         const int whence) {
  auto& cursor = *static_cast<SourceCursor*>(opaque);
  if (cursor.failed || cursor.source == nullptr) return AVERROR(EIO);
  if ((whence & AVSEEK_SIZE) != 0) {
    if (cursor.source->byte_size() >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      return AVERROR(EOVERFLOW);
    }
    return static_cast<std::int64_t>(cursor.source->byte_size());
  }

  const int origin = whence & ~AVSEEK_FORCE;
  std::int64_t base = 0;
  if (origin == SEEK_CUR) {
    if (cursor.offset > static_cast<std::uint64_t>(
                            std::numeric_limits<std::int64_t>::max())) {
      return AVERROR(EOVERFLOW);
    }
    base = static_cast<std::int64_t>(cursor.offset);
  } else if (origin == SEEK_END) {
    if (cursor.source->byte_size() >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      return AVERROR(EOVERFLOW);
    }
    base = static_cast<std::int64_t>(cursor.source->byte_size());
  } else if (origin != SEEK_SET) {
    return AVERROR(EINVAL);
  }
  if ((offset > 0 && base > std::numeric_limits<std::int64_t>::max() - offset) ||
      (offset < 0 && base < std::numeric_limits<std::int64_t>::min() - offset)) {
    return AVERROR(EOVERFLOW);
  }
  const auto target = base + offset;
  if (target < 0 || static_cast<std::uint64_t>(target) >
                        cursor.source->byte_size()) {
    return AVERROR(EINVAL);
  }
  cursor.offset = static_cast<std::uint64_t>(target);
  return target;
}

struct FormatLease final {
  AVFormatContext* format{nullptr};
  AVIOContext* io{nullptr};

  ~FormatLease() {
    if (format != nullptr) avformat_close_input(&format);
    if (io != nullptr) avio_context_free(&io);
  }
};

struct PacketDeleter final {
  void operator()(AVPacket* packet) const noexcept {
    av_packet_free(&packet);
  }
};

[[nodiscard]] MediaDemuxResult failure(const MediaDemuxState state,
                                       std::string code,
                                       std::string diagnostic) {
  return MediaDemuxResult{
      .state = state,
      .index = std::nullopt,
      .code = std::move(code),
      .diagnostic = std::move(diagnostic),
  };
}

[[nodiscard]] std::string ffmpeg_error(const int code) {
  std::array<char, AV_ERROR_MAX_STRING_SIZE> message{};
  if (av_strerror(code, message.data(), message.size()) < 0) {
    return "unknown demux provider error";
  }
  return message.data();
}

[[nodiscard]] constexpr std::uint32_t fourcc(const char a, const char b,
                                             const char c,
                                             const char d) noexcept {
  return static_cast<std::uint32_t>(static_cast<unsigned char>(a)) |
         (static_cast<std::uint32_t>(static_cast<unsigned char>(b)) << 8U) |
         (static_cast<std::uint32_t>(static_cast<unsigned char>(c)) << 16U) |
         (static_cast<std::uint32_t>(static_cast<unsigned char>(d)) << 24U);
}

[[nodiscard]] bool encrypted_codec_tag(const std::uint32_t tag) noexcept {
  return tag == fourcc('e', 'n', 'c', 'v') ||
         tag == fourcc('e', 'n', 'c', 'a');
}

[[nodiscard]] bool encrypted_side_data(const AVPacketSideData* side_data,
                                       const int side_data_count) noexcept {
  return av_packet_side_data_get(side_data, side_data_count,
                                 AV_PKT_DATA_ENCRYPTION_INIT_INFO) != nullptr ||
         av_packet_side_data_get(side_data, side_data_count,
                                 AV_PKT_DATA_ENCRYPTION_INFO) != nullptr;
}

[[nodiscard]] std::int16_t orientation_degrees(
    const AVCodecParameters& parameters) {
  const auto* side_data = av_packet_side_data_get(
      parameters.coded_side_data, parameters.nb_coded_side_data,
      AV_PKT_DATA_DISPLAYMATRIX);
  if (side_data == nullptr ||
      side_data->size < 9U * sizeof(std::int32_t)) {
    return 0;
  }
  const auto* matrix = reinterpret_cast<const std::int32_t*>(side_data->data);
  const auto provider_rotation = av_display_rotation_get(matrix);
  if (!std::isfinite(provider_rotation)) return 0;
  auto normalized = static_cast<int>(std::lround(provider_rotation)) % 360;
  if (normalized < 0) normalized += 360;
  return static_cast<std::int16_t>(normalized);
}

struct AvcConfigurationSummary final {
  std::uint8_t profile_idc{0};
  std::uint8_t level_idc{0};
  std::uint8_t nal_length_bytes{0};
  std::uint8_t chroma_format_idc{1};
  std::uint8_t bit_depth_luma{8};
  std::uint8_t bit_depth_chroma{8};
  bool vui_video_signal_present{false};
  bool vui_full_range{false};
  std::uint8_t vui_color_primaries{2};
  std::uint8_t vui_color_transfer{2};
  std::uint8_t vui_color_matrix{2};
  std::uint32_t vui_sample_aspect_numerator{0};
  std::uint32_t vui_sample_aspect_denominator{0};
};

class RbspBitReader final {
 public:
  explicit RbspBitReader(const std::span<const std::uint8_t> ebsp) {
    bytes_.reserve(ebsp.size());
    std::uint8_t zero_count = 0;
    for (const auto byte : ebsp) {
      if (zero_count >= 2 && byte == 0x03U) {
        zero_count = 0;
        continue;
      }
      bytes_.push_back(byte);
      zero_count = byte == 0 ? static_cast<std::uint8_t>(zero_count + 1U) : 0;
    }
  }

  [[nodiscard]] std::optional<std::uint32_t> bits(
      const std::uint8_t count) noexcept {
    if (count > 32 || bit_offset_ + count > bytes_.size() * 8U) {
      return std::nullopt;
    }
    std::uint32_t value = 0;
    for (std::uint8_t index = 0; index < count; ++index) {
      value = static_cast<std::uint32_t>(
          (value << 1U) |
          ((bytes_[bit_offset_ / 8U] >> (7U - bit_offset_ % 8U)) & 1U));
      ++bit_offset_;
    }
    return value;
  }

  [[nodiscard]] std::optional<std::uint32_t> ue() noexcept {
    std::uint8_t leading_zeroes = 0;
    for (;;) {
      const auto bit = bits(1);
      if (!bit) return std::nullopt;
      if (*bit != 0) break;
      if (++leading_zeroes > 31) return std::nullopt;
    }
    const auto suffix = bits(leading_zeroes);
    if (!suffix) return std::nullopt;
    return ((1U << leading_zeroes) - 1U) + *suffix;
  }

  [[nodiscard]] std::optional<std::int32_t> se() noexcept {
    const auto encoded = ue();
    if (!encoded) return std::nullopt;
    const auto magnitude = static_cast<std::int32_t>((*encoded + 1U) / 2U);
    return (*encoded & 1U) != 0U ? magnitude : -magnitude;
  }

 private:
  std::vector<std::uint8_t> bytes_;
  std::size_t bit_offset_{0};
};

[[nodiscard]] bool skip_scaling_list(RbspBitReader& reader,
                                     const std::uint8_t size) noexcept {
  std::int32_t last_scale = 8;
  std::int32_t next_scale = 8;
  for (std::uint8_t index = 0; index < size; ++index) {
    if (next_scale != 0) {
      const auto delta = reader.se();
      if (!delta) return false;
      next_scale = (last_scale + *delta + 256) % 256;
    }
    last_scale = next_scale == 0 ? last_scale : next_scale;
  }
  return true;
}

void read_sps_video_signal(const std::span<const std::uint8_t> nal,
                           AvcConfigurationSummary& result) noexcept {
  if (nal.size() < 4 || (nal.front() & 0x1fU) != 7U) return;
  RbspBitReader reader{nal.subspan(1)};
  const auto profile = reader.bits(8);
  const auto constraints = reader.bits(8);
  const auto level = reader.bits(8);
  const auto sequence_id = reader.ue();
  if (!profile || !constraints || !level || !sequence_id) return;

  std::uint32_t chroma_format = 1;
  if (*profile == 100 || *profile == 110 || *profile == 122 ||
      *profile == 244 || *profile == 44 || *profile == 83 ||
      *profile == 86 || *profile == 118 || *profile == 128 ||
      *profile == 138 || *profile == 139 || *profile == 134 ||
      *profile == 135) {
    const auto parsed_chroma = reader.ue();
    if (!parsed_chroma || *parsed_chroma > 3) return;
    chroma_format = *parsed_chroma;
    if (chroma_format == 3 && !reader.bits(1)) return;
    if (!reader.ue() || !reader.ue() || !reader.bits(1)) return;
    const auto scaling_matrix = reader.bits(1);
    if (!scaling_matrix) return;
    if (*scaling_matrix != 0) {
      const auto list_count = chroma_format == 3 ? 12U : 8U;
      for (std::uint8_t index = 0; index < list_count; ++index) {
        const auto present = reader.bits(1);
        if (!present) return;
        if (*present != 0 &&
            !skip_scaling_list(reader, index < 6 ? 16U : 64U)) {
          return;
        }
      }
    }
  }

  if (!reader.ue()) return;  // log2_max_frame_num_minus4
  const auto picture_order = reader.ue();
  if (!picture_order) return;
  if (*picture_order == 0) {
    if (!reader.ue()) return;
  } else if (*picture_order == 1) {
    if (!reader.bits(1) || !reader.se() || !reader.se()) return;
    const auto cycle = reader.ue();
    if (!cycle || *cycle > 255) return;
    for (std::uint32_t index = 0; index < *cycle; ++index) {
      if (!reader.se()) return;
    }
  } else if (*picture_order > 2) {
    return;
  }
  if (!reader.ue() || !reader.bits(1) || !reader.ue() || !reader.ue()) return;
  const auto frame_only = reader.bits(1);
  if (!frame_only) return;
  if (*frame_only == 0 && !reader.bits(1)) return;
  if (!reader.bits(1)) return;
  const auto cropped = reader.bits(1);
  if (!cropped) return;
  if (*cropped != 0 &&
      (!reader.ue() || !reader.ue() || !reader.ue() || !reader.ue())) {
    return;
  }
  const auto vui_present = reader.bits(1);
  if (!vui_present || *vui_present == 0) return;

  const auto aspect_present = reader.bits(1);
  if (!aspect_present) return;
  if (*aspect_present != 0) {
    const auto aspect_idc = reader.bits(8);
    if (!aspect_idc) return;
    if (*aspect_idc == 1) {
      result.vui_sample_aspect_numerator = 1;
      result.vui_sample_aspect_denominator = 1;
    } else if (*aspect_idc == 255) {
      const auto width = reader.bits(16);
      const auto height = reader.bits(16);
      if (!width || !height || *width == 0 || *height == 0) return;
      result.vui_sample_aspect_numerator = *width;
      result.vui_sample_aspect_denominator = *height;
    }
  }
  const auto overscan_present = reader.bits(1);
  if (!overscan_present) return;
  if (*overscan_present != 0 && !reader.bits(1)) return;
  const auto video_signal_present = reader.bits(1);
  if (!video_signal_present || *video_signal_present == 0) return;
  if (!reader.bits(3)) return;
  const auto full_range = reader.bits(1);
  const auto color_description = reader.bits(1);
  if (!full_range || !color_description) return;
  result.vui_video_signal_present = true;
  result.vui_full_range = *full_range != 0;
  if (*color_description != 0) {
    const auto primaries = reader.bits(8);
    const auto transfer = reader.bits(8);
    const auto matrix = reader.bits(8);
    if (!primaries || !transfer || !matrix) return;
    result.vui_color_primaries = static_cast<std::uint8_t>(*primaries);
    result.vui_color_transfer = static_cast<std::uint8_t>(*transfer);
    result.vui_color_matrix = static_cast<std::uint8_t>(*matrix);
  }
}

[[nodiscard]] std::optional<AvcConfigurationSummary>
parse_avc_decoder_configuration(const std::uint8_t* bytes, const int size) {
  if (bytes == nullptr || size < 7 || bytes[0] != 1) return std::nullopt;
  const auto available = static_cast<std::size_t>(size);
  AvcConfigurationSummary result{
      .profile_idc = bytes[1],
      .level_idc = bytes[3],
      .nal_length_bytes = static_cast<std::uint8_t>((bytes[4] & 0x03U) + 1U),
  };
  if ((bytes[4] & 0xfcU) != 0xfcU || (bytes[5] & 0xe0U) != 0xe0U ||
      result.nal_length_bytes != 4) {
    return std::nullopt;
  }

  std::size_t cursor = 6;
  const auto sequence_count = static_cast<std::uint8_t>(bytes[5] & 0x1fU);
  if (sequence_count == 0) return std::nullopt;
  if (cursor + 2 > available) return std::nullopt;
  const auto first_sequence_length = static_cast<std::size_t>(
      (static_cast<std::uint16_t>(bytes[cursor]) << 8U) |
      static_cast<std::uint16_t>(bytes[cursor + 1]));
  if (first_sequence_length == 0 ||
      first_sequence_length > available - cursor - 2U) {
    return std::nullopt;
  }
  const std::span<const std::uint8_t> first_sequence{
      bytes + cursor + 2U, first_sequence_length};
  const auto skip_arrays = [&](const std::uint8_t count,
                               std::size_t& offset) noexcept {
    for (std::uint8_t index = 0; index < count; ++index) {
      if (offset + 2 > available) return false;
      const auto length = static_cast<std::size_t>(
          (static_cast<std::uint16_t>(bytes[offset]) << 8U) |
          static_cast<std::uint16_t>(bytes[offset + 1]));
      offset += 2;
      if (length == 0 || length > available - offset) return false;
      offset += length;
    }
    return true;
  };
  if (!skip_arrays(sequence_count, cursor) || cursor >= available) {
    return std::nullopt;
  }
  const auto picture_count = bytes[cursor++];
  if (picture_count == 0 || !skip_arrays(picture_count, cursor)) {
    return std::nullopt;
  }

  const bool extended_profile =
      result.profile_idc == 100 || result.profile_idc == 110 ||
      result.profile_idc == 122 || result.profile_idc == 144;
  if (extended_profile) {
    if (cursor + 4 > available || (bytes[cursor] & 0xfcU) != 0xfcU ||
        (bytes[cursor + 1] & 0xf8U) != 0xf8U ||
        (bytes[cursor + 2] & 0xf8U) != 0xf8U) {
      return std::nullopt;
    }
    result.chroma_format_idc = bytes[cursor] & 0x03U;
    result.bit_depth_luma =
        static_cast<std::uint8_t>(8U + (bytes[cursor + 1] & 0x07U));
    result.bit_depth_chroma =
        static_cast<std::uint8_t>(8U + (bytes[cursor + 2] & 0x07U));
  }
  read_sps_video_signal(first_sequence, result);
  return result;
}

[[nodiscard]] bool supported_h264_profile(
    const std::uint8_t profile_idc) noexcept {
  return refusion::core::desktop_video_import::admitted_h264_profile_idc(
      profile_idc);
}

struct AacConfigurationSummary final {
  std::uint8_t audio_object_type{0};
  std::uint32_t sample_rate_hz{0};
  std::uint8_t channel_configuration{0};
};

[[nodiscard]] std::optional<AacConfigurationSummary>
parse_aac_audio_specific_configuration(const std::uint8_t* bytes,
                                       const int size) {
  constexpr std::array<std::uint32_t, 13> kSampleRates{
      96'000, 88'200, 64'000, 48'000, 44'100, 32'000, 24'000,
      22'050, 16'000, 12'000, 11'025, 8'000, 7'350};
  if (bytes == nullptr || size < 2) return std::nullopt;
  const auto object_type = static_cast<std::uint8_t>(bytes[0] >> 3U);
  const auto frequency_index = static_cast<std::uint8_t>(
      ((bytes[0] & 0x07U) << 1U) | (bytes[1] >> 7U));
  const auto channel_configuration =
      static_cast<std::uint8_t>((bytes[1] >> 3U) & 0x0fU);
  if (object_type == 0 || object_type == 31 ||
      frequency_index >= kSampleRates.size() || channel_configuration == 0) {
    return std::nullopt;
  }
  return AacConfigurationSummary{
      .audio_object_type = object_type,
      .sample_rate_hz = kSampleRates[frequency_index],
      .channel_configuration = channel_configuration,
  };
}

[[nodiscard]] bool positive_rational(const AVRational value) noexcept {
  return value.num > 0 && value.den > 0;
}

[[nodiscard]] std::string bytes_digest(const std::uint8_t* bytes,
                                       const int size) {
  if (bytes == nullptr || size <= 0) return {};
  return sha256_content_digest(std::span<const std::uint8_t>{
      bytes, static_cast<std::size_t>(size)});
}

struct SelectedStream final {
  MediaStreamId stream_id;
  std::uint64_t next_sample_index{0};
};

}  // namespace

MediaDemuxResult FfmpegMediaDemuxer::build_index(
    ImmutableCompressedSourceLease& source,
    const MediaCancellationToken* cancellation) {
  const auto was_cancelled = [cancellation]() noexcept {
    return cancellation != nullptr && cancellation->cancelled();
  };
  if (was_cancelled()) {
    return failure(MediaDemuxState::cancelled, "RFX-MEDIA-IMPORT-CANCELLED",
                   "media indexing was cancelled before provider admission");
  }
  if (source.byte_size() == 0 || source.byte_size() > kMaximumSourceBytes ||
      !valid_sha256_digest(source.content_digest())) {
    return failure(MediaDemuxState::invalid_source,
                   "RFX-MEDIA-IMPORT-SOURCE-INVALID",
                   "compressed source identity is incomplete");
  }

  SourceCursor cursor{.source = &source};
  FormatLease lease;
  auto* avio_buffer =
      static_cast<std::uint8_t*>(av_malloc(kAvioBufferBytes));
  if (avio_buffer == nullptr) {
    return failure(MediaDemuxState::provider_failure,
                   "RFX-MEDIA-IMPORT-PROVIDER-FAILED",
                   "cannot allocate bounded demux I/O buffer");
  }
  lease.io = avio_alloc_context(
      avio_buffer, static_cast<int>(kAvioBufferBytes), 0, &cursor, read_packet,
      nullptr, seek_source);
  if (lease.io == nullptr) {
    av_free(avio_buffer);
    return failure(MediaDemuxState::provider_failure,
                   "RFX-MEDIA-IMPORT-PROVIDER-FAILED",
                   "cannot create custom demux I/O context");
  }

  lease.format = avformat_alloc_context();
  if (lease.format == nullptr) {
    return failure(MediaDemuxState::provider_failure,
                   "RFX-MEDIA-IMPORT-PROVIDER-FAILED",
                   "cannot allocate demux context");
  }
  lease.format->pb = lease.io;
  lease.format->flags |= AVFMT_FLAG_CUSTOM_IO;
  const AVInputFormat* input_format = nullptr;
  const auto probe_result = av_probe_input_buffer2(
      lease.io, &input_format, "immutable-compressed-source", nullptr, 0, 0);
  if (probe_result < 0 || input_format == nullptr ||
      std::strcmp(input_format->name, "mov,mp4,m4a,3gp,3g2,mj2") != 0) {
    return failure(MediaDemuxState::unsupported_container,
                   "RFX-MEDIA-IMPORT-CONTAINER-UNSUPPORTED",
                   "compressed source is not an admitted ISO BMFF MP4/MOV container");
  }
  auto* format = lease.format;
  // This is a provider-local virtual label, not a host path. A non-null label
  // lets libavformat classify the bounded custom AVIO as local/seekable without
  // opening a protocol or receiving filesystem authority.
  const auto open_result = avformat_open_input(
      &format, "immutable-compressed-source", input_format, nullptr);
  lease.format = format;
  if (open_result < 0) {
    return failure(MediaDemuxState::corrupt, "RFX-MEDIA-IMPORT-CORRUPT",
                   "ISO BMFF header rejected: " + ffmpeg_error(open_result));
  }
  const auto info_result = avformat_find_stream_info(lease.format, nullptr);
  if (was_cancelled()) {
    return failure(MediaDemuxState::cancelled, "RFX-MEDIA-IMPORT-CANCELLED",
                   "media indexing was cancelled during Stream discovery");
  }
  if (info_result < 0 || cursor.failed) {
    return failure(MediaDemuxState::corrupt, "RFX-MEDIA-IMPORT-CORRUPT",
                   "ISO BMFF stream tables are incomplete: " +
                       ffmpeg_error(info_result));
  }

  MediaIndex index{
      .contract_version = 1,
      .source_digest = source.content_digest(),
      .source_byte_size = source.byte_size(),
      .container_profile = MediaContainerProfile::iso_bmff_mp4,
  };
  if (const auto* brand = av_dict_get(lease.format->metadata, "major_brand",
                                      nullptr, 0);
      brand != nullptr && std::string_view{brand->value} == "qt  ") {
    index.container_profile = MediaContainerProfile::quicktime_mov;
  }

  std::unordered_map<int, SelectedStream> selected;
  std::size_t video_count = 0;
  std::size_t audio_count = 0;
  for (unsigned int stream_index = 0;
       stream_index < lease.format->nb_streams; ++stream_index) {
    if (was_cancelled()) {
      return failure(MediaDemuxState::cancelled, "RFX-MEDIA-IMPORT-CANCELLED",
                     "media indexing was cancelled during Stream validation");
    }
    const auto& stream = *lease.format->streams[stream_index];
    const auto& parameters = *stream.codecpar;
    if (encrypted_codec_tag(parameters.codec_tag) ||
        encrypted_side_data(parameters.coded_side_data,
                            parameters.nb_coded_side_data)) {
      return failure(MediaDemuxState::encrypted,
                     "RFX-MEDIA-IMPORT-ENCRYPTED",
                     "encrypted ISO BMFF tracks are not admitted");
    }
    // QuickTime timecode is ancillary metadata, not an alternate media stream.
    // It carries no decoded pixels/audio and is intentionally outside the
    // selected portable media truth for this slice.
    if (parameters.codec_type == AVMEDIA_TYPE_DATA &&
        parameters.codec_tag == fourcc('t', 'm', 'c', 'd')) {
      continue;
    }
    if (parameters.codec_type != AVMEDIA_TYPE_VIDEO &&
        parameters.codec_type != AVMEDIA_TYPE_AUDIO) {
      return failure(MediaDemuxState::unsupported_profile,
                     "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                     "first profile admits only one Video and optional Audio track");
    }
    if (stream.id <= 0 || stream.start_time == AV_NOPTS_VALUE ||
        stream.duration <= 0 || !positive_rational(stream.time_base) ||
        !within_ten_minutes(static_cast<std::uint64_t>(stream.duration),
                            stream.time_base) ||
        parameters.extradata_size <= 0) {
      return failure(MediaDemuxState::corrupt, "RFX-MEDIA-IMPORT-CORRUPT",
                     "required Stream identity, timing or codec configuration is absent");
    }

    MediaStreamDescriptor descriptor{
        .stream_id = MediaStreamId{"stream_track_" +
                                   std::to_string(stream.id)},
        .container_track_id = static_cast<std::uint32_t>(stream.id),
        .codec_configuration_digest =
            bytes_digest(parameters.extradata, parameters.extradata_size),
        .time_base = MediaTimeBase{.numerator = stream.time_base.num,
                                   .denominator = stream.time_base.den},
        .start = stream.start_time,
        .duration = static_cast<std::uint64_t>(stream.duration),
    };

    if (parameters.codec_type == AVMEDIA_TYPE_VIDEO) {
      ++video_count;
      if (video_count > 1 || parameters.codec_id != AV_CODEC_ID_H264) {
        return failure(MediaDemuxState::unsupported_profile,
                       "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                       "exactly one H.264 Video track is admitted");
      }
      const auto avc_configuration = parse_avc_decoder_configuration(
          parameters.extradata, parameters.extradata_size);
      if (!avc_configuration ||
          !supported_h264_profile(avc_configuration->profile_idc) ||
          avc_configuration->level_idc == 0 ||
          avc_configuration->level_idc >
              refusion::core::desktop_video_import::maximum_h264_level_idc) {
        return failure(MediaDemuxState::unsupported_profile,
                       "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                       "H.264 profile or level is outside Desktop v1");
      }
      if (parameters.width <= 0 || parameters.height <= 0 ||
          static_cast<std::uint32_t>(parameters.width) >
              refusion::core::desktop_video_import::maximum_coded_dimension ||
          static_cast<std::uint32_t>(parameters.height) >
              refusion::core::desktop_video_import::maximum_coded_dimension ||
          static_cast<std::uint64_t>(parameters.width) *
                  static_cast<std::uint64_t>(parameters.height) >
              refusion::core::desktop_video_import::maximum_coded_pixels ||
          (parameters.bit_rate > 0 &&
           static_cast<std::uint64_t>(parameters.bit_rate) >
               refusion::core::desktop_video_import::
                   maximum_bitrate_bits_per_second)) {
        return failure(MediaDemuxState::unsupported_profile,
                       "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                       "Video coded extent or bitrate exceeds Desktop v1");
      }
      if (avc_configuration->chroma_format_idc != 1 ||
          avc_configuration->bit_depth_luma != 8 ||
          avc_configuration->bit_depth_chroma != 8) {
        return failure(MediaDemuxState::unsupported_profile,
                       "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                       "Video pixel format is not declared 8-bit 4:2:0");
      }
      auto effective_range = parameters.color_range;
      auto effective_primaries = parameters.color_primaries;
      auto effective_transfer = parameters.color_trc;
      auto effective_matrix = parameters.color_space;
      if (avc_configuration->vui_video_signal_present) {
        if (effective_range == AVCOL_RANGE_UNSPECIFIED) {
          effective_range = avc_configuration->vui_full_range
                                ? AVCOL_RANGE_JPEG
                                : AVCOL_RANGE_MPEG;
        }
        if (effective_primaries == AVCOL_PRI_UNSPECIFIED) {
          effective_primaries = static_cast<AVColorPrimaries>(
              avc_configuration->vui_color_primaries);
        }
        if (effective_transfer == AVCOL_TRC_UNSPECIFIED) {
          effective_transfer = static_cast<AVColorTransferCharacteristic>(
              avc_configuration->vui_color_transfer);
        }
        if (effective_matrix == AVCOL_SPC_UNSPECIFIED) {
          effective_matrix = static_cast<AVColorSpace>(
              avc_configuration->vui_color_matrix);
        }
      }
      // Container codec parameters are preferred. Missing values are recovered
      // only from the same AVC configuration's SPS VUI; no resolution or host
      // decoder inference is permitted.
      const bool declared_video_range =
          effective_range == AVCOL_RANGE_MPEG;
      const bool explicit_bt709_transfer =
          effective_transfer == AVCOL_TRC_BT709;
      const bool defaultable_bt709_transfer =
          effective_transfer == AVCOL_TRC_UNSPECIFIED;
      if (!declared_video_range ||
          effective_primaries != AVCOL_PRI_BT709 ||
          (!explicit_bt709_transfer && !defaultable_bt709_transfer) ||
          effective_matrix != AVCOL_SPC_BT709) {
        return failure(MediaDemuxState::unsupported_profile,
                       "RFX-MEDIA-IMPORT-COLOR-MISSING",
                       "video-range BT.709 primaries/matrix and BT.709 or unspecified SDR transfer are required (range=" +
                           std::to_string(effective_range) +
                           ", primaries=" +
                           std::to_string(effective_primaries) +
                           ", transfer=" +
                           std::to_string(effective_transfer) +
                           ", matrix=" +
                           std::to_string(effective_matrix) + ")");
      }
      if (defaultable_bt709_transfer) {
        index.notices.push_back(MediaIndexNotice{
            .stream_id = descriptor.stream_id,
            .kind = MediaIndexNoticeKind::bt709_transfer_defaulted,
        });
      }
      if (
          (parameters.field_order != AV_FIELD_UNKNOWN &&
           parameters.field_order != AV_FIELD_PROGRESSIVE)) {
        return failure(MediaDemuxState::unsupported_profile,
                       "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                       "interlaced Video is outside Desktop v1");
      }
      const auto rate = positive_rational(stream.avg_frame_rate)
                            ? stream.avg_frame_rate
                            : parameters.framerate;
      if (!positive_rational(rate) ||
          static_cast<std::uint64_t>(rate.num) >
              static_cast<std::uint64_t>(
                  refusion::core::desktop_video_import::
                      maximum_presentation_frames_per_second) *
                  static_cast<std::uint64_t>(rate.den)) {
        return failure(MediaDemuxState::unsupported_profile,
                       "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                       "Video presentation rate exceeds Desktop v1");
      }
      auto aspect = parameters.sample_aspect_ratio;
      if (!positive_rational(aspect)) aspect = stream.sample_aspect_ratio;
      if (!positive_rational(aspect) &&
          avc_configuration->vui_sample_aspect_numerator != 0 &&
          avc_configuration->vui_sample_aspect_denominator != 0) {
        aspect = AVRational{
            .num = static_cast<int>(
                avc_configuration->vui_sample_aspect_numerator),
            .den = static_cast<int>(
                avc_configuration->vui_sample_aspect_denominator),
        };
      }
      if (!positive_rational(aspect) || aspect.num != aspect.den) {
        return failure(MediaDemuxState::unsupported_profile,
                       "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                       "non-square or missing sample aspect is not admitted");
      }
      const auto orientation = orientation_degrees(parameters);
      if (orientation != 0 && orientation != 90 && orientation != 180 &&
          orientation != 270) {
        return failure(MediaDemuxState::unsupported_profile,
                       "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                       "Video orientation is outside the admitted quarter turns");
      }
      const bool swaps_extent = orientation == 90 || orientation == 270;
      descriptor.kind = MediaStreamKind::video;
      descriptor.codec = MediaCodec::h264_avc;
      descriptor.format = VideoStreamFormat{
          .coded_extent = CanvasExtent{
              .width_pixels = static_cast<std::uint32_t>(parameters.width),
              .height_pixels = static_cast<std::uint32_t>(parameters.height),
          },
          .display_extent = CanvasExtent{
              .width_pixels = static_cast<std::uint32_t>(
                  swaps_extent ? parameters.height : parameters.width),
              .height_pixels = static_cast<std::uint32_t>(
                  swaps_extent ? parameters.width : parameters.height),
          },
          .presentation_rate = RationalRate{
              .numerator = static_cast<std::uint32_t>(rate.num),
              .denominator = static_cast<std::uint32_t>(rate.den),
          },
          .bit_depth = 8,
          .chroma_subsampling_x = 2,
          .chroma_subsampling_y = 2,
          .color_range = MediaColorRange::video,
          .color_primaries = "bt709",
          .color_transfer = "bt709",
          .color_matrix = "bt709",
          .orientation_degrees = orientation,
          .sample_aspect_numerator = static_cast<std::uint32_t>(aspect.num),
          .sample_aspect_denominator = static_cast<std::uint32_t>(aspect.den),
      };
    } else {
      ++audio_count;
      const auto aac_configuration =
          parse_aac_audio_specific_configuration(parameters.extradata,
                                                 parameters.extradata_size);
      if (audio_count > 1 || parameters.codec_id != AV_CODEC_ID_AAC ||
          !aac_configuration ||
          aac_configuration->audio_object_type != 2 ||
          (parameters.sample_rate != 44'100 &&
           parameters.sample_rate != 48'000) ||
          (parameters.ch_layout.nb_channels != 1 &&
           parameters.ch_layout.nb_channels != 2) ||
          aac_configuration->sample_rate_hz !=
              static_cast<std::uint32_t>(parameters.sample_rate) ||
          aac_configuration->channel_configuration !=
              static_cast<std::uint8_t>(parameters.ch_layout.nb_channels)) {
        return failure(MediaDemuxState::unsupported_profile,
                       "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                       "Audio track is outside AAC-LC mono/stereo Desktop v1");
      }
      descriptor.kind = MediaStreamKind::audio;
      descriptor.codec = MediaCodec::aac_lc;
      descriptor.format = AudioStreamFormat{
          .sample_rate_hz =
              static_cast<std::uint32_t>(parameters.sample_rate),
          .channels =
              static_cast<std::uint8_t>(parameters.ch_layout.nb_channels),
      };
    }
    const auto descriptor_validation =
        validate_media_stream_descriptor(descriptor);
    if (!descriptor_validation.valid) {
      return failure(MediaDemuxState::unsupported_profile,
                     "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                     descriptor_validation.message);
    }
    selected.emplace(static_cast<int>(stream_index),
                     SelectedStream{.stream_id = descriptor.stream_id});
    index.codec_configurations.push_back(MediaCodecConfiguration{
        .stream_id = descriptor.stream_id,
        .sample_description_index = 1,
        .content_digest = descriptor.codec_configuration_digest,
        .bytes = std::vector<std::uint8_t>{
            parameters.extradata,
            parameters.extradata + parameters.extradata_size},
    });
    index.streams.push_back(std::move(descriptor));
  }
  if (video_count != 1) {
    return failure(MediaDemuxState::unsupported_profile,
                   "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                   "exactly one admitted Video track is required");
  }

  std::unique_ptr<AVPacket, PacketDeleter> packet{av_packet_alloc()};
  if (!packet) {
    return failure(MediaDemuxState::provider_failure,
                   "RFX-MEDIA-IMPORT-PROVIDER-FAILED",
                   "cannot allocate compressed packet descriptor");
  }
  int read_result = 0;
  while ((read_result = av_read_frame(lease.format, packet.get())) >= 0) {
    if (was_cancelled()) {
      return failure(MediaDemuxState::cancelled, "RFX-MEDIA-IMPORT-CANCELLED",
                     "media indexing was cancelled during sample traversal");
    }
    const auto selected_stream = selected.find(packet->stream_index);
    if (selected_stream == selected.end()) {
      av_packet_unref(packet.get());
      continue;
    }
    if ((packet->flags & AV_PKT_FLAG_CORRUPT) != 0 ||
        packet->pts == AV_NOPTS_VALUE || packet->dts == AV_NOPTS_VALUE ||
        packet->duration <= 0 || packet->pos < 0 || packet->size <= 0 ||
        packet->size > std::numeric_limits<std::uint32_t>::max()) {
      return failure(MediaDemuxState::corrupt, "RFX-MEDIA-IMPORT-CORRUPT",
                     "compressed sample lacks exact PTS, DTS, duration or byte range");
    }
    if (av_packet_get_side_data(packet.get(), AV_PKT_DATA_ENCRYPTION_INFO,
                                nullptr) != nullptr ||
        av_packet_get_side_data(packet.get(), AV_PKT_DATA_NEW_EXTRADATA,
                                nullptr) != nullptr) {
      const bool encrypted = av_packet_get_side_data(
                                 packet.get(), AV_PKT_DATA_ENCRYPTION_INFO,
                                 nullptr) != nullptr;
      return failure(encrypted ? MediaDemuxState::encrypted
                               : MediaDemuxState::unsupported_profile,
                     encrypted ? "RFX-MEDIA-IMPORT-ENCRYPTED"
                               : "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED",
                     encrypted ? "encrypted compressed sample is not admitted"
                               : "dynamic sample descriptions are not admitted");
    }
    const auto& stream =
        *lease.format->streams[static_cast<unsigned>(packet->stream_index)];
    const bool video = stream.codecpar->codec_type == AVMEDIA_TYPE_VIDEO;
    auto& selected_record = selected_stream->second;
    index.samples_decode_order.push_back(CompressedSample{
        .stream_id = selected_record.stream_id,
        .sample_index = selected_record.next_sample_index++,
        .byte_offset = static_cast<std::uint64_t>(packet->pos),
        .byte_size = static_cast<std::uint32_t>(packet->size),
        .presentation_timestamp = packet->pts,
        .decode_timestamp = packet->dts,
        .duration = static_cast<std::uint64_t>(packet->duration),
        .time_base = MediaTimeBase{.numerator = stream.time_base.num,
                                   .denominator = stream.time_base.den},
        .sync_sample = (packet->flags & AV_PKT_FLAG_KEY) != 0,
        .discard_sample = (packet->flags & AV_PKT_FLAG_DISCARD) != 0,
        .dependencies = SampleDependencyFlags{
            .leading = SampleDependencyValue::unknown,
            .depends_on_others =
                video ? ((packet->flags & AV_PKT_FLAG_KEY) != 0
                             ? SampleDependencyValue::no
                             : SampleDependencyValue::yes)
                      : SampleDependencyValue::unknown,
            .is_depended_on =
                (packet->flags & AV_PKT_FLAG_DISPOSABLE) != 0
                    ? SampleDependencyValue::no
                    : SampleDependencyValue::unknown,
            .has_redundancy = SampleDependencyValue::unknown,
        },
        .sample_description_index = 1,
    });
    av_packet_unref(packet.get());
  }
  if (read_result != AVERROR_EOF || cursor.failed) {
    return failure(MediaDemuxState::corrupt, "RFX-MEDIA-IMPORT-CORRUPT",
                   "compressed sample tables ended unexpectedly: " +
                       ffmpeg_error(read_result));
  }

  const auto validation = validate_media_index(index);
  if (!validation.valid) {
    return failure(MediaDemuxState::corrupt, "RFX-MEDIA-IMPORT-CORRUPT",
                   validation.code + ": " + validation.message);
  }
  return MediaDemuxResult{
      .state = MediaDemuxState::indexed,
      .index = std::move(index),
      .code = "RFX-MEDIA-INDEX-READY",
      .diagnostic = "portable MediaIndex built without decoding media",
  };
}

}  // namespace refusion::adapters::media
