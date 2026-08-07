#include "StudioRuntimeComposition.hpp"

#include <memory>

namespace {

class NullRuntimeComposition final : public StudioRuntimeComposition {
 public:
  [[nodiscard]] QWindow* viewport_window() noexcept override { return nullptr; }
};

}  // namespace

std::unique_ptr<StudioRuntimeComposition> create_studio_runtime_composition() {
  return std::make_unique<NullRuntimeComposition>();
}
