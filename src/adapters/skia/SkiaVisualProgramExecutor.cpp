#include "SkiaVisualProgramExecutor.hpp"

#include "SkiaSceneCompositor.hpp"
#include "SkiaTextLayoutInternal.hpp"

#include "refusion/runtime/render/VisualOutputContract.hpp"

namespace refusion::adapters::skia {

void execute_visual_render_program(
    SkCanvas& canvas,
    SkiaTextLayoutEngine& text_layout_engine,
    const runtime::render::VisualRenderProgram& program,
    const runtime::render::ProjectTimeNs project_time_ns,
    const std::uint64_t transport_epoch_id,
    const runtime::render::VisualOutputConsumer output_consumer,
    const float target_width,
    const float target_height) {
  const auto frame = runtime::render::prepare_visual_output_frame(
      output_consumer, program, project_time_ns, transport_epoch_id,
      text_layout_engine);
  draw_visual_render_plan(canvas, text_layout_engine, frame.plan, target_width,
                          target_height);
}

}  // namespace refusion::adapters::skia
