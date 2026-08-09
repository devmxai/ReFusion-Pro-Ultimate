#pragma once

#include "refusion/core/MediaIndex.hpp"
#include "refusion/runtime/media/HardwareVideoDecode.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace refusion::runtime::media {

enum class MediaIndexDecodeProjectionState : std::uint8_t {
  ready,
  invalid_index,
  stream_missing,
  stream_not_video,
  unsupported_profile,
  timing_not_representable,
  ambiguous_presentation_order,
};

// Immutable, provider-neutral input prepared once from the shared MediaIndex.
// Platform decoders consume this result and compressed byte ranges; they never
// parse MP4/MOV structure or reinterpret project/media timing.
struct MediaIndexVideoDecodeProjection final {
  std::string source_digest;
  std::uint64_t source_byte_size{0};
  core::MediaStreamId stream_id;
  StrictDecodeProfile expected_profile;
  std::string codec_configuration_digest;
  std::vector<std::uint8_t> codec_configuration;
  std::vector<CompressedSampleDescriptor> samples_decode_order;

  [[nodiscard]] bool valid() const noexcept;
};

struct MediaIndexDecodeProjectionResult final {
  MediaIndexDecodeProjectionState state{
      MediaIndexDecodeProjectionState::invalid_index};
  std::optional<MediaIndexVideoDecodeProjection> projection;
  std::string code;
  std::string diagnostic;

  [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] MediaIndexDecodeProjectionResult
project_media_index_for_hardware_decode(
    const core::MediaIndex& index, const core::MediaStreamId& video_stream_id);

}  // namespace refusion::runtime::media
