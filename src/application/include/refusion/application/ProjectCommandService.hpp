#pragma once

#include "refusion/core/ProjectAuthority.hpp"

#include <memory>

namespace refusion::application {

class ProjectCommandService {
 public:
  virtual ~ProjectCommandService() = default;

  [[nodiscard]] virtual core::ProjectSnapshot active_snapshot() const = 0;
  [[nodiscard]] virtual core::ApplyResult submit(
      const core::RenameProjectCommand& command) = 0;
};

// The concrete Application Host is intentionally hidden from UI and CLI code.
// This factory is the process composition boundary; returned clients can submit
// commands but cannot construct or access the mutable Core authority.
[[nodiscard]] std::unique_ptr<ProjectCommandService> create_application_host(
    core::ProjectSnapshot initial_snapshot);

}  // namespace refusion::application
