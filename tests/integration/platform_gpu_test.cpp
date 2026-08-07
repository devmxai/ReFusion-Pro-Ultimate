#include "refusion/platform/PlatformGpuDeviceService.hpp"

#include <stdexcept>
#include <string>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("platform GPU test requirement failed");
  }
}

}  // namespace

int main() {
  auto service = refusion::platform::create_platform_gpu_device_service();
  require(service != nullptr);
  const auto initial_identity = service->identity();
  require(!initial_identity.adapter_name.empty());
  require(initial_identity.generation != 0);
  require(service->health().ready());

  auto lease = service->borrow();
  require(lease.valid());
  require(lease.identity().generation == service->identity().generation);
  require(lease.native_handles().device != 0);
  require(lease.native_handles().command_queue != 0);

  using refusion::runtime::gpu::DeviceLifecycleEvent;
  using refusion::runtime::gpu::DeviceStatus;
  const auto suspended =
      service->handle_lifecycle_event(DeviceLifecycleEvent::will_sleep);
  require(suspended.status == DeviceStatus::suspended);
  require(suspended.identity.generation == initial_identity.generation);
  bool suspended_borrow_rejected = false;
  try {
    static_cast<void>(service->borrow());
  } catch (const std::runtime_error&) {
    suspended_borrow_rejected = true;
  }
  require(suspended_borrow_rejected);

  const auto resumed =
      service->handle_lifecycle_event(DeviceLifecycleEvent::did_wake);
  require(resumed.ready());
  require(resumed.identity.generation == initial_identity.generation);
  require(service->borrow().valid());

  const auto lost = service->report_device_loss("injected integration-test loss");
  require(lost.status == DeviceStatus::lost);
  require(lost.identity.generation == initial_identity.generation + 1);
  require(!lost.generation_matches(lease.identity()));
  const auto repeated_loss = service->report_device_loss("repeated loss");
  require(repeated_loss.identity.generation == lost.identity.generation);
  bool lost_borrow_rejected = false;
  try {
    static_cast<void>(service->borrow());
  } catch (const std::runtime_error&) {
    lost_borrow_rejected = true;
  }
  require(lost_borrow_rejected);
}
