#pragma once

#include "refusion/runtime/gpu/GpuDeviceService.hpp"

#include <memory>

namespace refusion::adapters::skia {

class SkiaGpuContexts final {
 public:
  ~SkiaGpuContexts();

  SkiaGpuContexts(const SkiaGpuContexts&) = delete;
  SkiaGpuContexts& operator=(const SkiaGpuContexts&) = delete;
  SkiaGpuContexts(SkiaGpuContexts&&) noexcept;
  SkiaGpuContexts& operator=(SkiaGpuContexts&&) noexcept;

  [[nodiscard]] static std::unique_ptr<SkiaGpuContexts> create(
      runtime::gpu::DeviceLease lease);

  [[nodiscard]] bool ganesh_ready() const noexcept;
  [[nodiscard]] bool graphite_ready() const noexcept;

 private:
  struct Implementation;

  explicit SkiaGpuContexts(std::unique_ptr<Implementation> implementation);

  std::unique_ptr<Implementation> implementation_;
};

}  // namespace refusion::adapters::skia
