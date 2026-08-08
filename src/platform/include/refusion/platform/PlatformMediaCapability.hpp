#pragma once

#include <memory>

#include "refusion/runtime/gpu/GpuDeviceService.hpp"
#include "refusion/runtime/gpu/GpuObservability.hpp"
#include "refusion/runtime/media/HardwareVideoDecode.hpp"
#include "refusion/runtime/media/MediaCapability.hpp"

namespace refusion::platform {

[[nodiscard]] std::unique_ptr<runtime::media::MediaCapabilityProbe>
create_platform_media_capability_probe(
    runtime::gpu::GpuDeviceService& gpu_device_service);

[[nodiscard]] std::unique_ptr<runtime::media::HardwareVideoDecoder>
create_platform_hardware_video_decoder(
    runtime::gpu::GpuDeviceService& gpu_device_service,
    std::shared_ptr<runtime::gpu::GpuObservabilityService> observability =
        nullptr);

}  // namespace refusion::platform
