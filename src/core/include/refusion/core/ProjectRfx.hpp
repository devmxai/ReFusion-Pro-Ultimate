#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace refusion::core {

struct RfxSourceLocation final {
  std::size_t byte_offset{0};
  std::size_t line{1};
  std::size_t column{1};
};

struct RfxDiagnostic final {
  std::string code;
  std::string message;
  RfxSourceLocation location;
};

struct RfxCompileResult final {
  std::optional<ProjectSnapshot> project;
  std::vector<RfxDiagnostic> diagnostics;

  [[nodiscard]] bool succeeded() const noexcept {
    return project.has_value() && diagnostics.empty();
  }
};

// Compile strict RFX1-RFX4 migration inputs or the bounded RFX5 contribution schema
// into the same portable Core snapshot consumed by UI, evaluator and renderer.
[[nodiscard]] RfxCompileResult compile_project_rfx(
    std::string_view source) noexcept;

// Produce canonical RFX5 spelling. This is intentionally a whole-document
// writer: project state is never persisted through ad-hoc text edits.
[[nodiscard]] std::string serialize_project_rfx(
    const ProjectSnapshot& project);

}  // namespace refusion::core
