#pragma once

#include "refusion/runtime/gpu/GpuDeviceService.hpp"
#include "refusion/runtime/media/MediaCapability.hpp"

#include <memory>

namespace refusion::platform {

[[nodiscard]] std::unique_ptr<runtime::media::MediaCapabilityProbe>
create_platform_media_capability_probe(
    runtime::gpu::GpuDeviceService& gpu_device_service);

}  // namespace refusion::platform
