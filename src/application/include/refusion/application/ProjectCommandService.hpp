#pragma once

#include "refusion/application/ProjectCandidateAdmission.hpp"
#include "refusion/core/ProjectAuthority.hpp"

#include <memory>

namespace refusion::application {

// Minimal accepted-revision boundary used by long-running shared workflows.
// A host may marshal these two operations onto its engine thread without
// exposing every authoring command or mutable Core authority.
class ProjectRevisionService {
 public:
  virtual ~ProjectRevisionService() = default;
  [[nodiscard]] virtual core::ProjectSnapshot active_snapshot() const = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::ReplaceProjectCommand& command) = 0;
};

class ProjectCommandService : public ProjectRevisionService {
 public:
  virtual ~ProjectCommandService() = default;

  using ProjectRevisionService::submit;

  virtual void set_candidate_admission_port(
      std::shared_ptr<ProjectCandidateAdmissionPort> port) = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::RenameProjectCommand& command) = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::SetVisualTransformCommand& command) = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::SetVisualPropertyCommand& command) = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::SetLayerEffectsCommand& command) = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::SetLayerMasksCommand& command) = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::AddVisualLayerCommand& command) = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::GroupNodesCommand& command) = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::ReparentNodesCommand& command) = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::AddEffectCommand& command) = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::AlignNodesCommand& command) = 0;
  [[nodiscard]] virtual core::ApplyResult
  submit(const core::AnimateEffectPropertyCommand& command) = 0;
};

// The concrete Application Host is intentionally hidden from UI and CLI code.
// This factory is the process composition boundary; returned clients can submit
// commands but cannot construct or access the mutable Core authority.
[[nodiscard]] std::unique_ptr<ProjectCommandService>
create_application_host(
    core::ProjectSnapshot initial_snapshot,
    std::shared_ptr<core::TextLayoutPort> text_layout_port = nullptr);

} // namespace refusion::application
