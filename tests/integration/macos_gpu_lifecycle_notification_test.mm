#include "refusion/platform/PlatformGpuDeviceService.hpp"

#import <AppKit/AppKit.h>

#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("macOS GPU lifecycle notification test failed");
  }
}

}  // namespace

int main() {
  using refusion::runtime::gpu::DeviceStatus;

  auto service = refusion::platform::create_platform_gpu_device_service();
  require(service->health().ready());
  const auto initial_generation = service->identity().generation;

  NSNotificationCenter* notifications =
      NSWorkspace.sharedWorkspace.notificationCenter;
  [notifications postNotificationName:NSWorkspaceWillSleepNotification
                                object:NSWorkspace.sharedWorkspace];
  const auto suspended = service->health();
  require(suspended.status == DeviceStatus::suspended);
  require(suspended.event_sequence == 1);
  require(suspended.identity.generation == initial_generation);

  [notifications postNotificationName:NSWorkspaceDidWakeNotification
                                object:NSWorkspace.sharedWorkspace];
  const auto resumed = service->health();
  require(resumed.ready());
  require(resumed.event_sequence == 2);
  require(resumed.identity.generation == initial_generation);
}
