#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <QString>

#include <memory>

class QWindow;
class StudioTransportBridge;

class StudioRuntimeComposition {
 public:
  virtual ~StudioRuntimeComposition() = default;

  [[nodiscard]] virtual QWindow* viewport_window() noexcept = 0;
  [[nodiscard]] virtual StudioTransportBridge* transport_bridge() noexcept = 0;
};

[[nodiscard]] std::unique_ptr<StudioRuntimeComposition>
create_studio_runtime_composition(const refusion::core::ProjectSnapshot& project,
                                  const QString& project_path);
