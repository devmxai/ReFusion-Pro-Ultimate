#include "refusion/adapters/media/FfmpegMediaDemuxer.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformMediaCapability.hpp"
#include "refusion/runtime/media/MediaIndexDecodeProjection.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(const bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

class FixtureSource final
    : public refusion::application::ImmutableCompressedSourceLease {
 public:
  explicit FixtureSource(std::string path)
      : path_(std::move(path)),
        size_(std::filesystem::file_size(path_)) {}

  [[nodiscard]] std::string content_digest() const override {
    return "sha256:f1f236f5e63c1fe52ad15888a6436fb757af4ced9d863dc867399645407f980e";
  }

  [[nodiscard]] std::uint64_t byte_size() const noexcept override {
    return size_;
  }

  [[nodiscard]] refusion::application::CompressedSourceReadResult read_at(
      const std::uint64_t offset,
      const std::span<std::uint8_t> destination) noexcept override {
    if (offset >= size_) {
      return {.state =
                  refusion::application::CompressedSourceReadState::end_of_source};
    }
    std::ifstream input(path_, std::ios::binary);
    if (!input ||
        offset > static_cast<std::uint64_t>(
                     std::numeric_limits<std::streamoff>::max())) {
      return {.state =
                  refusion::application::CompressedSourceReadState::failed};
    }
    input.seekg(static_cast<std::streamoff>(offset));
    const auto maximum = std::min<std::uint64_t>(
        destination.size(), size_ - offset);
    input.read(reinterpret_cast<char*>(destination.data()),
               static_cast<std::streamsize>(maximum));
    const auto count = input.gcount();
    if (count <= 0) {
      return {.state =
                  refusion::application::CompressedSourceReadState::failed};
    }
    return {
        .state = refusion::application::CompressedSourceReadState::read,
        .bytes_read = static_cast<std::size_t>(count),
    };
  }

  [[nodiscard]] const std::string& path() const noexcept { return path_; }

 private:
  std::string path_;
  std::uint64_t size_{0};
};

}  // namespace

int main() {
  FixtureSource source(REFUSION_VIDEO_PLAYBACK_SOURCE_PATH);
  require(source.byte_size() == 518'513,
          "imported Video fixture byte size changed");

  refusion::adapters::media::FfmpegMediaDemuxer demuxer;
  auto indexed = demuxer.build_index(source);
  require(indexed.succeeded(), "shared MP4 demux/index failed");
  const auto stream = std::find_if(
      indexed.index->streams.begin(), indexed.index->streams.end(),
      [](const auto& value) {
        return value.kind == refusion::core::MediaStreamKind::video;
      });
  require(stream != indexed.index->streams.end(),
          "fixture has no indexed Video stream");

  auto projected =
      refusion::runtime::media::project_media_index_for_hardware_decode(
          *indexed.index, stream->stream_id);
  require(projected.succeeded(), "MediaIndex decode projection failed");

  auto device = refusion::platform::create_platform_gpu_device_service();
  require(device != nullptr, "Apple GPU service is unavailable");
  auto decoder = refusion::platform::create_platform_hardware_video_decoder(
      *device);
  require(decoder != nullptr, "Apple hardware decoder service is unavailable");
  const auto& projection = *projected.projection;
  auto playback = decoder->open_playback({
      .source_path = source.path(),
      .source_byte_size = projection.source_byte_size,
      .expected_profile = projection.expected_profile,
      .codec_configuration = projection.codec_configuration,
      .samples_decode_order = projection.samples_decode_order,
  });
  require(playback != nullptr,
          "VideoToolbox rejected the imported MP4 playback source");

  auto presentation = projection.samples_decode_order;
  std::sort(presentation.begin(), presentation.end(),
            [](const auto& left, const auto& right) {
              return refusion::runtime::media::compare_exact_media_time(
                         left.timing.presentation_time,
                         right.timing.presentation_time) ==
                     std::strong_ordering::less;
            });
  require(presentation.size() > 16, "fixture playback window is too short");

  refusion::runtime::media::HardwareVideoPlaybackWindowResult forward;
  std::shared_ptr<const refusion::runtime::media::NativeVideoSurfaceLease>
      selected;
  for (std::size_t offset = 0; offset < presentation.size(); offset += 4) {
    forward = playback->decode_window({
        .target_presentation_time =
            presentation[offset].timing.presentation_time,
        .maximum_surface_count = 12,
        .lookahead_surface_count = 6,
    });
    require(forward.admitted(),
            "forward imported MP4 window was not decoded");
    selected = forward.queue->select_at(
        presentation[offset].timing.presentation_time);
    require(selected != nullptr &&
                selected->info().source_frame_index == offset,
            "exact VFR source frame selection differs");
    require(forward.counters.strict_path_clean(),
            "imported playback used a forbidden video-pixel path");
  }
  require(forward.counters.hardware_decoder_sessions == 1,
          "forward playback restarted the hardware decoder within one GOP stream");

  std::cout << "{\"hardware_playback\":true,\"selected_source_frame\":"
            << selected->info().source_frame_index
            << ",\"resident_surfaces\":" << forward.queue->size()
            << ",\"cpu_pixel_maps\":" << forward.counters.cpu_pixel_maps
            << ",\"cpu_pixel_uploads\":"
            << forward.counters.cpu_pixel_uploads
            << ",\"gpu_readbacks\":" << forward.counters.gpu_readbacks
            << "}\n";
}
