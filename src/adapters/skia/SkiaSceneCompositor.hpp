#pragma once

#include "refusion/runtime/render/VisualRenderPlan.hpp"

class SkCanvas;

namespace refusion::adapters::skia {

class SkiaTextLayoutEngine;

// Executes only the shared, backend-neutral visual plan. This translation unit
// is compiled unchanged for Metal, D3D12 and Vulkan lanes.
void draw_visual_render_plan(
    SkCanvas& canvas,
    SkiaTextLayoutEngine& text_layout_engine,
    const runtime::render::VisualRenderPlan& plan);

}  // namespace refusion::adapters::skia
