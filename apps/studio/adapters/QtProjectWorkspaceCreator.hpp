#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <QString>

#include <optional>

struct CreatedProjectWorkspace final {
  QString project_path;
  QString project_directory;
};

struct WorkspaceCreateResult final {
  std::optional<CreatedProjectWorkspace> workspace;
  QString diagnostic;

  [[nodiscard]] bool succeeded() const noexcept {
    return workspace.has_value();
  }
};

// Desktop filesystem adapter. The selected directory must already exist and be
// empty; Project.rfx is promoted last as the workspace commit marker.
[[nodiscard]] WorkspaceCreateResult create_project_workspace(
    const QString& selected_directory,
    const refusion::core::ProjectSnapshot& project) noexcept;
