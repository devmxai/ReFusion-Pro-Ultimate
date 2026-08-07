#pragma once

#include "refusion/runtime/gpu/GpuDeviceService.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <memory>

namespace refusion::platform {

[[nodiscard]] runtime::presentation::NativeWindowSystem
platform_native_window_system() noexcept;

[[nodiscard]] std::unique_ptr<runtime::presentation::ViewportPresenter>
create_platform_viewport_presenter(
    runtime::gpu::GpuDeviceService& device_service,
    runtime::presentation::ViewportFrameRenderer& frame_renderer);

}  // namespace refusion::platform
