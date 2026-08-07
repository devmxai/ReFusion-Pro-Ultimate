#pragma once

#include "refusion/core/ProjectDocument.hpp"
#include "refusion/runtime/gpu/GpuDeviceService.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <memory>

namespace refusion::adapters::skia {

class SkiaGpuContexts final : public runtime::presentation::ViewportFrameRenderer {
 public:
  ~SkiaGpuContexts();

  SkiaGpuContexts(const SkiaGpuContexts&) = delete;
  SkiaGpuContexts& operator=(const SkiaGpuContexts&) = delete;
  SkiaGpuContexts(SkiaGpuContexts&&) = delete;
  SkiaGpuContexts& operator=(SkiaGpuContexts&&) = delete;

  [[nodiscard]] static std::unique_ptr<SkiaGpuContexts> create(
      runtime::gpu::DeviceLease lease,
      core::CompositionSnapshot composition);

  [[nodiscard]] bool ganesh_ready() const noexcept;
  [[nodiscard]] bool graphite_ready() const noexcept;
  [[nodiscard]] const runtime::gpu::DeviceIdentity& device_identity() const noexcept override;
  [[nodiscard]] runtime::presentation::FrameResult render(
      const runtime::presentation::NativeFrameTarget& target,
      const runtime::presentation::FixtureFrame& frame) override;

 private:
  struct Implementation;

  explicit SkiaGpuContexts(std::unique_ptr<Implementation> implementation);

  std::unique_ptr<Implementation> implementation_;
};

}  // namespace refusion::adapters::skia
