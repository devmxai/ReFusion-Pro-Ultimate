#pragma once

#include "refusion/runtime/render/VisualOutputContract.hpp"

#include <cstdint>

class SkCanvas;

namespace refusion::adapters::skia {

class SkiaTextLayoutEngine;

// Common exact-time evaluation and drawing boundary. Native Metal/D3D/Vulkan
// bindings provide only a canvas/target and never include authoring documents
// or lower project semantics independently.
void execute_visual_render_program(
    SkCanvas& canvas,
    SkiaTextLayoutEngine& text_layout_engine,
    const runtime::render::VisualRenderProgram& program,
    runtime::render::ProjectTimeNs project_time_ns,
    std::uint64_t transport_epoch_id,
    runtime::render::VisualOutputConsumer output_consumer,
    float target_width,
    float target_height);

}  // namespace refusion::adapters::skia
