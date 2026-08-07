#include "refusion/adapters/skia/SkiaRuntime.hpp"

#include "include/core/SkGraphics.h"
#include "include/core/SkMilestone.h"

#include <mutex>

namespace refusion::adapters::skia {

void SkiaRuntime::initialize() {
  static std::once_flag initialized;
  std::call_once(initialized, [] { SkGraphics::Init(); });
}

BuildIdentity SkiaRuntime::build_identity() {
  initialize();
  return BuildIdentity{
      .source_revision = REFUSION_SKIA_REVISION,
      .milestone = SK_MILESTONE,
#if defined(SK_GANESH)
      .ganesh = true,
#else
      .ganesh = false,
#endif
#if defined(SK_GRAPHITE)
      .graphite = true,
#else
      .graphite = false,
#endif
#if defined(SK_METAL)
      .metal = true,
#else
      .metal = false,
#endif
#if defined(SK_DIRECT3D)
      .direct3d = true,
#else
      .direct3d = false,
#endif
  };
}

}  // namespace refusion::adapters::skia
