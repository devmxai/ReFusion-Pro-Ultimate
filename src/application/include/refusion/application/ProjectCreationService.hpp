#pragma once

#include "refusion/core/ProjectCreation.hpp"

#include <memory>
#include <vector>

namespace refusion::application {

class ProjectCreationService {
 public:
  virtual ~ProjectCreationService() = default;

  [[nodiscard]] virtual const std::vector<core::CompositionPresetDescriptor>&
  presets() const noexcept = 0;
  [[nodiscard]] virtual const std::vector<std::uint32_t>& frame_rates()
      const noexcept = 0;
  [[nodiscard]] virtual core::CreateProjectResult create(
      const core::CreateProjectRequest& request) = 0;
};

[[nodiscard]] std::unique_ptr<ProjectCreationService>
create_project_creation_service();

}  // namespace refusion::application
