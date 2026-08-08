#include "refusion/application/ProjectCreationService.hpp"

#include <memory>

namespace refusion::application {
namespace {

class DefaultProjectCreationService final : public ProjectCreationService {
 public:
  [[nodiscard]] const std::vector<core::CompositionPresetDescriptor>& presets()
      const noexcept override {
    return core::composition_presets();
  }

  [[nodiscard]] const std::vector<std::uint32_t>& frame_rates()
      const noexcept override {
    return core::supported_project_frame_rates();
  }

  [[nodiscard]] core::CreateProjectResult create(
      const core::CreateProjectRequest& request) override {
    return core::create_initial_project(request);
  }
};

}  // namespace

std::unique_ptr<ProjectCreationService> create_project_creation_service() {
  return std::make_unique<DefaultProjectCreationService>();
}

}  // namespace refusion::application
