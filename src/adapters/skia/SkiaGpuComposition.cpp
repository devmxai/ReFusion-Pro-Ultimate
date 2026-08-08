#include "refusion/adapters/skia/SkiaGpuComposition.hpp"

#include "SkiaTextLayoutInternal.hpp"
#include "refusion/adapters/skia/SkiaGpuContexts.hpp"

#include <memory>
#include <utility>

namespace refusion::adapters::skia {

std::unique_ptr<SkiaGpuContexts> create_skia_gpu_composition(
    runtime::gpu::BackendDeviceLease lease,
    std::shared_ptr<const runtime::media::DecodedSurfaceQueue>
        decoded_video_queue,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability,
    std::shared_ptr<core::FontAssetResolverPort> font_assets) {
  auto text_layout =
      std::make_unique<SkiaTextLayoutEngine>(std::move(font_assets));
  return SkiaGpuContexts::create(
      std::move(lease), std::move(decoded_video_queue), std::move(observability),
      std::move(text_layout));
}

}  // namespace refusion::adapters::skia
