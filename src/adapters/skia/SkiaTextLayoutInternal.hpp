#pragma once

#include "refusion/core/FontAssetResolver.hpp"
#include "refusion/core/TextLayout.hpp"

#include <memory>
#include <string>

class SkCanvas;
class SkPaint;

namespace refusion::adapters::skia {

class SkiaTextLayoutEngine final : public core::TextLayoutPort {
 public:
  SkiaTextLayoutEngine();
  explicit SkiaTextLayoutEngine(
      std::shared_ptr<core::FontAssetResolverPort> font_assets);
  ~SkiaTextLayoutEngine() override;

  SkiaTextLayoutEngine(const SkiaTextLayoutEngine&) = delete;
  SkiaTextLayoutEngine& operator=(const SkiaTextLayoutEngine&) = delete;

  [[nodiscard]] std::string layout_engine_digest() const override;
  [[nodiscard]] core::TextLayoutOutcome layout(
      const core::TextLayoutRequest& request) override;

  // Draws only a result already accepted by layout(). A missing key is a hard
  // failure: preview is forbidden from independently reshaping authored text.
  [[nodiscard]] bool draw_cached(SkCanvas& canvas,
                                 const std::string& cache_key,
                                 const SkPaint& paint);

 private:
  struct Implementation;
  std::unique_ptr<Implementation> implementation_;
};

}  // namespace refusion::adapters::skia
