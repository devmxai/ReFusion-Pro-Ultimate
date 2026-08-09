#include "refusion/adapters/media/FfmpegMediaDemuxer.hpp"

#include "refusion/core/ContentDigest.hpp"
#include "refusion/runtime/media/MediaIndexDecodeProjection.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace refusion::adapters::media;
using namespace refusion::application;
using namespace refusion::core;
using namespace refusion::runtime::media;

void require(const bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

class BufferCompressedSource final : public ImmutableCompressedSourceLease {
 public:
  explicit BufferCompressedSource(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    require(input.good(), "cannot open compressed fixture " + path.string());
    const auto end = input.tellg();
    require(end > 0, "compressed fixture is empty " + path.string());
    bytes_.resize(static_cast<std::size_t>(end));
    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char*>(bytes_.data()),
               static_cast<std::streamsize>(bytes_.size()));
    require(input.good(), "cannot read complete compressed fixture " +
                              path.string());
    digest_ = sha256_content_digest(bytes_);
  }

  [[nodiscard]] std::string content_digest() const override { return digest_; }

  [[nodiscard]] std::uint64_t byte_size() const noexcept override {
    return bytes_.size();
  }

  [[nodiscard]] CompressedSourceReadResult read_at(
      const std::uint64_t offset,
      const std::span<std::uint8_t> destination) noexcept override {
    if (offset > bytes_.size()) {
      return CompressedSourceReadResult{
          .state = CompressedSourceReadState::failed};
    }
    if (offset == bytes_.size()) {
      return CompressedSourceReadResult{
          .state = CompressedSourceReadState::end_of_source};
    }
    const auto available = bytes_.size() - static_cast<std::size_t>(offset);
    const auto count = std::min(available, destination.size());
    std::copy_n(bytes_.data() + static_cast<std::size_t>(offset), count,
                destination.data());
    return CompressedSourceReadResult{
        .state = CompressedSourceReadState::read, .bytes_read = count};
  }

 private:
  std::vector<std::uint8_t> bytes_;
  std::string digest_;
};

class AlwaysCancelled final : public MediaCancellationToken {
 public:
  [[nodiscard]] bool cancelled() const noexcept override { return true; }
};

class MemoryCompressedSource final : public ImmutableCompressedSourceLease {
 public:
  explicit MemoryCompressedSource(std::vector<std::uint8_t> bytes)
      : bytes_(std::move(bytes)), digest_(sha256_content_digest(bytes_)) {}

  [[nodiscard]] std::string content_digest() const override { return digest_; }
  [[nodiscard]] std::uint64_t byte_size() const noexcept override {
    return bytes_.size();
  }
  [[nodiscard]] CompressedSourceReadResult read_at(
      const std::uint64_t offset,
      const std::span<std::uint8_t> destination) noexcept override {
    if (offset > bytes_.size()) {
      return {.state = CompressedSourceReadState::failed};
    }
    if (offset == bytes_.size()) {
      return {.state = CompressedSourceReadState::end_of_source};
    }
    const auto count = std::min(
        destination.size(), bytes_.size() - static_cast<std::size_t>(offset));
    std::copy_n(bytes_.data() + static_cast<std::size_t>(offset), count,
                destination.data());
    return {.state = CompressedSourceReadState::read, .bytes_read = count};
  }

 private:
  std::vector<std::uint8_t> bytes_;
  std::string digest_;
};

struct AdmittedFixture final {
  const char* id;
  const char* file_name;
  const char* source_digest;
  std::uint64_t source_bytes;
  const char* expected_index_digest;
  MediaContainerProfile container;
  std::size_t sample_count;
  std::int64_t video_start;
  std::uint64_t video_duration;
  std::int32_t video_time_base_denominator;
  std::uint32_t coded_width;
  std::uint32_t coded_height;
  std::uint32_t display_width;
  std::uint32_t display_height;
  std::int16_t orientation;
  bool has_audio{true};
  std::uint32_t audio_rate;
  bool expected_defaulted_transfer{false};
  std::int64_t first_pts;
  std::int64_t first_dts;
  std::uint64_t first_duration;
  std::uint64_t first_byte_offset;
  std::uint32_t first_byte_size;
};

[[nodiscard]] const MediaStreamDescriptor& stream_of_kind(
    const MediaIndex& index, const MediaStreamKind kind) {
  const auto found = std::find_if(
      index.streams.begin(), index.streams.end(),
      [kind](const MediaStreamDescriptor& stream) { return stream.kind == kind; });
  require(found != index.streams.end(), "expected Stream kind is absent");
  return *found;
}

void verify_admitted(const std::filesystem::path& fixture_root,
                     const AdmittedFixture& fixture) {
  BufferCompressedSource source{fixture_root / fixture.id / fixture.file_name};
  require(source.content_digest() == fixture.source_digest,
          std::string{fixture.id} + " source digest differs");
  require(source.byte_size() == fixture.source_bytes,
          std::string{fixture.id} + " source size differs");

  FfmpegMediaDemuxer demuxer;
  const auto result = demuxer.build_index(source);
  require(result.succeeded(), std::string{fixture.id} + " rejected: " +
                                  result.code + ": " + result.diagnostic);
  const auto& index = *result.index;
  const auto validation = validate_media_index(index);
  require(validation.valid, std::string{fixture.id} + " invalid index: " +
                                validation.code + ": " + validation.message);
  require(index.source_digest == fixture.source_digest &&
              index.source_byte_size == fixture.source_bytes &&
              index.container_profile == fixture.container,
          std::string{fixture.id} + " immutable source identity differs");
  const auto expected_stream_count = fixture.has_audio ? 2U : 1U;
  require(index.streams.size() == expected_stream_count &&
              index.samples_decode_order.size() == fixture.sample_count,
          std::string{fixture.id} + " Stream/sample count differs");

  const auto& video = stream_of_kind(index, MediaStreamKind::video);
  const auto& video_format = std::get<VideoStreamFormat>(video.format);
  require(video.start == fixture.video_start &&
              video.duration == fixture.video_duration &&
              video.time_base == MediaTimeBase{.numerator = 1,
                                                .denominator = fixture.video_time_base_denominator},
          std::string{fixture.id} + " Video timing differs");
  require(video_format.coded_extent.width_pixels == fixture.coded_width &&
              video_format.coded_extent.height_pixels == fixture.coded_height &&
              video_format.display_extent.width_pixels == fixture.display_width &&
              video_format.display_extent.height_pixels == fixture.display_height &&
              video_format.orientation_degrees == fixture.orientation,
          std::string{fixture.id} + " Video geometry/orientation differs");
  const auto audio = std::find_if(
      index.streams.begin(), index.streams.end(),
      [](const MediaStreamDescriptor& stream) {
        return stream.kind == MediaStreamKind::audio;
      });
  require((audio != index.streams.end()) == fixture.has_audio,
          std::string{fixture.id} + " Audio presence differs");
  if (fixture.has_audio) {
    const auto& audio_format = std::get<AudioStreamFormat>(audio->format);
    require(audio_format.sample_rate_hz == fixture.audio_rate &&
                audio_format.channels == 1,
            std::string{fixture.id} + " Audio format differs");
  }
  const auto defaulted_transfer = std::any_of(
      index.notices.begin(), index.notices.end(),
      [](const MediaIndexNotice& notice) {
        return notice.kind ==
               MediaIndexNoticeKind::bt709_transfer_defaulted;
      });
  require(defaulted_transfer == fixture.expected_defaulted_transfer,
          std::string{fixture.id} +
              " BT.709 transfer normalization receipt differs");

  const auto& first = index.samples_decode_order.front();
  require(first.presentation_timestamp == fixture.first_pts &&
              first.decode_timestamp == fixture.first_dts &&
              first.duration == fixture.first_duration &&
              first.byte_offset == fixture.first_byte_offset &&
              first.byte_size == fixture.first_byte_size,
          std::string{fixture.id} + " first compressed sample differs");

  require(index.codec_configurations.size() == index.streams.size(),
          std::string{fixture.id} + " codec configurations are absent");
  const auto digest = media_index_digest(index);
  require(digest == fixture.expected_index_digest,
          std::string{fixture.id} + " canonical MediaIndex digest differs: " +
              digest);

  const auto decode_projection = project_media_index_for_hardware_decode(
      index, video.stream_id);
  require(decode_projection.succeeded(),
          std::string{fixture.id} + " decode projection rejected: " +
              decode_projection.code + ": " + decode_projection.diagnostic);
  const auto expected_video_samples = static_cast<std::size_t>(std::count_if(
      index.samples_decode_order.begin(), index.samples_decode_order.end(),
      [&video](const CompressedSample& sample) {
        return sample.stream_id == video.stream_id;
      }));
  require(decode_projection.projection->samples_decode_order.size() ==
                  expected_video_samples &&
              decode_projection.projection->codec_configuration_digest ==
                  video.codec_configuration_digest &&
              decode_projection.projection->expected_profile.coded_width ==
                  fixture.coded_width &&
              decode_projection.projection->expected_profile.coded_height ==
                  fixture.coded_height,
          std::string{fixture.id} +
              " shared MediaIndex did not produce exact scheduler inputs");
  std::cout << fixture.id << " media_index_digest=" << digest << '\n';
}

void verify_rejected(const std::filesystem::path& fixture_root,
                     const char* id, const char* file_name,
                     const char* expected_source_digest,
                     const MediaDemuxState expected_state,
                     const char* expected_code) {
  BufferCompressedSource source{fixture_root / id / file_name};
  require(source.content_digest() == expected_source_digest,
          std::string{id} + " source digest differs");
  FfmpegMediaDemuxer demuxer;
  const auto result = demuxer.build_index(source);
  require(!result.succeeded() && !result.index.has_value(),
          std::string{id} + " unexpectedly produced an index");
  require(result.state == expected_state && result.code == expected_code,
          std::string{id} + " wrong rejection: " + result.code + ": " +
              result.diagnostic);
}

}  // namespace

int main() {
  const std::filesystem::path fixture_root{REFUSION_VIDEO_IMPORT_FIXTURE_ROOT};

  {
    BufferCompressedSource source{
        fixture_root / "mp4-vfr-bframes-aac-offset" / "source.mp4"};
    AlwaysCancelled cancelled;
    FfmpegMediaDemuxer demuxer;
    const auto result = demuxer.build_index(source, &cancelled);
    require(result.state == MediaDemuxState::cancelled &&
                result.code == "RFX-MEDIA-IMPORT-CANCELLED" &&
                !result.index.has_value(),
            "cancelled demux published an index");
  }

  {
    std::vector<std::uint8_t> invalid_container(4'096, 0x51);
    MemoryCompressedSource source{std::move(invalid_container)};
    FfmpegMediaDemuxer demuxer;
    const auto result = demuxer.build_index(source);
    require(result.state == MediaDemuxState::unsupported_container &&
                result.code == "RFX-MEDIA-IMPORT-CONTAINER-UNSUPPORTED" &&
                !result.index.has_value(),
            "non-ISO-BMFF bytes did not fail closed as unsupported container");
  }

  verify_admitted(
      fixture_root,
      AdmittedFixture{
          .id = "mp4-vfr-bframes-aac-offset",
          .file_name = "source.mp4",
          .source_digest =
              "sha256:f1f236f5e63c1fe52ad15888a6436fb757af4ced9d863dc867399645407f980e",
          .source_bytes = 518'513,
          .expected_index_digest =
              "sha256:bebd0bf5ad5e88563ab81ea63662ddc643180db520375b962371790ee151c91a",
          .container = MediaContainerProfile::iso_bmff_mp4,
          .sample_count = 223,
          .video_start = 15'990,
          .video_duration = 89'000,
          .video_time_base_denominator = 30'000,
          .coded_width = 640,
          .coded_height = 360,
          .display_width = 640,
          .display_height = 360,
          .orientation = 0,
          .has_audio = true,
          .audio_rate = 48'000,
          .expected_defaulted_transfer = false,
          .first_pts = 15'990,
          .first_dts = 13'990,
          .first_duration = 1'000,
          .first_byte_offset = 4'503,
          .first_byte_size = 9'899,
      });
  verify_admitted(
      fixture_root,
      AdmittedFixture{
          .id = "mov-portrait-rotation-aac",
          .file_name = "source.mov",
          .source_digest =
              "sha256:588dfa30ac1bfffeedf5726b0e6bbbb1e621d44f1ec2e186337af16c7ec68d62",
          .source_bytes = 267'335,
          .expected_index_digest =
              "sha256:0f53e5900bb1d93fc6d45866dd369817cccca2c8d4a47e4e5149b5c33e8a4a7a",
          .container = MediaContainerProfile::quicktime_mov,
          .sample_count = 148,
          .video_start = 0,
          .video_duration = 30'720,
          .video_time_base_denominator = 15'360,
          .coded_width = 640,
          .coded_height = 360,
          .display_width = 360,
          .display_height = 640,
          .orientation = 90,
          .has_audio = true,
          .audio_rate = 44'100,
          .expected_defaulted_transfer = false,
          .first_pts = 0,
          .first_dts = -1'024,
          .first_duration = 512,
          .first_byte_offset = 36,
          .first_byte_size = 9'891,
      });
  verify_admitted(
      fixture_root,
      AdmittedFixture{
          .id = "mp4-landscape-1080p60",
          .file_name = "source.mp4",
          .source_digest =
              "sha256:c8ae10cb20f460184e910443383fa44d1c135468e8f0f3bad6f3c0bfd2302622",
          .source_bytes = 2'043'107,
          .expected_index_digest =
              "sha256:70c092e5ca8c75830c4ccfa561eec97ae376b2189898e899a8fab3bd12b4b1c2",
          .container = MediaContainerProfile::iso_bmff_mp4,
          .sample_count = 215,
          .video_start = 0,
          .video_duration = 30'720,
          .video_time_base_denominator = 15'360,
          .coded_width = 1'920,
          .coded_height = 1'080,
          .display_width = 1'920,
          .display_height = 1'080,
          .orientation = 0,
          .has_audio = true,
          .audio_rate = 48'000,
          .expected_defaulted_transfer = false,
          .first_pts = 0,
          .first_dts = -512,
          .first_duration = 256,
          .first_byte_offset = 4'658,
          .first_byte_size = 38'177,
      });
  verify_admitted(
      fixture_root,
      AdmittedFixture{
          .id = "mp4-portrait-4k30-partial-bt709",
          .file_name = "source.mp4",
          .source_digest =
              "sha256:d389c5fea01ca727b9ff967eac4955b04f643de58aed6e858a517ede5c088d2a",
          .source_bytes = 94'538,
          .expected_index_digest =
              "sha256:5d4aa884bcc403f7f2cecf6a711727c9103e444ad8ae2e820a89bfbb1f37248e",
          .container = MediaContainerProfile::iso_bmff_mp4,
          .sample_count = 6,
          .video_start = 0,
          .video_duration = 3'072,
          .video_time_base_denominator = 15'360,
          .coded_width = 2'160,
          .coded_height = 3'840,
          .display_width = 2'160,
          .display_height = 3'840,
          .orientation = 0,
          .has_audio = false,
          .audio_rate = 0,
          .expected_defaulted_transfer = true,
          .first_pts = 0,
          .first_dts = -1'024,
          .first_duration = 512,
          .first_byte_offset = 1'414,
          .first_byte_size = 53'755,
      });

  verify_rejected(
      fixture_root, "mp4-unsupported-hevc", "source.mp4",
      "sha256:c76310952bacba26d53f966c9bd5312017e8834a86f3c7620f2f19ab5671fe9b",
      MediaDemuxState::unsupported_profile,
      "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED");
  verify_rejected(
      fixture_root, "mp4-corrupt-truncated", "source.mp4",
      "sha256:906c23a62e78b98b4acd95a73c61dcdf8433ce7f83c68c81a08da6ca44fdbbb7",
      MediaDemuxState::corrupt, "RFX-MEDIA-IMPORT-CORRUPT");
  verify_rejected(
      fixture_root, "mp4-encrypted-cenc", "source.mp4",
      "sha256:3168851c6687129c9ec264c81be0ed5a8cc8e24685fdd366e9df514712771945",
      MediaDemuxState::encrypted, "RFX-MEDIA-IMPORT-ENCRYPTED");

  std::cout << "FFmpeg demux-only MediaIndex fixtures passed\n";
  return 0;
}
