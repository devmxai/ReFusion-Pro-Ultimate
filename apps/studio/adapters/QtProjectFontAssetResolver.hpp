#pragma once

#include "refusion/core/FontAssetResolver.hpp"

#include <QString>

class QtProjectFontAssetResolver final
    : public refusion::core::FontAssetResolverPort {
 public:
  explicit QtProjectFontAssetResolver(QString project_directory);

  [[nodiscard]] refusion::core::FontAssetResolution resolve_font_asset(
      const refusion::core::FontAssetRequest& request) override;

 private:
  QString project_directory_;
  QString font_root_;
};
