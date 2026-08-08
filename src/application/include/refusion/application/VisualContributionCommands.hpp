#pragma once

#include "refusion/application/ProjectCommandService.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"

#include <optional>
#include <string>
#include <vector>

namespace refusion::application {

struct VisualContributionParameterAssignment final {
  std::string parameter_id;
  core::VisualParameterValue value;

  friend bool operator==(const VisualContributionParameterAssignment&,
                         const VisualContributionParameterAssignment&) = default;
};

struct AddVisualContributionRequest final {
  core::CommandEnvelope envelope;
  core::VisualContributionCategory category{
      core::VisualContributionCategory::effect};
  core::LayerId layer_id;
  std::string descriptor_id;
  std::string instance_id;
  std::optional<std::size_t> insertion_index;
};

struct UpdateVisualContributionRequest final {
  core::CommandEnvelope envelope;
  core::VisualContributionCategory category{
      core::VisualContributionCategory::effect};
  core::LayerId layer_id;
  std::string instance_id;
  bool enabled{true};
  bool inverted{false};
  std::vector<VisualContributionParameterAssignment> parameters;
};

struct RemoveVisualContributionRequest final {
  core::CommandEnvelope envelope;
  core::VisualContributionCategory category{
      core::VisualContributionCategory::effect};
  core::LayerId layer_id;
  std::string instance_id;
};

// Qt/QML and future MCP adapters submit descriptor-addressed intent here. They
// never construct or replace LayerEffect/LayerMask project state themselves.
// The helper builds a candidate from the active snapshot and submits it through
// the same Application admission/publication transaction as every command.
[[nodiscard]] core::ApplyResult submit_visual_contribution(
    ProjectCommandService& commands,
    const AddVisualContributionRequest& request);
[[nodiscard]] core::ApplyResult submit_visual_contribution(
    ProjectCommandService& commands,
    const UpdateVisualContributionRequest& request);
[[nodiscard]] core::ApplyResult submit_visual_contribution(
    ProjectCommandService& commands,
    const RemoveVisualContributionRequest& request);

}  // namespace refusion::application
