#pragma once

#include "refusion/core/MediaIndex.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace refusion::application {

enum class CompressedSourceReadState : std::uint8_t {
  read,
  end_of_source,
  failed,
};

struct CompressedSourceReadResult final {
  CompressedSourceReadState state{CompressedSourceReadState::failed};
  std::size_t bytes_read{0};
};

// Lifetime-bearing, random-access compressed-byte lease. It deliberately
// exposes no host path; filesystem portals, bookmarks and native path encoding
// remain adapter mechanics.
class ImmutableCompressedSourceLease {
 public:
  virtual ~ImmutableCompressedSourceLease() = default;

  [[nodiscard]] virtual std::string content_digest() const = 0;
  [[nodiscard]] virtual std::uint64_t byte_size() const noexcept = 0;
  [[nodiscard]] virtual CompressedSourceReadResult read_at(
      std::uint64_t offset,
      std::span<std::uint8_t> destination) noexcept = 0;
};

class MediaCancellationToken {
 public:
  virtual ~MediaCancellationToken() = default;

  [[nodiscard]] virtual bool cancelled() const noexcept = 0;
};

enum class MediaDemuxState : std::uint8_t {
  indexed,
  invalid_source,
  unsupported_container,
  encrypted,
  corrupt,
  unsupported_profile,
  cancelled,
  provider_failure,
};

struct MediaDemuxResult final {
  MediaDemuxState state{MediaDemuxState::provider_failure};
  std::optional<core::MediaIndex> index;
  std::string code;
  std::string diagnostic;

  [[nodiscard]] bool succeeded() const noexcept {
    return state == MediaDemuxState::indexed && index.has_value();
  }
};

class MediaDemuxPort {
 public:
  virtual ~MediaDemuxPort() = default;

  [[nodiscard]] virtual MediaDemuxResult build_index(
      ImmutableCompressedSourceLease& source,
      const MediaCancellationToken* cancellation = nullptr) = 0;
};

}  // namespace refusion::application
