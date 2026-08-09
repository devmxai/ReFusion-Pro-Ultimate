#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace refusion::core {

enum class MediaContainerProfile : std::uint8_t {
  iso_bmff_mp4,
  quicktime_mov,
};

enum class SampleDependencyValue : std::uint8_t {
  unknown,
  no,
  yes,
};

struct SampleDependencyFlags final {
  SampleDependencyValue leading{SampleDependencyValue::unknown};
  SampleDependencyValue depends_on_others{SampleDependencyValue::unknown};
  SampleDependencyValue is_depended_on{SampleDependencyValue::unknown};
  SampleDependencyValue has_redundancy{SampleDependencyValue::unknown};

  friend bool operator==(const SampleDependencyFlags&,
                         const SampleDependencyFlags&) = default;
};

struct CompressedSample final {
  MediaStreamId stream_id;
  std::uint64_t sample_index{0};
  std::uint64_t byte_offset{0};
  std::uint32_t byte_size{0};
  std::int64_t presentation_timestamp{0};
  std::int64_t decode_timestamp{0};
  std::uint64_t duration{0};
  MediaTimeBase time_base;
  bool sync_sample{false};
  bool discard_sample{false};
  SampleDependencyFlags dependencies;
  std::uint32_t sample_description_index{1};

  friend bool operator==(const CompressedSample&,
                         const CompressedSample&) = default;
};

struct MediaCodecConfiguration final {
  MediaStreamId stream_id;
  std::uint32_t sample_description_index{1};
  std::string content_digest;
  std::vector<std::uint8_t> bytes;

  friend bool operator==(const MediaCodecConfiguration&,
                         const MediaCodecConfiguration&) = default;
};

enum class MediaIndexNoticeKind : std::uint8_t {
  bt709_transfer_defaulted,
};

// A deterministic, non-blocking normalization applied while producing the
// derived MediaIndex. It is included in the index digest so cache entries and
// downstream clients cannot lose the fact that source metadata was partial.
struct MediaIndexNotice final {
  MediaStreamId stream_id;
  MediaIndexNoticeKind kind{MediaIndexNoticeKind::bt709_transfer_defaulted};

  friend bool operator==(const MediaIndexNotice&,
                         const MediaIndexNotice&) = default;
};

// Derived, rebuildable container truth. No provider object, host path, decoded
// pixel, native handle or project-time decision is legal in this record.
struct MediaIndex final {
  std::uint32_t contract_version{1};
  std::string source_digest;
  std::uint64_t source_byte_size{0};
  MediaContainerProfile container_profile{
      MediaContainerProfile::iso_bmff_mp4};
  std::vector<MediaStreamDescriptor> streams;
  std::vector<MediaIndexNotice> notices;
  std::vector<MediaCodecConfiguration> codec_configurations;
  std::vector<CompressedSample> samples_decode_order;

  friend bool operator==(const MediaIndex&, const MediaIndex&) = default;
};

[[nodiscard]] CompositionValidation validate_media_index(
    const MediaIndex& index);

[[nodiscard]] std::string canonical_media_index_bytes(
    const MediaIndex& index);

[[nodiscard]] std::string media_index_digest(const MediaIndex& index);

}  // namespace refusion::core
