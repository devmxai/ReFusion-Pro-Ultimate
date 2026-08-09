#pragma once

#include "refusion/application/MediaDemuxPort.hpp"

namespace refusion::adapters::media {

class FfmpegMediaDemuxer final : public application::MediaDemuxPort {
 public:
  [[nodiscard]] application::MediaDemuxResult build_index(
      application::ImmutableCompressedSourceLease& source,
      const application::MediaCancellationToken* cancellation = nullptr)
      override;
};

}  // namespace refusion::adapters::media
