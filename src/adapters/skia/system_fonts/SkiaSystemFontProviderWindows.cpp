#include "SkiaSystemFontProvider.hpp"

#include "include/ports/SkTypeface_win.h"

namespace refusion::adapters::skia {

sk_sp<SkFontMgr> make_unqualified_system_font_manager() {
  return SkFontMgr_New_DirectWrite();
}

}  // namespace refusion::adapters::skia
