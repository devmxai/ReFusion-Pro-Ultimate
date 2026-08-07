#pragma once

#include "refusion/runtime/gpu/GpuDeviceService.hpp"

#include <memory>

namespace refusion::platform {

[[nodiscard]] std::unique_ptr<runtime::gpu::GpuDeviceService>
create_platform_gpu_device_service();

}  // namespace refusion::platform
