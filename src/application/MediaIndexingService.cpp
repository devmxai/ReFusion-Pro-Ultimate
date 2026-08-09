#include "refusion/application/MediaIndexingService.hpp"

#include <future>
#include <stdexcept>
#include <utility>

namespace refusion::application {
namespace {

[[nodiscard]] MediaDemuxResult rejected(MediaDemuxState state,
                                        std::string code,
                                        std::string diagnostic) {
  return MediaDemuxResult{
      .state = state,
      .index = std::nullopt,
      .code = std::move(code),
      .diagnostic = std::move(diagnostic),
  };
}

[[nodiscard]] std::future<MediaDemuxResult> ready_future(
    MediaDemuxResult result) {
  std::promise<MediaDemuxResult> promise;
  auto future = promise.get_future();
  promise.set_value(std::move(result));
  return future;
}

}  // namespace

MediaIndexingService::MediaIndexingService(
    MediaDemuxPort& demuxer, MediaIndexCachePort* cache,
    const std::size_t maximum_concurrent_jobs)
    : demuxer_(demuxer),
      cache_(cache),
      maximum_concurrent_jobs_(maximum_concurrent_jobs) {
  if (maximum_concurrent_jobs_ == 0 || maximum_concurrent_jobs_ > 4) {
    throw std::invalid_argument(
        "MediaIndexingService concurrency must be in [1, 4]");
  }
}

MediaIndexingService::~MediaIndexingService() {
  std::unique_lock lock(mutex_);
  stopping_ = true;
  completed_.wait(lock, [this] { return active_jobs_ == 0; });
}

std::future<MediaDemuxResult> MediaIndexingService::index_async(
    MediaIndexingRequest request) {
  if (!request.source) {
    return ready_future(rejected(MediaDemuxState::invalid_source,
                                 "RFX-MEDIA-IMPORT-SOURCE-INVALID",
                                 "media indexing requires a source lease"));
  }
  {
    std::lock_guard lock(mutex_);
    if (stopping_) {
      return ready_future(rejected(MediaDemuxState::provider_failure,
                                   "RFX-MEDIA-INDEX-SERVICE-STOPPING",
                                   "media indexing service is stopping"));
    }
    if (active_jobs_ >= maximum_concurrent_jobs_) {
      return ready_future(rejected(
          MediaDemuxState::provider_failure, "RFX-MEDIA-INDEX-CAPACITY",
          "bounded media indexing capacity is exhausted"));
    }
    ++active_jobs_;
  }

  try {
    return std::async(std::launch::async,
                      [this, request = std::move(request)]() mutable {
                        struct Completion final {
                          MediaIndexingService* service;
                          ~Completion() { service->finish_job(); }
                        } completion{this};
                        return execute(std::move(request));
                      });
  } catch (...) {
    finish_job();
    throw;
  }
}

MediaDemuxResult MediaIndexingService::execute(MediaIndexingRequest request) {
  const auto* cancellation = request.cancellation.get();
  if (cancellation != nullptr && cancellation->cancelled()) {
    return rejected(MediaDemuxState::cancelled,
                    "RFX-MEDIA-IMPORT-CANCELLED",
                    "media indexing was cancelled before cache admission");
  }

  const MediaIndexCacheKey key{
      .contract_version = 1,
      .source_digest = request.source->content_digest(),
      .source_byte_size = request.source->byte_size(),
  };
  if (request.permit_cache && cache_ != nullptr) {
    auto cached = cache_->load(key);
    if (cached) {
      const auto validation = core::validate_media_index(*cached);
      if (validation.valid && cached->contract_version == key.contract_version &&
          cached->source_digest == key.source_digest &&
          cached->source_byte_size == key.source_byte_size) {
        return MediaDemuxResult{
            .state = MediaDemuxState::indexed,
            .index = std::move(cached),
            .code = "RFX-MEDIA-INDEX-CACHE-HIT",
            .diagnostic =
                "validated content-addressed MediaIndex cache entry reused",
        };
      }
      cache_->invalidate(key);
    }
  }

  auto result = demuxer_.build_index(*request.source, cancellation);
  if (!result.succeeded()) return result;
  if (cancellation != nullptr && cancellation->cancelled()) {
    return rejected(MediaDemuxState::cancelled,
                    "RFX-MEDIA-IMPORT-CANCELLED",
                    "media indexing was cancelled before cache publication");
  }
  if (request.permit_cache && cache_ != nullptr) {
    static_cast<void>(cache_->store(key, *result.index));
  }
  return result;
}

void MediaIndexingService::finish_job() noexcept {
  {
    std::lock_guard lock(mutex_);
    if (active_jobs_ > 0) --active_jobs_;
  }
  completed_.notify_all();
}

}  // namespace refusion::application
