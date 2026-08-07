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

enum class DeviceStatus : std::uint8_t {
  ready,
  suspended,
  lost,
};

enum class DeviceLifecycleEvent : std::uint8_t {
  will_sleep,
  did_wake,
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

struct DeviceHealth final {
  DeviceIdentity identity;
  DeviceStatus status{DeviceStatus::lost};
  std::uint64_t event_sequence{0};
  std::string code;
  std::string diagnostic;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool generation_matches(
      const DeviceIdentity& candidate) const noexcept;
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

  [[nodiscard]] virtual DeviceIdentity identity() const noexcept = 0;
  [[nodiscard]] virtual DeviceHealth health() const = 0;
  [[nodiscard]] virtual DeviceHealth handle_lifecycle_event(
      DeviceLifecycleEvent event) = 0;
  [[nodiscard]] virtual DeviceHealth report_device_loss(
      std::string diagnostic) = 0;
  [[nodiscard]] virtual DeviceLease borrow() = 0;
};

}  // namespace refusion::runtime::gpu
