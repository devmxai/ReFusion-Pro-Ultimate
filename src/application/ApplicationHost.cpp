#include "refusion/application/ProjectCommandService.hpp"

#include <memory>
#include <utility>

namespace refusion::application {
namespace {

class ApplicationHost final : public ProjectCommandService {
 public:
  explicit ApplicationHost(core::ProjectSnapshot initial_snapshot)
      : authority_(std::move(initial_snapshot)) {}

  [[nodiscard]] core::ProjectSnapshot active_snapshot() const override {
    return authority_.active_snapshot();
  }

  [[nodiscard]] core::ApplyResult submit(
      const core::RenameProjectCommand& command) override {
    return authority_.apply(command);
  }

 private:
  core::ProjectAuthority authority_;
};

}  // namespace

std::unique_ptr<ProjectCommandService> create_application_host(
    core::ProjectSnapshot initial_snapshot) {
  return std::make_unique<ApplicationHost>(std::move(initial_snapshot));
}

}  // namespace refusion::application
