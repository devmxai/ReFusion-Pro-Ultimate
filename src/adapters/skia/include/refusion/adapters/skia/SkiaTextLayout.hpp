#pragma once

#include <memory>

#include "refusion/core/FontAssetResolver.hpp"
#include "refusion/core/TextLayout.hpp"

namespace refusion::adapters::skia {

// Creates the same backend-neutral layout port used by the live Skia preview.
// Offline probes use this factory so both lanes share one shaping algorithm,
// one set of metrics and one cache-key contract.
[[nodiscard]] std::unique_ptr<core::TextLayoutPort>
create_skia_text_layout_port();

[[nodiscard]] std::unique_ptr<core::TextLayoutPort>
create_skia_text_layout_port(
    std::shared_ptr<core::FontAssetResolverPort> font_assets);

}  // namespace refusion::adapters::skia
