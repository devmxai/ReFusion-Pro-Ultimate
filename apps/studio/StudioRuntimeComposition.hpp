#pragma once

#include <memory>

class QWindow;

class StudioRuntimeComposition {
 public:
  virtual ~StudioRuntimeComposition() = default;

  [[nodiscard]] virtual QWindow* viewport_window() noexcept = 0;
};

[[nodiscard]] std::unique_ptr<StudioRuntimeComposition>
create_studio_runtime_composition();
