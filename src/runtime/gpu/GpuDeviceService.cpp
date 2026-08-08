#include "refusion/runtime/gpu/GpuDeviceService.hpp"

#include <stdexcept>
#include <utility>

namespace refusion::runtime::gpu {

bool DeviceHealth::ready() const noexcept {
  return status == DeviceStatus::ready;
}

bool DeviceHealth::generation_matches(
    const DeviceIdentity& candidate) const noexcept {
  return identity.backend == candidate.backend &&
         identity.adapter_id == candidate.adapter_id &&
         identity.generation == candidate.generation;
}

BackendDeviceLease::BackendDeviceLease(
    DeviceIdentity identity,
    std::shared_ptr<const void> backend_device,
    std::shared_ptr<const void> backend_submission_queue)
    : identity_(std::move(identity)),
      backend_device_(std::move(backend_device)),
      backend_submission_queue_(std::move(backend_submission_queue)) {
  if (identity_.adapter_name.empty() || identity_.generation == 0) {
    throw std::invalid_argument("GPU device identity is incomplete");
  }
  if (!backend_device_ || !backend_submission_queue_) {
    throw std::invalid_argument("GPU device lease has invalid backend state");
  }
}

const DeviceIdentity& BackendDeviceLease::identity() const noexcept {
  return identity_;
}

bool BackendDeviceLease::valid() const noexcept {
  return identity_.generation != 0 && static_cast<bool>(backend_device_) &&
         static_cast<bool>(backend_submission_queue_);
}

const void* BackendDeviceLease::backend_private_device() const noexcept {
  return backend_device_.get();
}

const void* BackendDeviceLease::backend_private_submission_queue()
    const noexcept {
  return backend_submission_queue_.get();
}

}  // namespace refusion::runtime::gpu
