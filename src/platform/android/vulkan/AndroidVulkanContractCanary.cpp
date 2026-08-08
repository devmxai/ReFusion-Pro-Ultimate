#include "refusion/platform/PlatformGpuDeviceService.hpp"
#include "refusion/platform/PlatformViewportPresenter.hpp"

#include <android/native_window.h>
#include <vulkan/vulkan.h>

#include <atomic>
#include <cstdint>
#include <memory>
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

class AndroidVulkanContractCanaryDevice final
    : public runtime::gpu::GpuDeviceService {
 public:
  [[nodiscard]] runtime::gpu::DeviceIdentity identity()
      const noexcept override {
    return identity_;
  }

  [[nodiscard]] runtime::gpu::DeviceHealth health() const override {
    return runtime::gpu::DeviceHealth{
        .identity = identity_,
        .status = runtime::gpu::DeviceStatus::lost,
        .event_sequence = event_sequence_,
        .code = "RFX-ANDROID-CANARY-NOT-PRODUCT",
        .diagnostic =
            "Android Vulkan contract compiles in G1; runtime remains gated to G9",
    };
  }

  [[nodiscard]] runtime::gpu::DeviceHealth handle_lifecycle_event(
      runtime::gpu::DeviceLifecycleEvent) override {
    ++event_sequence_;
    return health();
  }

  [[nodiscard]] runtime::gpu::DeviceHealth report_device_loss(
      std::string) override {
    ++event_sequence_;
    ++identity_.generation;
    return health();
  }

  [[nodiscard]] runtime::gpu::BackendDeviceLease borrow() override {
    throw std::runtime_error(
        "RFX-ANDROID-CANARY-NOT-PRODUCT: Vulkan runtime is not admitted in G1");
  }

 private:
  // These native Vulkan types deliberately remain private to the platform
  // canary. No native handle enters Runtime or project state.
  [[maybe_unused]] VkInstance instance_{VK_NULL_HANDLE};
  [[maybe_unused]] VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
  [[maybe_unused]] VkDevice device_{VK_NULL_HANDLE};
  [[maybe_unused]] VkQueue queue_{VK_NULL_HANDLE};
  runtime::gpu::DeviceIdentity identity_{
      .backend = runtime::gpu::Backend::vulkan,
      .adapter_name = "Android Vulkan contract canary",
      .adapter_id = 1,
      .generation = 1,
  };
  std::uint64_t event_sequence_{0};
};

class AndroidVulkanContractCanaryPresenter final : public ViewportPresenter {
 public:
  AndroidVulkanContractCanaryPresenter(
      runtime::gpu::GpuDeviceService& device_service,
      ViewportFrameRenderer& renderer)
      : device_service_(device_service), renderer_(renderer) {
    telemetry_.device_generation = device_service_.identity().generation;
    telemetry_.device_status = runtime::gpu::DeviceStatus::lost;
  }

  [[nodiscard]] runtime::gpu::DeviceIdentity device_identity()
      const noexcept override {
    return device_service_.identity();
  }

  [[nodiscard]] FrameResult attach(NativeViewportHostLease host) override {
    if (!host.valid() ||
        host.window_system != NativeWindowSystem::android_native_window) {
      return reject("RFX-ANDROID-CANARY-HOST: expected ANativeWindow lease");
    }
    host_ = std::move(host);
    return FrameResult{.status = FrameStatus::accepted};
  }

  void detach() noexcept override { host_ = {}; }

  [[nodiscard]] FrameResult resize(const ViewportExtent extent) override {
    if (!extent.valid()) {
      return reject("RFX-ANDROID-CANARY-EXTENT: invalid viewport extent");
    }
    extent_ = extent;
    return FrameResult{.status = FrameStatus::accepted};
  }

  void set_visible(bool) noexcept override {}

  [[nodiscard]] FrameResult present(
      const PresentationFrameRequest&) override {
    ++telemetry_.frame_requests;
    return reject(
        "RFX-ANDROID-CANARY-NOT-PRODUCT: presentation remains gated to G9");
  }

  [[nodiscard]] PresentationTelemetry telemetry() const noexcept override {
    return telemetry_;
  }

 private:
  [[nodiscard]] FrameResult reject(std::string diagnostic) {
    ++telemetry_.rejected_frames;
    return FrameResult{
        .status = FrameStatus::rejected,
        .diagnostic = std::move(diagnostic),
    };
  }

  runtime::gpu::GpuDeviceService& device_service_;
  [[maybe_unused]] ViewportFrameRenderer& renderer_;
  NativeViewportHostLease host_;
  ViewportExtent extent_;
  PresentationTelemetry telemetry_;
};

}  // namespace

std::unique_ptr<runtime::gpu::GpuDeviceService>
create_platform_gpu_device_service() {
  return std::make_unique<AndroidVulkanContractCanaryDevice>();
}

runtime::presentation::NativeWindowSystem
platform_native_window_system() noexcept {
  return runtime::presentation::NativeWindowSystem::android_native_window;
}

runtime::presentation::NativeViewportHostLease
acquire_platform_viewport_host(const std::uintptr_t native_handle) {
  auto* window = reinterpret_cast<ANativeWindow*>(native_handle);
  if (window == nullptr) {
    throw std::invalid_argument(
        "RFX-ANDROID-CANARY-HOST: ANativeWindow is null");
  }
  ANativeWindow_acquire(window);
  static std::atomic_uint64_t next_host_id{1};
  return runtime::presentation::NativeViewportHostLease{
      .window_system = NativeWindowSystem::android_native_window,
      .host_id = next_host_id.fetch_add(1),
      .backend_private_state = std::shared_ptr<const void>(
          window,
          [](const void* value) {
            ANativeWindow_release(
                const_cast<ANativeWindow*>(
                    static_cast<const ANativeWindow*>(value)));
          }),
  };
}

std::unique_ptr<runtime::presentation::ViewportPresenter>
create_platform_viewport_presenter(
    runtime::gpu::GpuDeviceService& device_service,
    runtime::presentation::ViewportFrameRenderer& renderer,
    std::shared_ptr<runtime::gpu::GpuObservabilityService>) {
  return std::make_unique<AndroidVulkanContractCanaryPresenter>(device_service,
                                                               renderer);
}

}  // namespace refusion::platform
