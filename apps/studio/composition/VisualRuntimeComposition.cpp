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

namespace {

class VisualRuntimeComposition final : public StudioRuntimeComposition {
 public:
  VisualRuntimeComposition()
      : device_service_(refusion::platform::create_platform_gpu_device_service()),
        renderer_(refusion::adapters::skia::SkiaGpuContexts::create(
            device_service_->borrow())),
        presenter_(refusion::platform::create_platform_viewport_presenter(
            *device_service_, *renderer_)),
        render_session_(std::make_unique<
            refusion::runtime::presentation::ViewportRenderSession>(*presenter_)),
        viewport_window_(std::make_unique<EngineViewportWindow>(
            QString::fromStdString(device_service_->identity().adapter_name),
            *render_session_)) {}

  [[nodiscard]] QWindow* viewport_window() noexcept override {
    return viewport_window_.get();
  }

 private:
  std::unique_ptr<refusion::runtime::gpu::GpuDeviceService> device_service_;
  std::unique_ptr<refusion::runtime::presentation::ViewportFrameRenderer> renderer_;
  std::unique_ptr<refusion::runtime::presentation::ViewportPresenter> presenter_;
  std::unique_ptr<refusion::runtime::presentation::ViewportRenderSession>
      render_session_;
  std::unique_ptr<EngineViewportWindow> viewport_window_;
};

}  // namespace

std::unique_ptr<StudioRuntimeComposition> create_studio_runtime_composition() {
  return std::make_unique<VisualRuntimeComposition>();
}
