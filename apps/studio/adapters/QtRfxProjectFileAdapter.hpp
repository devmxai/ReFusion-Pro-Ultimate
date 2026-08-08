#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <QByteArray>
#include <QString>

#include <optional>

struct OpenedProject final {
  refusion::core::ProjectSnapshot snapshot;
  QString canonical_path;
  QByteArray source_bytes;
};

struct ProjectOpenResult final {
  std::optional<OpenedProject> project;
  QString diagnostic;

  [[nodiscard]] bool succeeded() const noexcept { return project.has_value(); }
};

// Qt owns only filesystem adaptation. Project.rfx grammar and semantics are
// compiled by portable Core into the one ProjectSnapshot authority model.
[[nodiscard]] ProjectOpenResult open_refusion_project(const QString& path) noexcept;
