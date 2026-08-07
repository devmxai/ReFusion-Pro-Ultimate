#include "StudioRuntimeComposition.hpp"

#include "EngineViewportWindow.hpp"

#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformViewportPresenter.hpp"
#include "refusion/runtime/gpu/GpuDeviceService.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <QWindow>
#include <QString>

#include <memory>
#include <stdexcept>
#include <utility>

namespace {

class VisualRuntimeComposition final : public StudioRuntimeComposition {
 public:
  VisualRuntimeComposition(const refusion::core::ProjectSnapshot& project,
                           QString project_path)
      : device_service_(refusion::platform::create_platform_gpu_device_service()),
        renderer_(refusion::adapters::skia::SkiaGpuContexts::create(
            device_service_->borrow(), require_composition(project))),
        presenter_(refusion::platform::create_platform_viewport_presenter(
            *device_service_, *renderer_)),
        render_session_(std::make_unique<
            refusion::runtime::presentation::ViewportRenderSession>(
                *presenter_, playback_spec(project))),
        viewport_window_(std::make_unique<EngineViewportWindow>(
            QString::fromStdString(device_service_->identity().adapter_name),
            std::move(project_path),
            require_composition(project),
            *render_session_)) {}

  [[nodiscard]] QWindow* viewport_window() noexcept override {
    return viewport_window_.get();
  }

 private:
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

  std::unique_ptr<refusion::runtime::gpu::GpuDeviceService> device_service_;
  std::unique_ptr<refusion::runtime::presentation::ViewportFrameRenderer> renderer_;
  std::unique_ptr<refusion::runtime::presentation::ViewportPresenter> presenter_;
  std::unique_ptr<refusion::runtime::presentation::ViewportRenderSession>
      render_session_;
  std::unique_ptr<EngineViewportWindow> viewport_window_;
};

}  // namespace

std::unique_ptr<StudioRuntimeComposition> create_studio_runtime_composition(
    const refusion::core::ProjectSnapshot& project,
    const QString& project_path) {
  return std::make_unique<VisualRuntimeComposition>(project, project_path);
}
