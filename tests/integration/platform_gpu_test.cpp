#include "refusion/platform/PlatformGpuDeviceService.hpp"

#include <stdexcept>

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
  require(!service->identity().adapter_name.empty());
  require(service->identity().generation != 0);

  auto lease = service->borrow();
  require(lease.valid());
  require(lease.identity().generation == service->identity().generation);
  require(lease.native_handles().device != 0);
  require(lease.native_handles().command_queue != 0);
}
