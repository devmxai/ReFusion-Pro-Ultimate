#pragma once

#include "refusion/core/FontAssetResolver.hpp"
#include "refusion/runtime/gpu/GpuDeviceService.hpp"
#include "refusion/runtime/gpu/GpuObservability.hpp"
#include "refusion/runtime/media/HardwareVideoDecode.hpp"
#include "refusion/runtime/render/VisualRenderPlan.hpp"

#include <memory>

namespace refusion::adapters::skia {

class SkiaGpuContexts;

// Common composition root: resolves project assets and constructs the shared
// text engine before handing an already prepared executor to the thin native
// context. Native Metal/D3D/Vulkan files never see project asset semantics.
[[nodiscard]] std::unique_ptr<SkiaGpuContexts>
create_skia_gpu_composition(
    runtime::gpu::BackendDeviceLease lease,
    std::shared_ptr<const runtime::media::DecodedSurfaceQueue>
        decoded_video_queue = nullptr,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability =
        nullptr,
    std::shared_ptr<core::FontAssetResolverPort> font_assets = nullptr);

}  // namespace refusion::adapters::skia
