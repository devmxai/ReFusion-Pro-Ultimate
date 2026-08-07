#pragma once

#include <string>

namespace refusion::adapters::skia {

struct BuildIdentity final {
  std::string source_revision;
  int milestone{0};
  bool ganesh{false};
  bool graphite{false};
  bool metal{false};
  bool direct3d{false};
};

class SkiaRuntime final {
 public:
  static void initialize();
  [[nodiscard]] static BuildIdentity build_identity();
};

}  // namespace refusion::adapters::skia
