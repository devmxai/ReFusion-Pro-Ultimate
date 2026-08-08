#pragma once

#include "refusion/core/ProjectAuthority.hpp"

#include <memory>

namespace refusion::application {

// A fully prepared, unpublished Runtime candidate. All fallible semantic,
// capability, asset and render-program work completes before this is returned.
// Engine state is committed while Application excludes accepted-state readers.
// UI/observer projection is a distinct no-fail phase after that lock is
// released, so Qt/model notifications may read the newly accepted snapshot
// without re-entering the admission mutex.
class PreparedProjectRevision {
 public:
  virtual ~PreparedProjectRevision() = default;

  // Bounded pointer/state swaps only. Implementations must not emit Qt signals,
  // reset UI models or call back into ProjectCommandService.
  virtual void commit_engine_state() noexcept = 0;

  // Runs synchronously after Core and Runtime are committed and the admission
  // lock is released. It may publish immutable UI/diagnostic projections, but
  // must not mutate project authority or perform fallible preparation.
  virtual void publish_observer_projections() noexcept {}
};

struct CandidatePreparationResult final {
  std::unique_ptr<PreparedProjectRevision> prepared;
  core::Diagnostic diagnostic;

  [[nodiscard]] bool succeeded() const noexcept {
    return prepared != nullptr && !diagnostic.blocking;
  }
};

// Application owns admission sequencing; Runtime implements this portable port.
// Studio, file watchers and GPU backends may not publish a revision themselves.
class ProjectCandidateAdmissionPort {
 public:
  virtual ~ProjectCandidateAdmissionPort() = default;
  [[nodiscard]] virtual CandidatePreparationResult prepare(
      const core::ProjectSnapshot& candidate) = 0;
};

}  // namespace refusion::application
