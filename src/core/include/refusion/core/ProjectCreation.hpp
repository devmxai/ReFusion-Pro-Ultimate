#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace refusion::core {

struct ProjectResolutionDescriptor final {
  std::string id;
  std::string display_name;
  CanvasExtent canvas;
};

struct CompositionPresetDescriptor final {
  std::string id;
  std::string display_name;
  std::string aspect_label;
  std::vector<ProjectResolutionDescriptor> resolutions;
};

struct CreateProjectRequest final {
  std::string display_name;
  std::string composition_preset_id;
  std::string resolution_id;
  std::uint32_t frame_rate{30};
  std::uint32_t duration_seconds{30};
};

struct CreateProjectResult final {
  std::optional<ProjectSnapshot> project;
  std::string code;
  std::string message;

  [[nodiscard]] bool succeeded() const noexcept {
    return project.has_value() && code.empty();
  }
};

[[nodiscard]] const std::vector<CompositionPresetDescriptor>&
composition_presets() noexcept;

[[nodiscard]] const std::vector<std::uint32_t>&
supported_project_frame_rates() noexcept;

// Creates a validated Revision 1 blueprint. IDs are engine-owned and are never
// accepted from a UI or filesystem client.
[[nodiscard]] CreateProjectResult create_initial_project(
    const CreateProjectRequest& request) noexcept;

}  // namespace refusion::core
