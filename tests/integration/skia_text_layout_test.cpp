#include "refusion/adapters/skia/SkiaTextLayout.hpp"
#include "refusion/core/CanonicalText.hpp"
#include "refusion/core/ContentDigest.hpp"
#include "refusion/core/FontAssetResolver.hpp"
#include "refusion/core/ProjectDocument.hpp"
#include "refusion/core/TextLayout.hpp"

#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("Skia text layout test requirement failed");
  }
}

[[nodiscard]] refusion::core::TextLayoutRequest request_fixture() {
  using namespace refusion::core;
  return TextLayoutRequest{
      .text = TextLayerContent{
          .text = "Modern text layout",
          .font = FontIdentity{.family_name = "Arial"},
          .font_size = 42.0,
          .box = TextBox{
              .width = 420.0,
              .height = 140.0,
              .padding_top = 10.0,
              .padding_right = 12.0,
              .padding_bottom = 10.0,
              .padding_left = 12.0,
          },
          .horizontal_alignment = TextHorizontalAlignment::center,
          .vertical_alignment = TextVerticalAlignment::center,
          .wrap = TextWrapMode::word,
          .overflow = TextOverflowMode::clip,
          .line_height_ratio = 1.2,
          .letter_spacing = 1.0,
          .fill = ColorRgba8{.red = 255, .green = 255, .blue = 255},
      },
  };
}

class FixtureFontResolver final
    : public refusion::core::FontAssetResolverPort {
 public:
  struct Entry final {
    std::string path;
    std::string digest;
  };

  explicit FixtureFontResolver(std::unordered_map<std::string, Entry> entries)
      : entries_(std::move(entries)) {}

  [[nodiscard]] refusion::core::FontAssetResolution resolve_font_asset(
      const refusion::core::FontAssetRequest& request) override {
    const auto found = entries_.find(request.asset_id);
    if (found == entries_.end()) {
      return {.diagnostic_code = "RFX-FONT-ASSET-MISSING-001",
              .diagnostic_message = "fixture asset ID is not admitted"};
    }
    if (found->second.digest != request.expected_content_digest) {
      return {.diagnostic_code = "RFX-FONT-ASSET-DIGEST-001",
              .diagnostic_message = "fixture expected digest differs"};
    }
    std::ifstream input(found->second.path, std::ios::binary);
    if (!input) {
      return {.diagnostic_code = "RFX-FONT-ASSET-MISSING-001",
              .diagnostic_message = "fixture font bytes are missing"};
    }
    auto bytes = std::make_shared<std::vector<std::uint8_t>>(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
    const auto actual = refusion::core::sha256_content_digest(*bytes);
    if (actual != found->second.digest) {
      return {.diagnostic_code = "RFX-FONT-ASSET-DIGEST-001",
              .diagnostic_message = "fixture font bytes changed"};
    }
    return {.asset = refusion::core::FontAssetBlob{
                .bytes = std::move(bytes),
                .verified_content_digest = actual,
                .face_index = request.face_index,
            }};
  }

 private:
  std::unordered_map<std::string, Entry> entries_;
};

[[nodiscard]] std::string layout_receipt(
    const std::string& name,
    const refusion::core::TextLayoutResult& result) {
  using refusion::core::canonical_float64;
  std::ostringstream output;
  output << "case=" << name << "\n"
         << "qualified=" << (result.font_qualified ? 1 : 0) << "\n"
         << "font=" << result.resolved_font_digest << "\n"
         << "ascent=" << canonical_float64(result.ascent) << "\n"
         << "descent=" << canonical_float64(result.descent) << "\n"
         << "leading=" << canonical_float64(result.leading) << "\n"
         << "logical=" << canonical_float64(result.logical_bounds.left) << ','
         << canonical_float64(result.logical_bounds.top) << ','
         << canonical_float64(result.logical_bounds.right) << ','
         << canonical_float64(result.logical_bounds.bottom) << "\n"
         << "ink=" << canonical_float64(result.ink_bounds.left) << ','
         << canonical_float64(result.ink_bounds.top) << ','
         << canonical_float64(result.ink_bounds.right) << ','
         << canonical_float64(result.ink_bounds.bottom) << "\n"
         << "line_count=" << result.lines.size() << "\n";
  for (std::size_t index = 0; index < result.lines.size(); ++index) {
    const auto& line = result.lines[index];
    output << "line=" << index << ':' << line.utf8_start << ':'
           << line.utf8_length << ':' << canonical_float64(line.origin_x)
           << ':' << canonical_float64(line.baseline_y) << ':'
           << canonical_float64(line.logical_width) << "\n"
           << "glyphs=";
    for (std::size_t glyph = 0; glyph < line.glyph_ids.size(); ++glyph) {
      if (glyph != 0) {
        output << ',';
      }
      output << line.glyph_ids[glyph];
    }
    output << "\npositions=";
    for (std::size_t glyph = 0; glyph < line.glyph_positions_x.size(); ++glyph) {
      if (glyph != 0) {
        output << ',';
      }
      output << canonical_float64(line.glyph_positions_x[glyph]);
    }
    output << "\n";
  }
  return output.str();
}

[[nodiscard]] std::string read_text_file(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  require(static_cast<bool>(input));
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  using namespace refusion::core;
  auto layout =
      refusion::adapters::skia::create_skia_text_layout_port();
  require(layout != nullptr);
  require(layout->layout_engine_digest().starts_with(
      "skia-harfbuzz-icu-freetype-text-layout-v2:"));

  const auto request = request_fixture();
  const auto first = layout->layout(request);
  require(first.succeeded());
  require(first.result->lines.size() == 1);
  require(first.result->baselines.size() == first.result->lines.size());
  require(first.result->ascent > 0.0);
  require(first.result->descent >= 0.0);
  require(!first.result->font_qualified);
  require(first.result->resolved_font_digest.starts_with(
      "system-unqualified:Arial:"));
  require(first.result->cache_key ==
          text_layout_cache_key(request, layout->layout_engine_digest()));
  const auto cached = layout->layout(request);
  require(cached.succeeded());
  require(cached.result == first.result);

  auto wrapped = request;
  wrapped.text.text = "one two three four five six seven eight";
  wrapped.text.box.width = 150.0;
  wrapped.text.box.height = 90.0;
  const auto wrapped_result = layout->layout(wrapped);
  require(wrapped_result.succeeded());
  require(wrapped_result.result->lines.size() > 1);
  require(wrapped_result.result->overflowed);
  require(wrapped_result.result->clipped_bounds.left >=
          wrapped_result.result->content_box.left);
  require(wrapped_result.result->clipped_bounds.right <=
          wrapped_result.result->content_box.right);
  require(wrapped_result.result->clipped_bounds.top >=
          wrapped_result.result->content_box.top);
  require(wrapped_result.result->clipped_bounds.bottom <=
          wrapped_result.result->content_box.bottom);

  auto rtl = request;
  rtl.text.text = "مرحبا بكم في ريفيوجن";
  rtl.text.direction = ParagraphDirection::right_to_left;
  rtl.text.horizontal_alignment = TextHorizontalAlignment::start;
  const auto rtl_result = layout->layout(rtl);
  require(rtl_result.succeeded());
  require(!rtl_result.result->lines.empty());
  require(rtl_result.result->lines.front().origin_x >=
          rtl_result.result->content_box.left);

  auto mixed = rtl;
  mixed.text.text = "ReFusion مَرْحَبًا 123";
  const auto mixed_result = layout->layout(mixed);
  require(mixed_result.succeeded());
  require(mixed_result.result->ink_bounds != LocalRect{});

  auto multiline = request;
  multiline.text.text = "first line\nsecond line with words";
  multiline.text.box.width = 260.0;
  multiline.text.box.height = 220.0;
  const auto multiline_result = layout->layout(multiline);
  require(multiline_result.succeeded());
  require(multiline_result.result->lines.size() >= 2);
  require(multiline_result.result->baselines.at(1) >
          multiline_result.result->baselines.at(0));

  auto missing = request;
  missing.text.font.family_name = "ReFusion Font That Cannot Exist 006C";
  const auto missing_result = layout->layout(missing);
  require(!missing_result.succeeded());
  require(missing_result.diagnostic->code ==
          "RFX-FONT-SYSTEM-MISSING-001");

  auto packaged = request;
  packaged.text.font = FontIdentity{
      .source = FontSourceKind::packaged_asset,
      .family_name = "Inter",
      .asset_id = "font_inter_regular",
      .content_digest =
          "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
  };
  const auto packaged_result = layout->layout(packaged);
  require(!packaged_result.succeeded());
  require(packaged_result.diagnostic->code ==
          "RFX-FONT-ASSET-RESOLUTION-001");

  constexpr const char* latin_digest =
      "sha256:f5f552c8c5edb61fe6efb824baf4d4de47b1a8689ab4925ff43f7bd6a4ebece5";
  constexpr const char* arabic_digest =
      "sha256:7ed3fe069312aceac454f17cf613a30f95271d6ed7ce58005ed4d016bd3823d7";
  auto fixture_assets = std::make_shared<FixtureFontResolver>(
      std::unordered_map<std::string, FixtureFontResolver::Entry>{
          {"font_noto_sans_regular",
           {.path = REFUSION_NOTO_SANS_LATIN_PATH,
            .digest = latin_digest}},
          {"font_noto_sans_arabic_regular",
           {.path = REFUSION_NOTO_SANS_ARABIC_PATH,
            .digest = arabic_digest}},
      });
  auto qualified_layout =
      refusion::adapters::skia::create_skia_text_layout_port(fixture_assets);

  auto qualified_latin = request;
  qualified_latin.text.font = FontIdentity{
      .source = FontSourceKind::packaged_asset,
      .family_name = "Noto Sans",
      .asset_id = "font_noto_sans_regular",
      .content_digest = latin_digest,
  };
  qualified_latin.text.text = "ReFusion glyph metrics 0123";
  const auto qualified_latin_result =
      qualified_layout->layout(qualified_latin);
  require(qualified_latin_result.succeeded());
  require(qualified_latin_result.result->font_qualified);
  require(qualified_latin_result.result->resolved_font_digest.starts_with(
      latin_digest));
  require(!qualified_latin_result.result->lines.front().glyph_ids.empty());
  require(qualified_latin_result.result->lines.front().glyph_ids.size() ==
          qualified_latin_result.result->lines.front().glyph_positions_x.size());

  auto qualified_arabic = request;
  qualified_arabic.text.font = FontIdentity{
      .source = FontSourceKind::packaged_asset,
      .family_name = "Noto Sans Arabic",
      .asset_id = "font_noto_sans_arabic_regular",
      .content_digest = arabic_digest,
  };
  qualified_arabic.text.text = "ReFusion مَرْحَبًا ١٢٣";
  qualified_arabic.text.direction = ParagraphDirection::right_to_left;
  qualified_arabic.text.horizontal_alignment =
      TextHorizontalAlignment::start;
  const auto qualified_arabic_result =
      qualified_layout->layout(qualified_arabic);
  require(qualified_arabic_result.succeeded());
  require(qualified_arabic_result.result->font_qualified);
  require(qualified_arabic_result.result->resolved_font_digest.starts_with(
      arabic_digest));
  require(!qualified_arabic_result.result->lines.front().glyph_ids.empty());

  auto wrapped_arabic = qualified_arabic;
  wrapped_arabic.text.text =
      "محرّك فيديو احترافي موحّد عبر جميع المنصات بدون اختلاف";
  wrapped_arabic.text.box.width = 180.0;
  wrapped_arabic.text.box.height = 300.0;
  const auto wrapped_arabic_result = qualified_layout->layout(wrapped_arabic);
  require(wrapped_arabic_result.succeeded());
  require(wrapped_arabic_result.result->lines.size() > 1);
  const auto receipt =
      layout_receipt("latin", *qualified_latin_result.result) +
      layout_receipt("arabic-mixed", *qualified_arabic_result.result) +
      layout_receipt("arabic-wrapped", *wrapped_arabic_result.result);
  require(receipt == read_text_file(REFUSION_NOTO_LAYOUT_RECEIPT_PATH));

  CompositionSnapshot composition{
      .composition_id = CompositionId{"cmp_skia_text"},
      .display_name = "Skia Text",
      .canvas = {.width_pixels = 640, .height_pixels = 360},
      .frame_rate = {.numerator = 30, .denominator = 1},
      .duration = 1'000'000'000,
      .layers = {
          LayerSnapshot{
              .layer_id = LayerId{"lyr_text"},
              .display_name = "Text",
              .active_range = {.start = 0, .duration = 1'000'000'000},
              .transform = {.position_x = 320.0, .position_y = 180.0},
              .content = request.text,
              .effects = {
                  LayerEffect{
                      .effect_id = EffectId{"fx_shadow"},
                      .parameters = DropShadowEffect{
                          .offset_x = 4.0,
                          .offset_y = 8.0,
                          .sigma_x = 6.0,
                          .sigma_y = 6.0,
                          .color = ColorRgba8{.alpha = 160},
                      },
                  },
              },
          },
      },
  };
  const auto evaluated = evaluate_visual_layers(composition, 0, *layout);
  require(evaluated.size() == 1);
  require(evaluated.front().text_layout.has_value());
  require(evaluated.front().text_layout == first.result);
  require(evaluated.front().bounds.effect_local.left <
          evaluated.front().bounds.geometry_local.left);
  require(evaluated.front().bounds.world.left > 0.0);
}
