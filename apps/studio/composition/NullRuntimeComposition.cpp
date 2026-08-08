#include "StudioRuntimeComposition.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

namespace {

class NullRuntimeComposition final : public StudioRuntimeComposition {
 public:
  explicit NullRuntimeComposition(refusion::core::ProjectSnapshot project)
      : project_(std::make_shared<const refusion::core::ProjectSnapshot>(
            std::move(project))) {}

  [[nodiscard]] QWindow* viewport_window() noexcept override { return nullptr; }
  [[nodiscard]] StudioTransportBridge* transport_bridge() noexcept override {
    return nullptr;
  }

  [[nodiscard]] refusion::application::CandidatePreparationResult prepare(
      const refusion::core::ProjectSnapshot& project) override {
    if (!project.composition || !project_->composition) {
      return {.diagnostic = {
                  .code = "RFX-RUNTIME-RELOAD-001",
                  .message = "composition is required",
                  .blocking = true,
              }};
    }
    const auto& current = *project_->composition;
    const auto& candidate = *project.composition;
    if (candidate.composition_id != current.composition_id ||
        candidate.canvas != current.canvas ||
        candidate.frame_rate != current.frame_rate ||
        candidate.duration != current.duration) {
      return {.diagnostic = {
                  .code = "RFX-RUNTIME-RELOAD-002",
                  .message = "live edit must preserve composition ID, canvas, frame rate and duration",
                  .blocking = true,
              }};
    }
    class Prepared final
        : public refusion::application::PreparedProjectRevision {
     public:
      Prepared(NullRuntimeComposition& owner,
               std::shared_ptr<const refusion::core::ProjectSnapshot> project)
          : owner_(owner), project_(std::move(project)) {}

      void commit_engine_state() noexcept override {
        owner_.project_ = std::move(project_);
      }

     private:
      NullRuntimeComposition& owner_;
      std::shared_ptr<const refusion::core::ProjectSnapshot> project_;
    };
    return {.prepared = std::make_unique<Prepared>(
                *this,
                std::make_shared<const refusion::core::ProjectSnapshot>(
                    project))};
  }

 private:
  std::shared_ptr<const refusion::core::ProjectSnapshot> project_;
};

}  // namespace

std::shared_ptr<StudioRuntimeComposition> create_studio_runtime_composition(
    const refusion::core::ProjectSnapshot& project,
    const QString&,
    std::shared_ptr<refusion::core::FontAssetResolverPort>) {
  return std::make_shared<NullRuntimeComposition>(project);
}
