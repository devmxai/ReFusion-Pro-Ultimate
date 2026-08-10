#pragma once

#include "refusion/runtime/render/VisualOutputContract.hpp"
#include "refusion/runtime/render/ViewportMapping.hpp"

#include <cstdint>
#include <memory>

class SkSurface;

namespace refusion::adapters::skia {

class SkiaTextLayoutEngine;
class SkiaVideoFrameResolver;

// Common exact-time evaluation and drawing boundary. It owns the reusable
// full-resolution Composition surface so all native backends receive identical
// raster and sampling behavior.
class SkiaVisualProgramExecutor final {
 public:
  SkiaVisualProgramExecutor();
  ~SkiaVisualProgramExecutor();

  SkiaVisualProgramExecutor(const SkiaVisualProgramExecutor&) = delete;
  SkiaVisualProgramExecutor& operator=(const SkiaVisualProgramExecutor&) = delete;
  SkiaVisualProgramExecutor(SkiaVisualProgramExecutor&&) noexcept;
  SkiaVisualProgramExecutor& operator=(SkiaVisualProgramExecutor&&) noexcept;

  void execute(
      SkSurface& target_surface,
      SkiaTextLayoutEngine& text_layout_engine,
      const runtime::render::VisualRenderProgram& program,
      std::uint64_t presentation_sequence,
      runtime::render::ProjectTimeNs project_time_ns,
      std::uint64_t transport_epoch_id,
      runtime::render::VisualOutputConsumer output_consumer,
      const runtime::render::CanvasViewportState& canvas_view,
      std::uint32_t target_width_pixels,
      std::uint32_t target_height_pixels,
      SkiaVideoFrameResolver* video_frames = nullptr);

 private:
  struct Implementation;
  std::unique_ptr<Implementation> implementation_;
};

}  // namespace refusion::adapters::skia
