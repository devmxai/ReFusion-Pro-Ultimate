#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformViewportPresenter.hpp"

#import <Metal/Metal.h>
#import <UIKit/UIKit.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace refusion::platform {
namespace {

using runtime::presentation::FrameResult;
using runtime::presentation::FrameStatus;
using runtime::presentation::NativeViewportHostLease;
using runtime::presentation::NativeWindowSystem;
using runtime::presentation::PresentationFrameRequest;
using runtime::presentation::PresentationTelemetry;
using runtime::presentation::ViewportExtent;
using runtime::presentation::ViewportFrameRenderer;
using runtime::presentation::ViewportPresenter;

struct IosMetalDeviceState final {
  id<MTLDevice> device;
  id<MTLCommandQueue> command_queue;
};

class IosMetalDeviceService final
    : public runtime::gpu::GpuDeviceService {
 public:
  IosMetalDeviceService() {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) {
      throw std::runtime_error("iOS Metal returned no default device");
    }
    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (queue == nil) {
      throw std::runtime_error("iOS Metal failed to create the engine queue");
    }
    state_ = std::make_shared<IosMetalDeviceState>(IosMetalDeviceState{
        .device = device,
        .command_queue = queue,
    });
    const char* name = device.name.UTF8String;
    identity_ = runtime::gpu::DeviceIdentity{
        .backend = runtime::gpu::Backend::metal,
        .adapter_name = name == nullptr ? "iOS Metal device" : name,
        .adapter_id = static_cast<std::uint64_t>(device.registryID),
        .generation = 1,
    };
  }

  [[nodiscard]] runtime::gpu::DeviceIdentity identity()
      const noexcept override {
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
      status_ = runtime::gpu::DeviceStatus::suspended;
      code_ = "RFX-IOS-GPU-SUSPENDED";
      diagnostic_ = "iOS Metal contract canary was suspended";
    } else if (status_ != runtime::gpu::DeviceStatus::lost) {
      status_ = runtime::gpu::DeviceStatus::ready;
      code_.clear();
      diagnostic_.clear();
    }
    return health_locked();
  }

  [[nodiscard]] runtime::gpu::DeviceHealth report_device_loss(
      std::string diagnostic) override {
    std::scoped_lock lock(mutex_);
    ++event_sequence_;
    ++identity_.generation;
    status_ = runtime::gpu::DeviceStatus::lost;
    code_ = "RFX-IOS-GPU-LOST";
    diagnostic_ = diagnostic.empty() ? "iOS Metal device was lost"
                                     : std::move(diagnostic);
    return health_locked();
  }

  [[nodiscard]] runtime::gpu::BackendDeviceLease borrow() override {
    std::scoped_lock lock(mutex_);
    if (status_ != runtime::gpu::DeviceStatus::ready) {
      throw std::runtime_error(code_.empty() ? "iOS Metal device is not ready"
                                             : code_);
    }
    return runtime::gpu::BackendDeviceLease(
        identity_,
        std::shared_ptr<const void>(
            state_, (__bridge const void*)state_->device),
        std::shared_ptr<const void>(
            state_, (__bridge const void*)state_->command_queue));
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

  mutable std::mutex mutex_;
  runtime::gpu::DeviceIdentity identity_;
  std::shared_ptr<IosMetalDeviceState> state_;
  runtime::gpu::DeviceStatus status_{runtime::gpu::DeviceStatus::ready};
  std::uint64_t event_sequence_{0};
  std::string code_;
  std::string diagnostic_;
};

class IosMetalContractCanaryPresenter final : public ViewportPresenter {
 public:
  IosMetalContractCanaryPresenter(
      runtime::gpu::GpuDeviceService& device_service,
      ViewportFrameRenderer& renderer)
      : device_service_(device_service), renderer_(renderer) {
    if (device_service_.identity().backend != runtime::gpu::Backend::metal ||
        renderer_.device_identity() != device_service_.identity()) {
      throw std::invalid_argument(
          "iOS presenter canary requires one shared Metal device identity");
    }
    telemetry_.device_generation = device_service_.identity().generation;
    telemetry_.device_status = runtime::gpu::DeviceStatus::ready;
  }

  [[nodiscard]] runtime::gpu::DeviceIdentity device_identity()
      const noexcept override {
    return device_service_.identity();
  }

  [[nodiscard]] FrameResult attach(NativeViewportHostLease host) override {
    if (!host.valid() || host.window_system != NativeWindowSystem::ui_view) {
      ++telemetry_.rejected_frames;
      return rejected("RFX-IOS-CANARY-HOST: expected a UIView host lease");
    }
    host_ = std::move(host);
    return FrameResult{.status = FrameStatus::accepted};
  }

  void detach() noexcept override {
    host_ = {};
    visible_ = false;
  }

  [[nodiscard]] FrameResult resize(const ViewportExtent extent) override {
    if (!extent.valid()) {
      ++telemetry_.rejected_frames;
      return rejected("RFX-IOS-CANARY-EXTENT: invalid viewport extent");
    }
    extent_ = extent;
    return FrameResult{.status = FrameStatus::accepted};
  }

  void set_visible(const bool visible) noexcept override {
    visible_ = visible;
  }

  [[nodiscard]] FrameResult present(
      const PresentationFrameRequest&) override {
    ++telemetry_.frame_requests;
    ++telemetry_.rejected_frames;
    return rejected(
        "RFX-IOS-CANARY-NOT-PRODUCT: compile contract only; iOS runtime "
        "presentation remains gated to G9");
  }

  [[nodiscard]] PresentationTelemetry telemetry() const noexcept override {
    return telemetry_;
  }

 private:
  [[nodiscard]] static FrameResult rejected(std::string diagnostic) {
    return FrameResult{
        .status = FrameStatus::rejected,
        .diagnostic = std::move(diagnostic),
    };
  }

  runtime::gpu::GpuDeviceService& device_service_;
  ViewportFrameRenderer& renderer_;
  NativeViewportHostLease host_;
  ViewportExtent extent_;
  PresentationTelemetry telemetry_;
  bool visible_{false};
};

}  // namespace

std::unique_ptr<runtime::gpu::GpuDeviceService>
create_platform_gpu_device_service() {
  return std::make_unique<IosMetalDeviceService>();
}

runtime::presentation::NativeWindowSystem
platform_native_window_system() noexcept {
  return runtime::presentation::NativeWindowSystem::ui_view;
}

runtime::presentation::NativeViewportHostLease
acquire_platform_viewport_host(const std::uintptr_t native_handle) {
  UIView* view = (__bridge UIView*)(reinterpret_cast<void*>(native_handle));
  if (view == nil || ![view isKindOfClass:[UIView class]]) {
    throw std::invalid_argument(
        "RFX-IOS-CANARY-HOST: native host is not a UIView");
  }
  static std::atomic_uint64_t next_host_id{1};
  return runtime::presentation::NativeViewportHostLease{
      .window_system = runtime::presentation::NativeWindowSystem::ui_view,
      .host_id = next_host_id.fetch_add(1),
      .backend_private_state = std::shared_ptr<const void>(
          CFBridgingRetain(view),
          [](const void* value) { CFRelease(value); }),
  };
}

std::unique_ptr<runtime::presentation::ViewportPresenter>
create_platform_viewport_presenter(
    runtime::gpu::GpuDeviceService& device_service,
    runtime::presentation::ViewportFrameRenderer& renderer,
    std::shared_ptr<runtime::gpu::GpuObservabilityService>) {
  return std::make_unique<IosMetalContractCanaryPresenter>(device_service,
                                                           renderer);
}

}  // namespace refusion::platform
