#pragma once

#include "refusion/application/MediaDemuxPort.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace refusion::application {

struct MediaIndexCacheKey final {
  std::uint32_t contract_version{1};
  std::string source_digest;
  std::uint64_t source_byte_size{0};

  friend bool operator==(const MediaIndexCacheKey&,
                         const MediaIndexCacheKey&) = default;
};

// The cache is derived state. A cache implementation may persist or discard
// entries, but it never publishes project truth or changes MediaIndex meaning.
class MediaIndexCachePort {
 public:
  virtual ~MediaIndexCachePort() = default;

  [[nodiscard]] virtual std::optional<core::MediaIndex> load(
      const MediaIndexCacheKey& key) = 0;
  [[nodiscard]] virtual bool store(const MediaIndexCacheKey& key,
                                   const core::MediaIndex& index) = 0;
  virtual void invalidate(const MediaIndexCacheKey& key) = 0;
};

struct MediaIndexingRequest final {
  std::shared_ptr<ImmutableCompressedSourceLease> source;
  std::shared_ptr<const MediaCancellationToken> cancellation;
  bool permit_cache{true};
};

// Bounded asynchronous orchestration for CPU metadata/container parsing only.
// It neither decodes media nor publishes a Project revision. The service must
// outlive futures returned by index_async; destruction waits for admitted work.
class MediaIndexingService final {
 public:
  MediaIndexingService(MediaDemuxPort& demuxer, MediaIndexCachePort* cache,
                       std::size_t maximum_concurrent_jobs);
  ~MediaIndexingService();

  MediaIndexingService(const MediaIndexingService&) = delete;
  MediaIndexingService& operator=(const MediaIndexingService&) = delete;

  [[nodiscard]] std::future<MediaDemuxResult> index_async(
      MediaIndexingRequest request);

 private:
  [[nodiscard]] MediaDemuxResult execute(MediaIndexingRequest request);
  void finish_job() noexcept;

  MediaDemuxPort& demuxer_;
  MediaIndexCachePort* cache_{nullptr};
  std::size_t maximum_concurrent_jobs_{0};
  std::mutex mutex_;
  std::condition_variable completed_;
  std::size_t active_jobs_{0};
  bool stopping_{false};
};

}  // namespace refusion::application
