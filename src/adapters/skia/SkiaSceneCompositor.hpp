#pragma once

#include "refusion/runtime/render/VisualRenderPlan.hpp"
#include "include/core/SkRefCnt.h"

class SkCanvas;
class SkImage;

namespace refusion::adapters::skia {

class SkiaTextLayoutEngine;

class SkiaVideoFrameResolver {
 public:
  virtual ~SkiaVideoFrameResolver() = default;
  [[nodiscard]] virtual sk_sp<SkImage> resolve_video_frame(
      const runtime::render::DrawVideoFrame& frame) = 0;
};

// Executes only the shared, backend-neutral visual plan. This translation unit
// is compiled unchanged for Metal, D3D12 and Vulkan lanes.
void draw_visual_render_plan(
    SkCanvas& canvas,
    SkiaTextLayoutEngine& text_layout_engine,
    const runtime::render::VisualRenderPlan& plan,
    SkiaVideoFrameResolver* video_frames = nullptr);

}  // namespace refusion::adapters::skia
