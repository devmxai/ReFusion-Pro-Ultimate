#include "refusion/application/MediaIndexingService.hpp"

#include "refusion/core/ContentDigest.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace {

using namespace refusion::application;
using namespace refusion::core;

void require(const bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

class MemorySource final : public ImmutableCompressedSourceLease {
 public:
  MemorySource() : bytes_(100, 0x4a), digest_(sha256_content_digest(bytes_)) {}

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

class AtomicCancellation final : public MediaCancellationToken {
 public:
  [[nodiscard]] bool cancelled() const noexcept override {
    return cancelled_.load();
  }
  void cancel() noexcept { cancelled_.store(true); }

 private:
  std::atomic_bool cancelled_{false};
};

[[nodiscard]] MediaIndex valid_index(
    const ImmutableCompressedSourceLease& source) {
  const std::vector<std::uint8_t> configuration{1, 100, 0, 40};
  const auto configuration_digest = sha256_content_digest(configuration);
  MediaIndex index{
      .contract_version = 1,
      .source_digest = source.content_digest(),
      .source_byte_size = source.byte_size(),
      .container_profile = MediaContainerProfile::iso_bmff_mp4,
  };
  index.streams.push_back(MediaStreamDescriptor{
      .stream_id = MediaStreamId{"stream_video"},
      .container_track_id = 1,
      .kind = MediaStreamKind::video,
      .codec = MediaCodec::h264_avc,
      .codec_configuration_digest = configuration_digest,
      .time_base = MediaTimeBase{.numerator = 1, .denominator = 30},
      .start = 0,
      .duration = 1,
      .format = VideoStreamFormat{
          .coded_extent = {.width_pixels = 16, .height_pixels = 16},
          .display_extent = {.width_pixels = 16, .height_pixels = 16},
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
  index.samples_decode_order.push_back(CompressedSample{
      .stream_id = MediaStreamId{"stream_video"},
      .sample_index = 0,
      .byte_offset = 0,
      .byte_size = 10,
      .presentation_timestamp = 0,
      .decode_timestamp = 0,
      .duration = 1,
      .time_base = MediaTimeBase{.numerator = 1, .denominator = 30},
      .sync_sample = true,
      .sample_description_index = 1,
  });
  return index;
}

class FakeDemux final : public MediaDemuxPort {
 public:
  [[nodiscard]] MediaDemuxResult build_index(
      ImmutableCompressedSourceLease& source,
      const MediaCancellationToken* cancellation = nullptr) override {
    ++calls;
    {
      std::unique_lock lock(mutex_);
      started_ = true;
      started_cv_.notify_all();
      release_cv_.wait(lock, [this] { return !blocking_ || released_; });
    }
    if (cancellation != nullptr && cancellation->cancelled()) {
      return {.state = MediaDemuxState::cancelled,
              .code = "RFX-MEDIA-IMPORT-CANCELLED"};
    }
    return {.state = MediaDemuxState::indexed,
            .index = valid_index(source),
            .code = "RFX-MEDIA-INDEX-READY"};
  }

  void block() {
    std::lock_guard lock(mutex_);
    blocking_ = true;
    released_ = false;
    started_ = false;
  }
  void wait_started() {
    std::unique_lock lock(mutex_);
    started_cv_.wait(lock, [this] { return started_; });
  }
  void release() {
    {
      std::lock_guard lock(mutex_);
      released_ = true;
      blocking_ = false;
    }
    release_cv_.notify_all();
  }

  std::atomic_uint64_t calls{0};

 private:
  std::mutex mutex_;
  std::condition_variable started_cv_;
  std::condition_variable release_cv_;
  bool blocking_{false};
  bool released_{false};
  bool started_{false};
};

class MemoryCache final : public MediaIndexCachePort {
 public:
  [[nodiscard]] std::optional<MediaIndex> load(
      const MediaIndexCacheKey& key) override {
    std::lock_guard lock(mutex_);
    ++loads;
    if (!value || !stored_key || *stored_key != key) return std::nullopt;
    return value;
  }
  [[nodiscard]] bool store(const MediaIndexCacheKey& key,
                           const MediaIndex& index) override {
    std::lock_guard lock(mutex_);
    ++stores;
    stored_key = key;
    value = index;
    return true;
  }
  void invalidate(const MediaIndexCacheKey&) override {
    std::lock_guard lock(mutex_);
    ++invalidations;
    value.reset();
    stored_key.reset();
  }

  std::mutex mutex_;
  std::optional<MediaIndexCacheKey> stored_key;
  std::optional<MediaIndex> value;
  std::uint64_t loads{0};
  std::uint64_t stores{0};
  std::uint64_t invalidations{0};
};

}  // namespace

int main() {
  auto source = std::make_shared<MemorySource>();

  FakeDemux demuxer;
  MemoryCache cache;
  MediaIndexingService service(demuxer, &cache, 1);

  auto first = service.index_async({.source = source});
  require(first.get().succeeded(), "first asynchronous index failed");
  auto second = service.index_async({.source = source});
  const auto cached = second.get();
  require(cached.succeeded() &&
              cached.code == "RFX-MEDIA-INDEX-CACHE-HIT" &&
              demuxer.calls == 1 && cache.stores == 1,
          "validated content-addressed cache was not reused");

  {
    std::lock_guard lock(cache.mutex_);
    cache.value->codec_configurations.front().bytes.front() ^= 0xffU;
  }
  auto rebuilt = service.index_async({.source = source});
  require(rebuilt.get().succeeded() && demuxer.calls == 2 &&
              cache.invalidations == 1 && cache.stores == 2,
          "invalid derived cache entry was not invalidated and rebuilt");

  auto cancelled = std::make_shared<AtomicCancellation>();
  cancelled->cancel();
  const auto cancelled_result =
      service.index_async({.source = source, .cancellation = cancelled}).get();
  require(cancelled_result.state == MediaDemuxState::cancelled &&
              !cancelled_result.index.has_value(),
          "pre-cancelled asynchronous indexing published an index");

  demuxer.block();
  auto admitted = service.index_async({.source = source, .permit_cache = false});
  demuxer.wait_started();
  const auto saturated =
      service.index_async({.source = source, .permit_cache = false}).get();
  require(saturated.code == "RFX-MEDIA-INDEX-CAPACITY" &&
              !saturated.index.has_value(),
          "bounded indexing service admitted excess concurrent work");
  demuxer.release();
  require(admitted.get().succeeded(), "admitted bounded indexing job failed");

  std::cout << "bounded asynchronous media indexing tests passed\n";
  return 0;
}
