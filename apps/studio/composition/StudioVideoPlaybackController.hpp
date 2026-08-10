#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <QString>

#include <cstdint>
#include <memory>

namespace refusion::adapters::skia {
class SkiaGpuContexts;
}

namespace refusion::runtime::gpu {
class GpuDeviceService;
}

namespace refusion::runtime::presentation {
class ViewportRenderSession;
}

// Studio host for the engine-owned Video playback pipeline. It observes one
// accepted Project snapshot and ProjectClock state, but owns neither. All
// compressed indexing/timing semantics remain shared and decoded pixels remain
// in native GPU surface leases through Skia compositing.
class StudioVideoPlaybackController final {
 public:
  StudioVideoPlaybackController(
      QString project_path,
      const refusion::core::ProjectSnapshot& initial_project,
      refusion::runtime::gpu::GpuDeviceService& gpu_device_service,
      refusion::adapters::skia::SkiaGpuContexts& renderer,
      refusion::runtime::presentation::ViewportRenderSession& render_session);
  ~StudioVideoPlaybackController();

  StudioVideoPlaybackController(const StudioVideoPlaybackController&) = delete;
  StudioVideoPlaybackController& operator=(
      const StudioVideoPlaybackController&) = delete;

  void publishProject(
      const refusion::core::ProjectSnapshot& project) noexcept;

 private:
  class Implementation;
  std::unique_ptr<Implementation> implementation_;
};
