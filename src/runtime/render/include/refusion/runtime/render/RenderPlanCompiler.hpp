#pragma once

#include "refusion/core/ProjectDocument.hpp"
#include "refusion/core/TextLayout.hpp"
#include "refusion/runtime/render/VisualRenderPlan.hpp"

namespace refusion::runtime::render {

// Candidate compilation is fallible and runs before accepted-revision
// publication. The returned program is immutable and safe to publish by swap.
[[nodiscard]] VisualRenderProgram compile_visual_render_program(
    const core::ProjectSnapshot& project);

// Exact-time evaluation reuses Core's single hierarchy/animation evaluator,
// then lowers its immutable result to the backend-neutral drawing contract.
[[nodiscard]] VisualRenderPlan evaluate_visual_render_plan(
    const VisualRenderProgram& program,
    ProjectTimeNs project_time_ns,
    std::uint64_t clock_epoch,
    core::TextLayoutPort& text_layout_port);

}  // namespace refusion::runtime::render
