#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "refusion/runtime/media/HardwareVideoDecode.hpp"

namespace refusion::platform::apple {

// Apple-only ephemeral view used by native GPU adapters. The retained opaque
// surface keeps CoreVideo and both Metal texture views alive. These handles
// never cross into Runtime/Core/project contracts or serialized state.
struct MetalVideoSurfaceView final {
  std::shared_ptr<const runtime::media::NativeVideoSurfaceLease> surface;
  std::uintptr_t luma_texture{0};
  std::uintptr_t chroma_texture{0};
  std::uint32_t luma_width{0};
  std::uint32_t luma_height{0};
  std::uint32_t chroma_width{0};
  std::uint32_t chroma_height{0};

  [[nodiscard]] bool valid() const noexcept;
};

[[nodiscard]] std::optional<MetalVideoSurfaceView> borrow_metal_video_surface(
    std::shared_ptr<const runtime::media::NativeVideoSurfaceLease> surface);

}  // namespace refusion::platform::apple
