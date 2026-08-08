#include "refusion/runtime/render/VisualOutputContract.hpp"

#include "refusion/runtime/render/RenderPlanCompiler.hpp"

#include <stdexcept>

namespace refusion::runtime::render {
namespace {

[[nodiscard]] bool known_consumer(
    const VisualOutputConsumer consumer) noexcept {
  switch (consumer) {
    case VisualOutputConsumer::interactive_preview:
    case VisualOutputConsumer::offline_export:
      return true;
  }
  return false;
}

[[nodiscard]] bool complementary_consumers(
    const VisualOutputConsumer first,
    const VisualOutputConsumer second) noexcept {
  return (first == VisualOutputConsumer::interactive_preview &&
          second == VisualOutputConsumer::offline_export) ||
         (first == VisualOutputConsumer::offline_export &&
          second == VisualOutputConsumer::interactive_preview);
}

}  // namespace

bool VisualOutputFrame::valid() const noexcept {
  return known_consumer(consumer) && plan.valid();
}

VisualOutputFrame prepare_visual_output_frame(
    const VisualOutputConsumer consumer,
    const VisualRenderProgram& program,
    const ProjectTimeNs project_time_ns,
    const std::uint64_t clock_epoch,
    core::TextLayoutPort& text_layout_port) {
  if (!known_consumer(consumer)) {
    throw std::invalid_argument(
        "RFX-VISUAL-OUTPUT-001: unknown visual output consumer");
  }
  return VisualOutputFrame{
      .consumer = consumer,
      .plan = evaluate_visual_render_plan(
          program, project_time_ns, clock_epoch, text_layout_port),
  };
}

VisualOutputParityReceipt compare_visual_output_semantics(
    const VisualOutputFrame& first,
    const VisualOutputFrame& second) {
  if (!first.valid() || !second.valid()) {
    return {
        .code = "RFX-VISUAL-OUTPUT-PARITY-001",
        .diagnostic = "visual output parity requires two valid frames",
    };
  }
  if (!complementary_consumers(first.consumer, second.consumer)) {
    return {
        .code = "RFX-VISUAL-OUTPUT-PARITY-002",
        .diagnostic = "parity requires one Preview and one Offline Export frame",
    };
  }
  const auto& left = first.plan;
  const auto& right = second.plan;
  const bool same_semantic_sample =
      left.stamp.project_id == right.stamp.project_id &&
      left.stamp.revision == right.stamp.revision &&
      left.stamp.composition_id == right.stamp.composition_id &&
      left.stamp.project_time_ns == right.stamp.project_time_ns &&
      left.canvas_width_pixels == right.canvas_width_pixels &&
      left.canvas_height_pixels == right.canvas_height_pixels &&
      left.semantic_digest == right.semantic_digest;
  if (!same_semantic_sample) {
    return {
        .code = "RFX-VISUAL-OUTPUT-PARITY-003",
        .diagnostic =
            "Preview and Offline Export resolved different visual semantics",
    };
  }
  return {
      .matched = true,
      .code = "RFX-VISUAL-OUTPUT-PARITY-MATCHED",
      .semantic_digest = left.semantic_digest,
  };
}

}  // namespace refusion::runtime::render
