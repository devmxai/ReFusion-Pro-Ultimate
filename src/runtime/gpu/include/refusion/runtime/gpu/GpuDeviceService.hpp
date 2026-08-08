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

  friend bool operator==(const DeviceIdentity&, const DeviceIdentity&) = default;
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

// A lifetime-bearing, backend-opaque borrow of the engine-owned GPU device.
// Runtime may inspect identity only. The two private accessors are for native
// backend/media bridge translation units and are guarded by architecture-check.
// They intentionally expose neither integer handles nor any native API type.
class BackendDeviceLease final {
 public:
  BackendDeviceLease(DeviceIdentity identity,
                     std::shared_ptr<const void> backend_device,
                     std::shared_ptr<const void> backend_submission_queue);

  BackendDeviceLease(const BackendDeviceLease&) = delete;
  BackendDeviceLease& operator=(const BackendDeviceLease&) = delete;
  BackendDeviceLease(BackendDeviceLease&&) noexcept = default;
  BackendDeviceLease& operator=(BackendDeviceLease&&) noexcept = default;
  ~BackendDeviceLease() = default;

  [[nodiscard]] const DeviceIdentity& identity() const noexcept;
  [[nodiscard]] bool valid() const noexcept;

  // Backend-private opaque access. Callers must first validate identity().
  [[nodiscard]] const void* backend_private_device() const noexcept;
  [[nodiscard]] const void* backend_private_submission_queue() const noexcept;

 private:
  DeviceIdentity identity_;
  std::shared_ptr<const void> backend_device_;
  std::shared_ptr<const void> backend_submission_queue_;
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
  [[nodiscard]] virtual BackendDeviceLease borrow() = 0;
};

}  // namespace refusion::runtime::gpu
