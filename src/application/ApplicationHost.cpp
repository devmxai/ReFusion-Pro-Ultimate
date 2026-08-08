#include "refusion/application/ProjectCommandService.hpp"

#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace refusion::application {
namespace {

class ApplicationHost final : public ProjectCommandService {
 public:
  ApplicationHost(core::ProjectSnapshot initial_snapshot,
                  std::shared_ptr<core::TextLayoutPort> text_layout_port)
      : authority_(std::move(initial_snapshot), std::move(text_layout_port)) {}

  [[nodiscard]] core::ProjectSnapshot active_snapshot() const override {
    std::scoped_lock lock(admission_mutex_);
    return authority_.active_snapshot();
  }

  void set_candidate_admission_port(
      std::shared_ptr<ProjectCandidateAdmissionPort> port) override {
    std::scoped_lock lock(admission_mutex_);
    admission_port_ = std::move(port);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::RenameProjectCommand& command) override {
    return submit_with_admission(command);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::ReplaceProjectCommand& command) override {
    return submit_with_admission(command);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::SetVisualTransformCommand& command) override {
    return submit_with_admission(command);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::SetVisualPropertyCommand& command) override {
    return submit_with_admission(command);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::SetLayerEffectsCommand& command) override {
    return submit_with_admission(command);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::SetLayerMasksCommand& command) override {
    return submit_with_admission(command);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::AddVisualLayerCommand& command) override {
    return submit_with_admission(command);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::GroupNodesCommand& command) override {
    return submit_with_admission(command);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::ReparentNodesCommand& command) override {
    return submit_with_admission(command);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::AddEffectCommand& command) override {
    return submit_with_admission(command);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::AlignNodesCommand& command) override {
    return submit_with_admission(command);
  }

  [[nodiscard]] core::ApplyResult
  submit(const core::AnimateEffectPropertyCommand& command) override {
    return submit_with_admission(command);
  }

 private:
  template <typename Command>
  [[nodiscard]] core::ApplyResult submit_with_admission(
      const Command& command) {
    std::unique_lock lock(admission_mutex_);
    const auto preview = authority_.preview(command);
    if (!preview.accepted()) {
      return preview;
    }
    if (preview.replayed() || !admission_port_) {
      return authority_.apply(command);
    }

    CandidatePreparationResult preparation;
    try {
      preparation = admission_port_->prepare(preview.active_snapshot);
    } catch (const std::exception& error) {
      preparation.diagnostic = core::Diagnostic{
          .code = "RFX-RUNTIME-PREPARE-FAILED",
          .message = error.what(),
          .blocking = true,
      };
    } catch (...) {
      preparation.diagnostic = core::Diagnostic{
          .code = "RFX-RUNTIME-PREPARE-FAILED",
          .message = "candidate preparation failed with an unknown error",
          .blocking = true,
      };
    }
    if (!preparation.succeeded()) {
      auto diagnostic = std::move(preparation.diagnostic);
      if (diagnostic.code.empty()) {
        diagnostic.code = "RFX-RUNTIME-PREPARE-REJECTED";
      }
      if (diagnostic.message.empty()) {
        diagnostic.message = "Runtime rejected candidate preparation";
      }
      diagnostic.blocking = true;
      return core::ApplyResult{
          .status = core::ApplyStatus::rejected,
          .command_id = command.envelope.command_id,
          .active_snapshot = authority_.active_snapshot(),
          .diagnostic = std::move(diagnostic),
      };
    }

    auto result = authority_.apply(command);
    if (!result.accepted()) {
      return result;
    }
    preparation.prepared->commit_engine_state();

    // Core and Runtime are now one accepted bundle. Projection publication is
    // deliberately outside the admission lock: Qt model resets and bindings
    // synchronously read active_snapshot(), which must never self-deadlock.
    lock.unlock();
    preparation.prepared->publish_observer_projections();
    return result;
  }

  mutable std::mutex admission_mutex_;
  core::ProjectAuthority authority_;
  std::shared_ptr<ProjectCandidateAdmissionPort> admission_port_;
};

} // namespace

std::unique_ptr<ProjectCommandService>
create_application_host(
    core::ProjectSnapshot initial_snapshot,
    std::shared_ptr<core::TextLayoutPort> text_layout_port) {
  return std::make_unique<ApplicationHost>(std::move(initial_snapshot),
                                           std::move(text_layout_port));
}

} // namespace refusion::application
