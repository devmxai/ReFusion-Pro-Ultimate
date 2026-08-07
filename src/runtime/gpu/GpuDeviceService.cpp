#include "refusion/runtime/gpu/GpuDeviceService.hpp"

#include <stdexcept>
#include <utility>

namespace refusion::runtime::gpu {

DeviceLease::DeviceLease(DeviceIdentity identity,
                         NativeHandles handles,
                         std::shared_ptr<const void> lifetime)
    : identity_(std::move(identity)),
      handles_(handles),
      lifetime_(std::move(lifetime)) {
  if (identity_.adapter_name.empty() || identity_.generation == 0) {
    throw std::invalid_argument("GPU device identity is incomplete");
  }
  if (handles_.device == 0 || handles_.command_queue == 0 || !lifetime_) {
    throw std::invalid_argument("GPU device lease has invalid native state");
  }
}

const DeviceIdentity& DeviceLease::identity() const noexcept { return identity_; }

NativeHandles DeviceLease::native_handles() const noexcept { return handles_; }

bool DeviceLease::valid() const noexcept {
  return handles_.device != 0 && handles_.command_queue != 0 &&
         static_cast<bool>(lifetime_);
}

}  // namespace refusion::runtime::gpu
