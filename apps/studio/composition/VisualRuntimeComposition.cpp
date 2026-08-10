#include "StudioRuntimeComposition.hpp"

#include "EngineViewportWindow.hpp"
#include "StudioTransportBridge.hpp"
#include "composition/StudioVideoPlaybackController.hpp"

#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/adapters/skia/SkiaGpuComposition.hpp"
#include "refusion/adapters/skia/SkiaTextLayout.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformViewportPresenter.hpp"
#include "refusion/runtime/gpu/GpuDeviceService.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"
#include "refusion/runtime/render/RenderPlanCompiler.hpp"

#include <QWindow>
#include <QString>

#include <memory>
#include <stdexcept>
#include <utility>

namespace {

class VisualRuntimeComposition final : public StudioRuntimeComposition {
 public:
  VisualRuntimeComposition(const refusion::core::ProjectSnapshot& project,
                           QString project_path,
                           std::shared_ptr<
                               refusion::core::FontAssetResolverPort>
                               font_assets)
      : composition_(std::make_shared<const refusion::core::CompositionSnapshot>(
            require_composition(project))),
        render_program_(std::make_shared<const
                        refusion::runtime::render::VisualRenderProgram>(
            refusion::runtime::render::compile_visual_render_program(project))),
        preflight_text_layout_(
            refusion::adapters::skia::create_skia_text_layout_port(
                font_assets)),
        device_service_(refusion::platform::create_platform_gpu_device_service()),
        renderer_(refusion::adapters::skia::create_skia_gpu_composition(
            device_service_->borrow(), nullptr, nullptr,
            std::move(font_assets))),
        presenter_(refusion::platform::create_platform_viewport_presenter(
            *device_service_, *renderer_)),
        render_session_(std::make_unique<
            refusion::runtime::presentation::ViewportRenderSession>(
                *presenter_, playback_spec(project),
                refusion::runtime::presentation::ViewportRenderSession::ClockNow{},
                render_program_)),
        video_playback_(std::make_unique<StudioVideoPlaybackController>(
            project_path, project, *device_service_, *renderer_,
            *render_session_)),
        viewport_window_(std::make_unique<EngineViewportWindow>(
            QString::fromStdString(device_service_->identity().adapter_name),
            std::move(project_path),
            composition_,
            *render_session_)),
        transport_bridge_(std::make_unique<StudioTransportBridge>(
            *render_session_, composition_)) {
    QObject::connect(viewport_window_.get(),
                     &EngineViewportWindow::telemetryChanged,
                     transport_bridge_.get(),
                     &StudioTransportBridge::refresh);
  }

  [[nodiscard]] QWindow* viewport_window() noexcept override {
    return viewport_window_.get();
  }

  [[nodiscard]] StudioTransportBridge* transport_bridge() noexcept override {
    return transport_bridge_.get();
  }

  [[nodiscard]] refusion::application::CandidatePreparationResult prepare(
      const refusion::core::ProjectSnapshot& project) override {
    if (!project.composition) {
      return rejection("RFX-RUNTIME-RELOAD-001", "composition is required");
    }
    const auto& candidate = *project.composition;
    if (candidate.composition_id != composition_->composition_id ||
        candidate.canvas != composition_->canvas ||
        candidate.frame_rate != composition_->frame_rate ||
        candidate.duration != composition_->duration) {
      return rejection(
          "RFX-RUNTIME-RELOAD-002",
          "live edit must preserve composition ID, canvas, frame rate and duration");
    }
    auto accepted_project =
        std::make_shared<const refusion::core::ProjectSnapshot>(project);
    auto program = std::make_shared<const
        refusion::runtime::render::VisualRenderProgram>(
        refusion::runtime::render::compile_visual_render_program(
            *accepted_project));
    const auto composition_time = transport_bridge_->compositionTimeNs();
    const auto epoch = render_session_->playback_state().clock_epoch_id;
    static_cast<void>(refusion::runtime::render::evaluate_visual_render_plan(
        *program, composition_time, epoch, *preflight_text_layout_));
    auto composition =
        std::make_shared<const refusion::core::CompositionSnapshot>(candidate);
    auto timeline = transport_bridge_->prepareComposition(composition);
    return {.prepared = std::make_unique<Prepared>(
                *this, std::move(accepted_project), std::move(program),
                std::move(composition),
                std::move(timeline))};
  }

 private:
  class Prepared final
      : public refusion::application::PreparedProjectRevision {
   public:
    Prepared(VisualRuntimeComposition& owner,
             std::shared_ptr<const refusion::core::ProjectSnapshot> project,
             std::shared_ptr<const
                 refusion::runtime::render::VisualRenderProgram> program,
             std::shared_ptr<const refusion::core::CompositionSnapshot>
                 composition,
             StudioTransportBridge::PreparedCompositionProjection timeline)
        : owner_(owner),
          project_(std::move(project)),
          program_(std::move(program)),
          composition_(std::move(composition)),
          timeline_(std::move(timeline)) {}

    void commit_engine_state() noexcept override {
      owner_.render_session_->publish_render_program(program_);
      owner_.render_program_ = program_;
      owner_.composition_ = composition_;
      owner_.video_playback_->publishProject(*project_);
    }

    void publish_observer_projections() noexcept override {
      owner_.viewport_window_->publishComposition(composition_);
      owner_.transport_bridge_->publishComposition(std::move(timeline_));
    }

   private:
    VisualRuntimeComposition& owner_;
    std::shared_ptr<const refusion::core::ProjectSnapshot> project_;
    std::shared_ptr<const refusion::runtime::render::VisualRenderProgram>
        program_;
    std::shared_ptr<const refusion::core::CompositionSnapshot> composition_;
    StudioTransportBridge::PreparedCompositionProjection timeline_;
  };

  [[nodiscard]] static refusion::application::CandidatePreparationResult
  rejection(std::string code, std::string message) {
    return {.diagnostic = {.code = std::move(code),
                           .message = std::move(message),
                           .blocking = true}};
  }

  [[nodiscard]] static refusion::core::CompositionSnapshot require_composition(
      const refusion::core::ProjectSnapshot& project) {
    if (!project.composition) {
      throw std::invalid_argument("RFX-PROJECT-115: project has no composition");
    }
    return *project.composition;
  }

  [[nodiscard]] static refusion::runtime::presentation::PlaybackSpec playback_spec(
      const refusion::core::ProjectSnapshot& project) {
    const auto composition = require_composition(project);
    return refusion::runtime::presentation::PlaybackSpec{
        .duration_ns = composition.duration,
        .frame_rate_numerator = composition.frame_rate.numerator,
        .frame_rate_denominator = composition.frame_rate.denominator,
        .loop = true,
    };
  }

  std::shared_ptr<const refusion::core::CompositionSnapshot> composition_;
  std::shared_ptr<const refusion::runtime::render::VisualRenderProgram>
      render_program_;
  std::shared_ptr<refusion::core::TextLayoutPort> preflight_text_layout_;
  std::unique_ptr<refusion::runtime::gpu::GpuDeviceService> device_service_;
  std::unique_ptr<refusion::adapters::skia::SkiaGpuContexts> renderer_;
  std::unique_ptr<refusion::runtime::presentation::ViewportPresenter> presenter_;
  std::unique_ptr<refusion::runtime::presentation::ViewportRenderSession>
      render_session_;
  std::unique_ptr<StudioVideoPlaybackController> video_playback_;
  std::unique_ptr<EngineViewportWindow> viewport_window_;
  std::unique_ptr<StudioTransportBridge> transport_bridge_;
};

}  // namespace

std::shared_ptr<StudioRuntimeComposition> create_studio_runtime_composition(
    const refusion::core::ProjectSnapshot& project,
    const QString& project_path,
    std::shared_ptr<refusion::core::FontAssetResolverPort> font_assets) {
  return std::make_shared<VisualRuntimeComposition>(
      project, project_path, std::move(font_assets));
}
