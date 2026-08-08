#pragma once

#include "refusion/core/TextLayout.hpp"
#include "refusion/runtime/render/VisualRenderPlan.hpp"

#include <cstdint>
#include <string>

namespace refusion::runtime::render {

enum class VisualOutputConsumer : std::uint8_t {
  interactive_preview,
  offline_export,
};

// One exact-time plan prepared by the shared semantic route. The consumer
// identity is metadata only and cannot participate in plan lowering.
struct VisualOutputFrame final {
  VisualOutputConsumer consumer{VisualOutputConsumer::interactive_preview};
  VisualRenderPlan plan;

  [[nodiscard]] bool valid() const noexcept;
};

struct VisualOutputParityReceipt final {
  bool matched{false};
  std::string code;
  std::string diagnostic;
  std::string semantic_digest;
};

// Preview and future export call this same function. There is intentionally no
// consumer-specific switch in evaluation; output mechanics begin only after
// this shared VisualRenderPlan has been prepared.
[[nodiscard]] VisualOutputFrame prepare_visual_output_frame(
    VisualOutputConsumer consumer,
    const VisualRenderProgram& program,
    ProjectTimeNs project_time_ns,
    std::uint64_t clock_epoch,
    core::TextLayoutPort& text_layout_port);

// Qualification compares semantic identity while allowing independent clock
// epochs. Device/presenter/export scheduling must not redefine project pixels.
[[nodiscard]] VisualOutputParityReceipt compare_visual_output_semantics(
    const VisualOutputFrame& first,
    const VisualOutputFrame& second);

}  // namespace refusion::runtime::render
