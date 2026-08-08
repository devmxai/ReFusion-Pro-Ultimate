#pragma once

#include "refusion/runtime/gpu/GpuDeviceService.hpp"
#include "refusion/runtime/gpu/GpuObservability.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <memory>

namespace refusion::platform {

[[nodiscard]] runtime::presentation::NativeWindowSystem
platform_native_window_system() noexcept;

// Acquires and validates the native host at the platform boundary. The input
// exists only at this call site; the returned Runtime lease owns its lifetime.
[[nodiscard]] runtime::presentation::NativeViewportHostLease
acquire_platform_viewport_host(std::uintptr_t native_handle);

[[nodiscard]] std::unique_ptr<runtime::presentation::ViewportPresenter>
create_platform_viewport_presenter(
    runtime::gpu::GpuDeviceService& device_service,
    runtime::presentation::ViewportFrameRenderer& frame_renderer,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability =
        nullptr);

}  // namespace refusion::platform
