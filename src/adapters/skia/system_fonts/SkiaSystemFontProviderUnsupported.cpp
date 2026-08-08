#include "SkiaSystemFontProvider.hpp"

namespace refusion::adapters::skia {

sk_sp<SkFontMgr> make_unqualified_system_font_manager() { return nullptr; }

}  // namespace refusion::adapters::skia
