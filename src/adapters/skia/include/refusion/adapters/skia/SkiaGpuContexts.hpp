#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "refusion/runtime/gpu/GpuDeviceService.hpp"
#include "refusion/runtime/gpu/GpuObservability.hpp"
#include "refusion/runtime/media/HardwareVideoDecode.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"
#include "refusion/runtime/render/VisualRenderPlan.hpp"

namespace refusion::adapters::skia {

class SkiaTextLayoutEngine;

class SkiaGpuContexts final
    : public runtime::presentation::ViewportFrameRenderer {
 public:
  ~SkiaGpuContexts();

  SkiaGpuContexts(const SkiaGpuContexts&) = delete;
  SkiaGpuContexts& operator=(const SkiaGpuContexts&) = delete;
  SkiaGpuContexts(SkiaGpuContexts&&) = delete;
  SkiaGpuContexts& operator=(SkiaGpuContexts&&) = delete;

  [[nodiscard]] static std::unique_ptr<SkiaGpuContexts> create(
      runtime::gpu::BackendDeviceLease lease,
      std::shared_ptr<const runtime::media::DecodedSurfaceQueue>
          decoded_video_queue = nullptr,
      std::shared_ptr<runtime::gpu::GpuObservabilityService> observability =
          nullptr);

  [[nodiscard]] static std::unique_ptr<SkiaGpuContexts> create(
      runtime::gpu::BackendDeviceLease lease,
      std::shared_ptr<const runtime::media::DecodedSurfaceQueue>
          decoded_video_queue,
      std::shared_ptr<runtime::gpu::GpuObservabilityService> observability,
      std::unique_ptr<SkiaTextLayoutEngine> text_layout_engine);

  [[nodiscard]] bool ganesh_ready() const noexcept;
  [[nodiscard]] bool graphite_ready() const noexcept;
  [[nodiscard]] std::string text_layout_engine_digest() const;
  [[nodiscard]] std::optional<std::uint64_t> selected_video_source_frame_index()
      const noexcept;
  [[nodiscard]] const runtime::gpu::DeviceIdentity& device_identity()
      const noexcept override;
  [[nodiscard]] runtime::presentation::FrameResult render(
      const runtime::presentation::BackendFrameTargetLease& target,
      const runtime::presentation::PresentationFrameRequest& frame) override;
  [[nodiscard]] runtime::presentation::FrameResult retire_frame_targets()
      override;

 private:
  struct Implementation;

  explicit SkiaGpuContexts(std::unique_ptr<Implementation> implementation);

  std::unique_ptr<Implementation> implementation_;
};

}  // namespace refusion::adapters::skia
