#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <QString>

#include <optional>

struct OpenedProject final {
  refusion::core::ProjectSnapshot snapshot;
  QString canonical_path;
};

struct ProjectOpenResult final {
  std::optional<OpenedProject> project;
  QString diagnostic;

  [[nodiscard]] bool succeeded() const noexcept { return project.has_value(); }
};

[[nodiscard]] ProjectOpenResult open_refusion_project(const QString& path) noexcept;
