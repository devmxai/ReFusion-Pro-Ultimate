#pragma once

#include "refusion/application/ProjectCandidateAdmission.hpp"
#include "refusion/core/FontAssetResolver.hpp"

#include <QString>

#include <memory>

class QWindow;
class StudioTransportBridge;

class StudioRuntimeComposition
    : public refusion::application::ProjectCandidateAdmissionPort {
 public:
  virtual ~StudioRuntimeComposition() = default;

  [[nodiscard]] virtual QWindow* viewport_window() noexcept = 0;
  [[nodiscard]] virtual StudioTransportBridge* transport_bridge() noexcept = 0;
};

[[nodiscard]] std::shared_ptr<StudioRuntimeComposition>
create_studio_runtime_composition(const refusion::core::ProjectSnapshot& project,
                                  const QString& project_path,
                                  std::shared_ptr<
                                      refusion::core::FontAssetResolverPort>
                                      font_assets);
