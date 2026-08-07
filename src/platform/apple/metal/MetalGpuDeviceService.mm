#include "refusion/platform/PlatformGpuDeviceService.hpp"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace refusion::platform {
namespace {

struct MetalState final {
  id<MTLDevice> device;
  id<MTLCommandQueue> command_queue;
};

class MetalGpuDeviceService final : public runtime::gpu::GpuDeviceService {
 public:
  MetalGpuDeviceService() {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) {
      throw std::runtime_error("Metal returned no default GPU device");
    }

    id<MTLCommandQueue> command_queue = [device newCommandQueue];
    if (command_queue == nil) {
      throw std::runtime_error("Metal failed to create the engine command queue");
    }

    state_ = std::make_shared<MetalState>(MetalState{
        .device = device,
        .command_queue = command_queue,
    });

    const char* utf8_name = [[device name] UTF8String];
    identity_ = runtime::gpu::DeviceIdentity{
        .backend = runtime::gpu::Backend::metal,
        .adapter_name = utf8_name == nullptr ? "Metal device" : utf8_name,
        .adapter_id = static_cast<std::uint64_t>([device registryID]),
        .generation = 1,
    };
  }

  [[nodiscard]] const runtime::gpu::DeviceIdentity& identity() const noexcept override {
    return identity_;
  }

  [[nodiscard]] runtime::gpu::DeviceLease borrow() override {
    return runtime::gpu::DeviceLease(
        identity_,
        runtime::gpu::NativeHandles{
            .device = reinterpret_cast<std::uintptr_t>((__bridge void*)state_->device),
            .command_queue = reinterpret_cast<std::uintptr_t>(
                (__bridge void*)state_->command_queue),
        },
        state_);
  }

 private:
  runtime::gpu::DeviceIdentity identity_;
  std::shared_ptr<MetalState> state_;
};

}  // namespace

std::unique_ptr<runtime::gpu::GpuDeviceService>
create_platform_gpu_device_service() {
  return std::make_unique<MetalGpuDeviceService>();
}

}  // namespace refusion::platform
