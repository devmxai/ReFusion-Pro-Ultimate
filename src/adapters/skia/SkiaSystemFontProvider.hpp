#pragma once

#include "include/core/SkFontMgr.h"

namespace refusion::adapters::skia {

// Explicitly unqualified author convenience. Qualified project text never
// calls this provider and always uses byte-backed packaged fonts.
[[nodiscard]] sk_sp<SkFontMgr> make_unqualified_system_font_manager();

}  // namespace refusion::adapters::skia
