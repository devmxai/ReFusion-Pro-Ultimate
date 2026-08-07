#include "refusion/adapters/skia/SkiaRuntime.hpp"

#if defined(REFUSION_TEST_SKIA_GPU_CONTEXTS)
#include "TestComposition.hpp"
#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/platform/PlatformGpuDeviceService.hpp"
#endif

#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("Skia adapter test requirement failed");
  }
}

}  // namespace

int main() {
  using refusion::adapters::skia::SkiaRuntime;

  SkiaRuntime::initialize();
  const auto identity = SkiaRuntime::build_identity();
  require(identity.source_revision.size() == 40);
  require(identity.milestone > 0);
  require(identity.ganesh);
  require(identity.graphite);
#if defined(__APPLE__)
  require(identity.metal);
  require(!identity.direct3d);
#elif defined(_WIN32)
  require(identity.direct3d);
  require(!identity.metal);
#endif

#if defined(REFUSION_TEST_SKIA_GPU_CONTEXTS)
  auto device_service = refusion::platform::create_platform_gpu_device_service();
  auto contexts = refusion::adapters::skia::SkiaGpuContexts::create(
      device_service->borrow(), test_composition());
  require(contexts->ganesh_ready());
  require(contexts->graphite_ready());
#endif
}
