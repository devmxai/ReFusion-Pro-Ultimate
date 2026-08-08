#include "SkiaSystemFontProvider.hpp"

#include "include/ports/SkFontMgr_mac_ct.h"

namespace refusion::adapters::skia {

sk_sp<SkFontMgr> make_unqualified_system_font_manager() {
  return SkFontMgr_New_CoreText(nullptr);
}

}  // namespace refusion::adapters::skia
