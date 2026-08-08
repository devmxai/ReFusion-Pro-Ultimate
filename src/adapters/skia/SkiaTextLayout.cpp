#include "SkiaTextLayoutInternal.hpp"
#include "SkiaSystemFontProvider.hpp"

#include "refusion/adapters/skia/SkiaRuntime.hpp"
#include "refusion/adapters/skia/SkiaTextLayout.hpp"
#include "refusion/core/ContentDigest.hpp"

#include "include/core/SkCanvas.h"
#include "include/core/SkData.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkPaint.h"
#include "include/core/SkString.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkTypeface.h"
#include "include/ports/SkFontMgr_data.h"
#include "modules/skshaper/include/SkShaper.h"
#include "modules/skshaper/include/SkShaper_harfbuzz.h"
#include "modules/skunicode/include/SkUnicode_icu.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace refusion::adapters::skia {
namespace {

using core::LocalRect;
using core::ParagraphDirection;
using core::TextHorizontalAlignment;
using core::TextLayoutDiagnostic;
using core::TextLayoutOutcome;
using core::TextLayoutRequest;
using core::TextLayoutResult;
using core::TextLineLayout;
using core::TextVerticalAlignment;

[[nodiscard]] LocalRect from_sk_rect(const SkRect& rect) noexcept {
  return LocalRect{
      .left = rect.left(),
      .top = rect.top(),
      .right = rect.right(),
      .bottom = rect.bottom(),
  };
}

[[nodiscard]] LocalRect translated(const LocalRect& rect,
                                   const double x,
                                   const double y) noexcept {
  return LocalRect{
      .left = rect.left + x,
      .top = rect.top + y,
      .right = rect.right + x,
      .bottom = rect.bottom + y,
  };
}

[[nodiscard]] LocalRect united(const LocalRect& lhs,
                               const LocalRect& rhs) noexcept {
  return LocalRect{
      .left = std::min(lhs.left, rhs.left),
      .top = std::min(lhs.top, rhs.top),
      .right = std::max(lhs.right, rhs.right),
      .bottom = std::max(lhs.bottom, rhs.bottom),
  };
}

[[nodiscard]] LocalRect intersected(const LocalRect& lhs,
                                    const LocalRect& rhs) noexcept {
  const LocalRect result{
      .left = std::max(lhs.left, rhs.left),
      .top = std::max(lhs.top, rhs.top),
      .right = std::min(lhs.right, rhs.right),
      .bottom = std::min(lhs.bottom, rhs.bottom),
  };
  if (result.right < result.left || result.bottom < result.top) {
    return {};
  }
  return result;
}

[[nodiscard]] bool exceeds(const LocalRect& rect,
                           const LocalRect& limit) noexcept {
  constexpr double epsilon = 0.01;
  return rect.left < limit.left - epsilon || rect.top < limit.top - epsilon ||
         rect.right > limit.right + epsilon ||
         rect.bottom > limit.bottom + epsilon;
}

class LetterSpacingBlobHandler final : public SkShaper::RunHandler {
 public:
  LetterSpacingBlobHandler(const char* utf8_text, const float letter_spacing)
      : utf8_text_(utf8_text), letter_spacing_(letter_spacing) {}

  void beginLine() override {
    current_position_ = {0.0F, 0.0F};
    max_run_ascent_ = 0.0F;
    max_run_descent_ = 0.0F;
    max_run_leading_ = 0.0F;
    total_cluster_count_ = 0;
    glyph_ids_.clear();
    glyph_positions_x_.clear();
    have_ink_bounds_ = false;
    ink_bounds_ = SkRect::MakeEmpty();
  }

  void runInfo(const RunInfo& info) override {
    SkFontMetrics metrics;
    info.fFont.getMetrics(&metrics);
    max_run_ascent_ = std::min(max_run_ascent_, metrics.fAscent);
    max_run_descent_ = std::max(max_run_descent_, metrics.fDescent);
    max_run_leading_ = std::max(max_run_leading_, metrics.fLeading);
  }

  void commitRunInfo() override {}

  Buffer runBuffer(const RunInfo& info) override {
    glyph_count_ = info.glyphCount <= static_cast<std::size_t>(INT_MAX)
                       ? static_cast<int>(info.glyphCount)
                       : INT_MAX;
    const int utf8_count =
        info.utf8Range.size() <= static_cast<std::size_t>(INT_MAX)
            ? static_cast<int>(info.utf8Range.size())
            : INT_MAX;
    const auto& buffer =
        builder_.allocRunTextPos(info.fFont, glyph_count_, utf8_count);
    if (buffer.utf8text != nullptr && utf8_text_ != nullptr) {
      std::memcpy(buffer.utf8text,
                  utf8_text_ + info.utf8Range.begin(),
                  static_cast<std::size_t>(utf8_count));
    }
    clusters_ = buffer.clusters;
    glyphs_ = buffer.glyphs;
    positions_ = buffer.points();
    cluster_offset_ = static_cast<int>(info.utf8Range.begin());
    return Buffer{
        .glyphs = buffer.glyphs,
        .positions = positions_,
        .offsets = nullptr,
        .clusters = buffer.clusters,
        .point = current_position_,
    };
  }

  void commitRunBuffer(const RunInfo& info) override {
    const float direction = info.fAdvance.fX < 0.0F ? -1.0F : 1.0F;
    int cluster_ordinal = 0;
    std::optional<std::uint32_t> previous_cluster;
    for (int index = 0; index < glyph_count_; ++index) {
      if (clusters_ != nullptr) {
        clusters_[index] -= static_cast<std::uint32_t>(cluster_offset_);
        if (previous_cluster && *previous_cluster != clusters_[index]) {
          ++cluster_ordinal;
        }
        previous_cluster = clusters_[index];
      }
      positions_[index].fX += direction * letter_spacing_ *
                              static_cast<float>(cluster_ordinal);
      auto glyph_bounds = info.fFont.getBounds(glyphs_[index], nullptr);
      glyph_bounds.offset(positions_[index].fX, positions_[index].fY);
      if (!glyph_bounds.isEmpty()) {
        if (have_ink_bounds_) {
          ink_bounds_.join(glyph_bounds);
        } else {
          ink_bounds_ = glyph_bounds;
          have_ink_bounds_ = true;
        }
      }
      glyph_ids_.push_back(static_cast<std::uint32_t>(glyphs_[index]));
      glyph_positions_x_.push_back(
          static_cast<double>(positions_[index].fX));
    }
    const int run_cluster_count = glyph_count_ == 0 ? 0 : cluster_ordinal + 1;
    current_position_ += info.fAdvance;
    current_position_.fX += direction * letter_spacing_ *
                            static_cast<float>(run_cluster_count);
    total_cluster_count_ += run_cluster_count;
  }

  void commitLine() override {}

  [[nodiscard]] sk_sp<SkTextBlob> make_blob() { return builder_.make(); }

  [[nodiscard]] float logical_width() const noexcept {
    const float terminal_spacing =
        total_cluster_count_ == 0 ? 0.0F : std::abs(letter_spacing_);
    return std::max(0.0F, std::abs(current_position_.fX) - terminal_spacing);
  }

  [[nodiscard]] const std::vector<std::uint32_t>& glyph_ids() const noexcept {
    return glyph_ids_;
  }

  [[nodiscard]] const std::vector<double>& glyph_positions_x() const noexcept {
    return glyph_positions_x_;
  }

  [[nodiscard]] double ascent() const noexcept {
    return -static_cast<double>(max_run_ascent_);
  }
  [[nodiscard]] double descent() const noexcept {
    return static_cast<double>(max_run_descent_);
  }
  [[nodiscard]] double leading() const noexcept {
    return static_cast<double>(max_run_leading_);
  }
  [[nodiscard]] LocalRect ink_bounds() const noexcept {
    return have_ink_bounds_ ? from_sk_rect(ink_bounds_) : LocalRect{};
  }

 private:
  SkTextBlobBuilder builder_;
  const char* utf8_text_{nullptr};
  float letter_spacing_{0.0F};
  std::uint32_t* clusters_{nullptr};
  SkGlyphID* glyphs_{nullptr};
  SkPoint* positions_{nullptr};
  int cluster_offset_{0};
  int glyph_count_{0};
  int total_cluster_count_{0};
  std::vector<std::uint32_t> glyph_ids_;
  std::vector<double> glyph_positions_x_;
  SkPoint current_position_{0.0F, 0.0F};
  float max_run_ascent_{0.0F};
  float max_run_descent_{0.0F};
  float max_run_leading_{0.0F};
  bool have_ink_bounds_{false};
  SkRect ink_bounds_;
};

struct ShapedLine final {
  std::size_t utf8_start{0};
  std::size_t utf8_length{0};
  sk_sp<SkTextBlob> blob;
  double width{0.0};
  LocalRect ink_bounds;
  double ascent{0.0};
  double descent{0.0};
  double leading{0.0};
  std::vector<std::uint32_t> glyph_ids;
  std::vector<double> glyph_positions_x;
};

struct CachedLayout final {
  TextLayoutResult result;
  std::vector<sk_sp<SkTextBlob>> blobs;
};

[[nodiscard]] TextLayoutOutcome failure(std::string code,
                                        std::string message) {
  return TextLayoutOutcome{
      .result = std::nullopt,
      .diagnostic = TextLayoutDiagnostic{
          .code = std::move(code),
          .message = std::move(message),
      },
  };
}

[[nodiscard]] std::size_t trim_right_ascii_space(const std::string& text,
                                                 const std::size_t begin,
                                                 std::size_t end) {
  while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t')) {
    --end;
  }
  return end;
}

}  // namespace

struct SkiaTextLayoutEngine::Implementation final {
  struct QualifiedFont final {
    sk_sp<SkData> data;
    sk_sp<SkFontMgr> font_manager;
    sk_sp<SkTypeface> typeface;
    std::unique_ptr<SkShaper> shaper;
    std::string resolved_digest;
  };

  std::shared_ptr<core::FontAssetResolverPort> font_assets;
  sk_sp<SkFontMgr> system_font_manager;
  sk_sp<SkUnicode> unicode;
  std::unique_ptr<SkShaper> system_shaper;
  std::string engine_digest;
  std::unordered_map<std::string, CachedLayout> cache;
  std::unordered_map<std::string, std::unique_ptr<QualifiedFont>>
      qualified_fonts;
  std::mutex mutex;

  [[nodiscard]] bool exact_system_family(const std::string& family) const {
    for (int index = 0; index < system_font_manager->countFamilies(); ++index) {
      SkString candidate;
      system_font_manager->getFamilyName(index, &candidate);
      if (family == candidate.c_str()) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] ShapedLine shape(const std::string& source,
                                 const std::size_t start,
                                 const std::size_t length,
                                 const SkFont& font,
                                 const bool left_to_right,
                                 const double letter_spacing,
                                 SkShaper& shaper) const {
    const std::string line = source.substr(start, length);
    LetterSpacingBlobHandler handler(
        line.c_str(), static_cast<float>(letter_spacing));
    // Wrapping is decided above from explicit TextBox measurements. Keep the
    // shaper on one line with a large, finite width; near-FLT_MAX widths lose
    // positioning precision inside bidi/run calculations.
    constexpr float unwrapped_width = 1'000'000.0F;
    shaper.shape(line.c_str(), line.size(), font, left_to_right,
                 unwrapped_width, &handler);
    auto blob = handler.make_blob();
    return ShapedLine{
        .utf8_start = start,
        .utf8_length = length,
        .blob = blob,
        .width = handler.logical_width(),
        .ink_bounds = handler.ink_bounds(),
        .ascent = handler.ascent(),
        .descent = handler.descent(),
        .leading = handler.leading(),
        .glyph_ids = handler.glyph_ids(),
        .glyph_positions_x = handler.glyph_positions_x(),
    };
  }

  [[nodiscard]] std::vector<ShapedLine> shape_lines(
      const TextLayoutRequest& request,
      const SkFont& font,
      SkShaper& shaper) const {
    const auto& text = request.text;
    const bool left_to_right =
        text.direction == ParagraphDirection::left_to_right;
    const double available_width =
        core::text_box_content_bounds(text.box).right -
        core::text_box_content_bounds(text.box).left;
    std::vector<ShapedLine> lines;

    const auto emit = [&](const std::size_t start,
                          const std::size_t end) {
      lines.push_back(shape(text.text, start, end - start, font,
                            left_to_right, text.letter_spacing, shaper));
    };

    std::size_t paragraph_start = 0;
    while (paragraph_start <= text.text.size()) {
      const auto newline = text.text.find('\n', paragraph_start);
      const std::size_t paragraph_end =
          newline == std::string::npos ? text.text.size() : newline;
      if (text.wrap == core::TextWrapMode::no_wrap ||
          paragraph_start == paragraph_end) {
        emit(paragraph_start, paragraph_end);
      } else {
        std::size_t line_start = paragraph_start;
        std::size_t previous_break = paragraph_start;
        auto breaks = unicode->makeBreakIterator(SkUnicode::BreakType::kLines);
        const auto paragraph_length = paragraph_end - paragraph_start;
        if (!breaks || paragraph_length > static_cast<std::size_t>(INT_MAX) ||
            !breaks->setText(text.text.data() + paragraph_start,
                             static_cast<int>(paragraph_length))) {
          throw std::runtime_error(
              "Skia ICU failed to create the deterministic line-break program");
        }
        for (auto position = breaks->first(); !breaks->isDone();
             position = breaks->next()) {
          if (position <= 0) {
            continue;
          }
          const auto candidate_end =
              paragraph_start + static_cast<std::size_t>(position);
          const auto candidate = shape(
              text.text, line_start, candidate_end - line_start, font,
              left_to_right, text.letter_spacing, shaper);
          if (candidate.width > available_width &&
              previous_break > line_start) {
            const auto trimmed =
                trim_right_ascii_space(text.text, line_start, previous_break);
            emit(line_start, trimmed);
            line_start = previous_break;
          }
          previous_break = candidate_end;
        }
        const auto trimmed =
            trim_right_ascii_space(text.text, line_start, paragraph_end);
        emit(line_start, trimmed);
      }
      if (newline == std::string::npos) {
        break;
      }
      paragraph_start = newline + 1;
    }
    return lines;
  }
};

SkiaTextLayoutEngine::SkiaTextLayoutEngine()
    : SkiaTextLayoutEngine(nullptr) {}

SkiaTextLayoutEngine::SkiaTextLayoutEngine(
    std::shared_ptr<core::FontAssetResolverPort> font_assets)
    : implementation_(std::make_unique<Implementation>()) {
  SkiaRuntime::initialize();
  implementation_->font_assets = std::move(font_assets);
  implementation_->system_font_manager =
      make_unqualified_system_font_manager();
  implementation_->unicode = SkUnicodes::ICU::Make();
  implementation_->system_shaper = SkShapers::HB::ShaperDrivenWrapper(
      implementation_->unicode, implementation_->system_font_manager);
  if (!implementation_->system_font_manager || !implementation_->unicode ||
      !implementation_->system_shaper) {
    throw std::runtime_error(
        "Skia failed to create the HarfBuzz/ICU text layout engine");
  }
  const auto build = SkiaRuntime::build_identity();
  implementation_->engine_digest =
      "skia-harfbuzz-icu-freetype-text-layout-v2:" +
      build.source_revision +
      ":linebreak=icu:spacing=cluster:hinting=none";
}

SkiaTextLayoutEngine::~SkiaTextLayoutEngine() = default;

std::string SkiaTextLayoutEngine::layout_engine_digest() const {
  return implementation_->engine_digest;
}

TextLayoutOutcome SkiaTextLayoutEngine::layout(
    const TextLayoutRequest& request) {
  std::scoped_lock lock(implementation_->mutex);
  const auto key =
      core::text_layout_cache_key(request, implementation_->engine_digest);
  if (const auto found = implementation_->cache.find(key);
      found != implementation_->cache.end()) {
    return TextLayoutOutcome{.result = found->second.result};
  }

  const auto& text = request.text;
  sk_sp<SkTypeface> typeface;
  SkShaper* selected_shaper = nullptr;
  bool font_qualified = false;
  std::string resolved_font_digest;
  if (text.font.source == core::FontSourceKind::packaged_asset) {
    if (!implementation_->font_assets) {
      return failure(
          "RFX-FONT-ASSET-RESOLUTION-001",
          "Packaged Font identity is qualified, but no admitted asset-byte "
          "resolver is connected to the Skia text layout port");
    }
    const auto resolution = implementation_->font_assets->resolve_font_asset(
        core::FontAssetRequest{
            .asset_id = text.font.asset_id,
            .expected_content_digest = text.font.content_digest,
            .face_index = 0,
        });
    if (!resolution.succeeded()) {
      return failure(resolution.diagnostic_code.empty()
                         ? "RFX-FONT-ASSET-RESOLUTION-001"
                         : resolution.diagnostic_code,
                     resolution.diagnostic_message.empty()
                         ? "Packaged Font asset resolution failed"
                         : resolution.diagnostic_message);
    }
    const auto& asset = *resolution.asset;
    if (!asset.bytes || asset.bytes->empty() ||
        asset.face_index != 0 ||
        asset.verified_content_digest != text.font.content_digest ||
        !core::content_digest_matches(*asset.bytes,
                                      text.font.content_digest)) {
      return failure(
          "RFX-FONT-ASSET-DIGEST-001",
          "Resolved packaged Font bytes do not match the admitted content digest");
    }

    auto found =
        implementation_->qualified_fonts.find(text.font.content_digest);
    if (found == implementation_->qualified_fonts.end()) {
      auto qualified = std::make_unique<Implementation::QualifiedFont>();
      qualified->data = SkData::MakeWithCopy(asset.bytes->data(),
                                             asset.bytes->size());
      std::array<sk_sp<SkData>, 1> admitted_data{qualified->data};
      qualified->font_manager = SkFontMgr_New_Custom_Data(admitted_data);
      if (!qualified->font_manager ||
          qualified->font_manager->countFamilies() != 1) {
        return failure("RFX-FONT-ASSET-PARSE-001",
                       "FreeType rejected the admitted packaged Font bytes");
      }
      SkString actual_family;
      qualified->font_manager->getFamilyName(0, &actual_family);
      if (text.font.family_name != actual_family.c_str()) {
        return failure(
            "RFX-FONT-ASSET-FAMILY-001",
            "Packaged Font family metadata does not match the admitted bytes");
      }
      qualified->typeface = qualified->font_manager->matchFamilyStyle(
          actual_family.c_str(), SkFontStyle::Normal());
      qualified->shaper = SkShapers::HB::ShaperDrivenWrapper(
          implementation_->unicode, qualified->font_manager);
      qualified->resolved_digest =
          text.font.content_digest +
          ":face=0:weight=400:width=5:slant=upright:variations=none:"
          "language=und:script=auto:features=harfbuzz-default:"
          "fallback=primary-only:linebreak=icu:spacing=cluster";
      if (!qualified->typeface || !qualified->shaper) {
        return failure(
            "RFX-FONT-ASSET-PARSE-001",
            "Skia could not create a deterministic typeface and shaper from the admitted bytes");
      }
      found = implementation_->qualified_fonts
                  .emplace(text.font.content_digest, std::move(qualified))
                  .first;
    }
    typeface = found->second->typeface;
    selected_shaper = found->second->shaper.get();
    resolved_font_digest = found->second->resolved_digest;
    font_qualified = true;
  } else {
    if (!implementation_->exact_system_family(text.font.family_name)) {
      return failure("RFX-FONT-SYSTEM-MISSING-001",
                     "Requested system Font family is not installed: " +
                         text.font.family_name);
    }
    typeface = implementation_->system_font_manager->matchFamilyStyle(
        text.font.family_name.c_str(), SkFontStyle::Normal());
    if (!typeface) {
      return failure("RFX-FONT-SYSTEM-MISSING-001",
                     "Skia could not resolve the exact system Font family: " +
                         text.font.family_name);
    }
    selected_shaper = implementation_->system_shaper.get();
    resolved_font_digest =
        "system-unqualified:" + text.font.family_name + ":" +
        implementation_->engine_digest;
  }

  SkFont font(std::move(typeface), static_cast<float>(text.font_size));
  font.setEdging(SkFont::Edging::kAntiAlias);
  font.setHinting(SkFontHinting::kNone);
  auto shaped_lines =
      implementation_->shape_lines(request, font, *selected_shaper);
  if (shaped_lines.empty()) {
    return failure("RFX-TEXT-LAYOUT-EMPTY-001",
                   "Text layout produced no paragraph line");
  }

  double ascent = 0.0;
  double descent = 0.0;
  double leading = 0.0;
  for (const auto& line : shaped_lines) {
    ascent = std::max(ascent, line.ascent);
    descent = std::max(descent, line.descent);
    leading = std::max(leading, line.leading);
  }
  const double line_advance = std::max(
      text.font_size * text.line_height_ratio, ascent + descent + leading);
  const double total_height = ascent + descent +
                              line_advance *
                                  static_cast<double>(shaped_lines.size() - 1);
  const auto layout_box = core::text_box_bounds(text.box);
  const auto content_box = core::text_box_content_bounds(text.box);
  double first_baseline = content_box.top + ascent;
  if (text.vertical_alignment == TextVerticalAlignment::center) {
    first_baseline =
        (content_box.top + content_box.bottom - total_height) * 0.5 + ascent;
  } else if (text.vertical_alignment == TextVerticalAlignment::bottom) {
    first_baseline = content_box.bottom - total_height + ascent;
  }

  std::vector<TextLineLayout> line_results;
  std::vector<double> baselines;
  std::vector<sk_sp<SkTextBlob>> blobs;
  line_results.reserve(shaped_lines.size());
  baselines.reserve(shaped_lines.size());
  blobs.reserve(shaped_lines.size());
  bool have_logical_bounds = false;
  bool have_ink_bounds = false;
  LocalRect logical_bounds;
  LocalRect ink_bounds;

  for (std::size_t index = 0; index < shaped_lines.size(); ++index) {
    auto& line = shaped_lines[index];
    const bool start_is_left =
        text.horizontal_alignment == TextHorizontalAlignment::left ||
        (text.horizontal_alignment == TextHorizontalAlignment::start &&
         text.direction == ParagraphDirection::left_to_right) ||
        (text.horizontal_alignment == TextHorizontalAlignment::end &&
         text.direction == ParagraphDirection::right_to_left);
    const bool end_is_right =
        text.horizontal_alignment == TextHorizontalAlignment::right ||
        (text.horizontal_alignment == TextHorizontalAlignment::end &&
         text.direction == ParagraphDirection::left_to_right) ||
        (text.horizontal_alignment == TextHorizontalAlignment::start &&
         text.direction == ParagraphDirection::right_to_left);
    double origin_x = content_box.left;
    if (text.horizontal_alignment == TextHorizontalAlignment::center) {
      origin_x = (content_box.left + content_box.right - line.width) * 0.5;
    } else if (end_is_right && !start_is_left) {
      origin_x = content_box.right - line.width;
    }
    const double baseline =
        first_baseline + line_advance * static_cast<double>(index);
    const LocalRect line_logical{
        .left = origin_x,
        .top = baseline - ascent,
        .right = origin_x + line.width,
        .bottom = baseline + descent,
    };
    const auto line_ink = translated(line.ink_bounds, origin_x, baseline);
    logical_bounds = have_logical_bounds
                         ? united(logical_bounds, line_logical)
                         : line_logical;
    if (line.blob) {
      ink_bounds = have_ink_bounds ? united(ink_bounds, line_ink) : line_ink;
    }
    have_logical_bounds = true;
    have_ink_bounds = static_cast<bool>(line.blob) || have_ink_bounds;
    line_results.push_back(TextLineLayout{
        .utf8_start = line.utf8_start,
        .utf8_length = line.utf8_length,
        .origin_x = origin_x,
        .baseline_y = baseline,
        .logical_width = line.width,
        .ink_bounds = line_ink,
        .glyph_ids = std::move(line.glyph_ids),
        .glyph_positions_x = std::move(line.glyph_positions_x),
    });
    baselines.push_back(baseline);
    blobs.push_back(std::move(line.blob));
  }

  const bool overflowed =
      exceeds(logical_bounds, content_box) ||
      (have_ink_bounds && exceeds(ink_bounds, content_box));
  const auto clipped_bounds =
      text.overflow == core::TextOverflowMode::clip
          ? intersected(ink_bounds, content_box)
          : ink_bounds;
  TextLayoutResult result{
      .layout_box = layout_box,
      .content_box = content_box,
      .logical_bounds = logical_bounds,
      .ink_bounds = ink_bounds,
      .clipped_bounds = clipped_bounds,
      .lines = std::move(line_results),
      .baselines = std::move(baselines),
      .ascent = ascent,
      .descent = descent,
      .leading = leading,
      .overflowed = overflowed,
      .font_qualified = font_qualified,
      .resolved_font_digest = std::move(resolved_font_digest),
      .layout_engine_digest = implementation_->engine_digest,
      .cache_key = key,
  };
  implementation_->cache.emplace(
      key, CachedLayout{.result = result, .blobs = std::move(blobs)});
  return TextLayoutOutcome{.result = std::move(result)};
}

bool SkiaTextLayoutEngine::draw_cached(SkCanvas& canvas,
                                       const std::string& cache_key,
                                       const SkPaint& paint) {
  std::scoped_lock lock(implementation_->mutex);
  const auto found = implementation_->cache.find(cache_key);
  if (found == implementation_->cache.end() ||
      found->second.blobs.size() != found->second.result.lines.size()) {
    return false;
  }
  for (std::size_t index = 0; index < found->second.blobs.size(); ++index) {
    const auto& blob = found->second.blobs[index];
    const auto& line = found->second.result.lines[index];
    if (blob) {
      canvas.drawTextBlob(blob, static_cast<float>(line.origin_x),
                          static_cast<float>(line.baseline_y), paint);
    }
  }
  return true;
}

std::unique_ptr<core::TextLayoutPort> create_skia_text_layout_port() {
  return std::make_unique<SkiaTextLayoutEngine>();
}

std::unique_ptr<core::TextLayoutPort> create_skia_text_layout_port(
    std::shared_ptr<core::FontAssetResolverPort> font_assets) {
  return std::make_unique<SkiaTextLayoutEngine>(std::move(font_assets));
}

}  // namespace refusion::adapters::skia
