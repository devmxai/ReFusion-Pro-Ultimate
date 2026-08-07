#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace refusion::runtime::gpu {

enum class Backend : std::uint8_t {
  metal,
  direct3d12,
  vulkan,
};

struct DeviceIdentity final {
  Backend backend{Backend::metal};
  std::string adapter_name;
  std::uint64_t adapter_id{0};
  std::uint64_t generation{0};
};

struct NativeHandles final {
  std::uintptr_t device{0};
  std::uintptr_t command_queue{0};
};

class DeviceLease final {
 public:
  DeviceLease(DeviceIdentity identity,
              NativeHandles handles,
              std::shared_ptr<const void> lifetime);

  DeviceLease(const DeviceLease&) = delete;
  DeviceLease& operator=(const DeviceLease&) = delete;
  DeviceLease(DeviceLease&&) noexcept = default;
  DeviceLease& operator=(DeviceLease&&) noexcept = default;
  ~DeviceLease() = default;

  [[nodiscard]] const DeviceIdentity& identity() const noexcept;
  [[nodiscard]] NativeHandles native_handles() const noexcept;
  [[nodiscard]] bool valid() const noexcept;

 private:
  DeviceIdentity identity_;
  NativeHandles handles_;
  std::shared_ptr<const void> lifetime_;
};

class GpuDeviceService {
 public:
  virtual ~GpuDeviceService() = default;

  [[nodiscard]] virtual const DeviceIdentity& identity() const noexcept = 0;
  [[nodiscard]] virtual DeviceLease borrow() = 0;
};

}  // namespace refusion::runtime::gpu
