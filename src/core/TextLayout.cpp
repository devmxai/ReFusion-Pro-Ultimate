#include "refusion/core/TextLayout.hpp"

#include "refusion/core/CanonicalText.hpp"

#include <bit>
#include <cstdint>
#include <string_view>

namespace refusion::core {
namespace {

void append_hash(std::uint64_t& hash, const std::string_view value) {
  for (const unsigned char byte : value) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  hash ^= 0xffU;
  hash *= 1099511628211ULL;
}

void append_number(std::uint64_t& hash, const double value) {
  const auto bits = std::bit_cast<std::uint64_t>(value);
  char bytes[sizeof(bits)];
  for (std::size_t index = 0; index < sizeof(bits); ++index) {
    bytes[index] = static_cast<char>((bits >> (index * 8U)) & 0xffU);
  }
  append_hash(hash, std::string_view{bytes, sizeof(bytes)});
}

}  // namespace

std::string text_layout_cache_key(const TextLayoutRequest& request,
                                  const std::string& layout_engine_digest) {
  std::uint64_t hash = 14695981039346656037ULL;
  const auto& text = request.text;
  append_hash(hash, layout_engine_digest);
  append_hash(hash, text.text);
  append_hash(hash, canonical_uint64(static_cast<unsigned>(text.font.source)));
  append_hash(hash, text.font.family_name);
  append_hash(hash, text.font.asset_id);
  append_hash(hash, text.font.content_digest);
  append_number(hash, text.font_size);
  append_number(hash, text.box.width);
  append_number(hash, text.box.height);
  append_number(hash, text.box.padding_top);
  append_number(hash, text.box.padding_right);
  append_number(hash, text.box.padding_bottom);
  append_number(hash, text.box.padding_left);
  append_hash(hash, canonical_uint64(static_cast<unsigned>(text.direction)));
  append_hash(hash,
              canonical_uint64(static_cast<unsigned>(
                  text.horizontal_alignment)));
  append_hash(hash,
              canonical_uint64(static_cast<unsigned>(
                  text.vertical_alignment)));
  append_hash(hash, canonical_uint64(static_cast<unsigned>(text.wrap)));
  append_hash(hash, canonical_uint64(static_cast<unsigned>(text.overflow)));
  append_number(hash, text.line_height_ratio);
  append_number(hash, text.letter_spacing);

  return "rfx-text-layout-fnv1a64:" + canonical_hex64(hash);
}

}  // namespace refusion::core
