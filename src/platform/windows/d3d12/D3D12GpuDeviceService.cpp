#include "refusion/platform/PlatformGpuDeviceService.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace refusion::platform {
namespace {

using Microsoft::WRL::ComPtr;

[[nodiscard]] std::string utf8(const wchar_t* value) {
  const int required = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr,
                                            nullptr);
  if (required <= 1) {
    return "D3D12 adapter";
  }
  std::string converted(static_cast<std::size_t>(required), '\0');
  const int converted_size = WideCharToMultiByte(
      CP_UTF8, 0, value, -1, converted.data(), required, nullptr, nullptr);
  if (converted_size != required) {
    throw std::runtime_error("D3D12 adapter name UTF-8 conversion failed");
  }
  converted.pop_back();
  return converted;
}

struct D3D12State final {
  ComPtr<IDXGIAdapter1> adapter;
  ComPtr<ID3D12Device> device;
  ComPtr<ID3D12CommandQueue> command_queue;
};

class D3D12GpuDeviceService final : public runtime::gpu::GpuDeviceService {
 public:
  D3D12GpuDeviceService() {
    ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
      throw std::runtime_error("DXGI factory creation failed");
    }

    ComPtr<IDXGIAdapter1> selected_adapter;
    ComPtr<ID3D12Device> selected_device;
    for (UINT index = 0;; ++index) {
      ComPtr<IDXGIAdapter1> candidate;
      const HRESULT enumeration = factory->EnumAdapterByGpuPreference(
          index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
          IID_PPV_ARGS(&candidate));
      if (enumeration == DXGI_ERROR_NOT_FOUND) {
        break;
      }
      if (FAILED(enumeration) || !candidate) {
        throw std::runtime_error("DXGI hardware-adapter enumeration failed");
      }
      DXGI_ADAPTER_DESC1 description{};
      if (FAILED(candidate->GetDesc1(&description))) {
        throw std::runtime_error("DXGI adapter description query failed");
      }
      if ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) {
        continue;
      }
      ComPtr<ID3D12Device> device;
      if (SUCCEEDED(D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_11_0,
                                      IID_PPV_ARGS(&device)))) {
        selected_adapter = candidate;
        selected_device = device;
        break;
      }
    }
    if (!selected_adapter || !selected_device) {
      throw std::runtime_error("no hardware D3D12 adapter was admitted");
    }

    D3D12_COMMAND_QUEUE_DESC queue_description{};
    queue_description.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ComPtr<ID3D12CommandQueue> command_queue;
    if (FAILED(selected_device->CreateCommandQueue(&queue_description,
                                                    IID_PPV_ARGS(&command_queue)))) {
      throw std::runtime_error("D3D12 command queue creation failed");
    }

    DXGI_ADAPTER_DESC1 description{};
    if (FAILED(selected_adapter->GetDesc1(&description))) {
      throw std::runtime_error("selected DXGI adapter description query failed");
    }
    const auto low = static_cast<std::uint32_t>(description.AdapterLuid.LowPart);
    const auto high = static_cast<std::uint32_t>(description.AdapterLuid.HighPart);
    identity_ = runtime::gpu::DeviceIdentity{
        .backend = runtime::gpu::Backend::direct3d12,
        .adapter_name = utf8(description.Description),
        .adapter_id = (static_cast<std::uint64_t>(high) << 32U) | low,
        .generation = 1,
    };
    state_ = std::make_shared<D3D12State>(D3D12State{
        .adapter = selected_adapter,
        .device = selected_device,
        .command_queue = command_queue,
    });
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
        diagnostic_ = "D3D12 presentation suspended before system sleep";
      }
      return health_locked();
    }
    if (status_ == runtime::gpu::DeviceStatus::lost) {
      return health_locked();
    }
    const HRESULT removed_reason = state_->device->GetDeviceRemovedReason();
    if (FAILED(removed_reason)) {
      mark_lost_locked("D3D12 device reported removal after system wake");
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
            .device = reinterpret_cast<std::uintptr_t>(state_->device.Get()),
            .command_queue = reinterpret_cast<std::uintptr_t>(state_->command_queue.Get()),
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
    diagnostic_ = diagnostic.empty() ? "D3D12 device was lost" : std::move(diagnostic);
  }

  mutable std::mutex mutex_;
  runtime::gpu::DeviceIdentity identity_;
  std::shared_ptr<D3D12State> state_;
  runtime::gpu::DeviceStatus status_{runtime::gpu::DeviceStatus::ready};
  std::uint64_t event_sequence_{0};
  std::string code_;
  std::string diagnostic_;
};

}  // namespace

std::unique_ptr<runtime::gpu::GpuDeviceService>
create_platform_gpu_device_service() {
  return std::make_unique<D3D12GpuDeviceService>();
}

}  // namespace refusion::platform
