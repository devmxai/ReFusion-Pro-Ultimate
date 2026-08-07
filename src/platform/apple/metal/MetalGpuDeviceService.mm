#include "refusion/platform/PlatformGpuDeviceService.hpp"

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>

#include <cstdint>
#include <memory>
#include <mutex>
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

    NSNotificationCenter* notifications =
        NSWorkspace.sharedWorkspace.notificationCenter;
    will_sleep_observer_ = [notifications
        addObserverForName:NSWorkspaceWillSleepNotification
                    object:nil
                     queue:nil
                usingBlock:^(NSNotification*) {
                  static_cast<void>(handle_lifecycle_event(
                      runtime::gpu::DeviceLifecycleEvent::will_sleep));
                }];
    did_wake_observer_ = [notifications
        addObserverForName:NSWorkspaceDidWakeNotification
                    object:nil
                     queue:nil
                usingBlock:^(NSNotification*) {
                  static_cast<void>(handle_lifecycle_event(
                      runtime::gpu::DeviceLifecycleEvent::did_wake));
                }];
  }

  ~MetalGpuDeviceService() override {
    NSNotificationCenter* notifications =
        NSWorkspace.sharedWorkspace.notificationCenter;
    if (will_sleep_observer_ != nil) {
      [notifications removeObserver:will_sleep_observer_];
    }
    if (did_wake_observer_ != nil) {
      [notifications removeObserver:did_wake_observer_];
    }
  }

  [[nodiscard]] runtime::gpu::DeviceIdentity identity() const noexcept override {
    std::scoped_lock lock(mutex_);
    return identity_;
  }

  [[nodiscard]] runtime::gpu::DeviceHealth health() const override {
    std::scoped_lock lock(mutex_);
    return health_locked();
  }

  [[nodiscard]] runtime::gpu::DeviceHealth handle_lifecycle_event(
      const runtime::gpu::DeviceLifecycleEvent event) override {
    std::scoped_lock lock(mutex_);
    ++event_sequence_;
    if (event == runtime::gpu::DeviceLifecycleEvent::will_sleep) {
      if (status_ != runtime::gpu::DeviceStatus::lost) {
        status_ = runtime::gpu::DeviceStatus::suspended;
        code_ = "RFX-GPU-SUSPENDED";
        diagnostic_ = "Metal presentation suspended before system sleep";
      }
      return health_locked();
    }

    if (status_ == runtime::gpu::DeviceStatus::lost) {
      return health_locked();
    }
    id<MTLDevice> current_device = MTLCreateSystemDefaultDevice();
    if (current_device == nil ||
        static_cast<std::uint64_t>(current_device.registryID) !=
            identity_.adapter_id) {
      mark_lost_locked("Metal device changed or disappeared after system wake");
      return health_locked();
    }
    status_ = runtime::gpu::DeviceStatus::ready;
    code_.clear();
    diagnostic_.clear();
    return health_locked();
  }

  [[nodiscard]] runtime::gpu::DeviceHealth report_device_loss(
      std::string diagnostic) override {
    std::scoped_lock lock(mutex_);
    ++event_sequence_;
    mark_lost_locked(std::move(diagnostic));
    return health_locked();
  }

  [[nodiscard]] runtime::gpu::DeviceLease borrow() override {
    std::scoped_lock lock(mutex_);
    if (status_ != runtime::gpu::DeviceStatus::ready) {
      throw std::runtime_error(code_.empty() ? "GPU device is not ready" : code_);
    }
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
  [[nodiscard]] runtime::gpu::DeviceHealth health_locked() const {
    return runtime::gpu::DeviceHealth{
        .identity = identity_,
        .status = status_,
        .event_sequence = event_sequence_,
        .code = code_,
        .diagnostic = diagnostic_,
    };
  }

  void mark_lost_locked(std::string diagnostic) {
    if (status_ != runtime::gpu::DeviceStatus::lost) {
      ++identity_.generation;
    }
    status_ = runtime::gpu::DeviceStatus::lost;
    code_ = "RFX-GPU-LOST";
    diagnostic_ = diagnostic.empty() ? "Metal device was lost" : std::move(diagnostic);
  }

  mutable std::mutex mutex_;
  runtime::gpu::DeviceIdentity identity_;
  std::shared_ptr<MetalState> state_;
  runtime::gpu::DeviceStatus status_{runtime::gpu::DeviceStatus::ready};
  std::uint64_t event_sequence_{0};
  std::string code_;
  std::string diagnostic_;
  __strong id will_sleep_observer_{nil};
  __strong id did_wake_observer_{nil};
};

}  // namespace

std::unique_ptr<runtime::gpu::GpuDeviceService>
create_platform_gpu_device_service() {
  return std::make_unique<MetalGpuDeviceService>();
}

}  // namespace refusion::platform
