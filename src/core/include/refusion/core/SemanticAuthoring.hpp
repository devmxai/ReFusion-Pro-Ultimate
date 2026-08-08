#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace refusion::core {

enum class SemanticLintSeverity : std::uint8_t {
  warning,
};

// Lint is advisory project analysis, separate from schema validation and typed
// intent postconditions. It may identify suspicious creative topology but never
// accepts, rejects or mutates a revision.
struct SemanticAuthoringIssue final {
  std::string code;
  std::string message;
  SemanticLintSeverity severity{SemanticLintSeverity::warning};
  std::vector<VisualNodeRef> nodes;

  friend bool operator==(const SemanticAuthoringIssue&,
                         const SemanticAuthoringIssue&) = default;
};

[[nodiscard]] std::vector<SemanticAuthoringIssue>
semantic_authoring_lint(const CompositionSnapshot& composition);

} // namespace refusion::core
