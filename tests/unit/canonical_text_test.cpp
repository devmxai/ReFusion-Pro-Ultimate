#include "refusion/core/CanonicalText.hpp"

#include <limits>
#include <stdexcept>
#include <string>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("canonical text test requirement failed");
  }
}

}  // namespace

int main() {
  using namespace refusion::core;

  require(canonical_float64(0.0) == "0");
  require(canonical_float64(-0.0) == "0");
  require(canonical_float64(1.5) == "1.5");
  require(canonical_fixed6_float64(1.5) == "1.500000");
  require(canonical_uint64(std::numeric_limits<std::uint64_t>::max()) ==
          "18446744073709551615");
  require(canonical_hex64(0x0123456789abcdefULL) == "0123456789abcdef");

  bool rejected_non_finite = false;
  try {
    (void)canonical_float64(std::numeric_limits<double>::infinity());
  } catch (const std::invalid_argument&) {
    rejected_non_finite = true;
  }
  require(rejected_non_finite);

  const std::string arabic = "مرحباً ReFusion";
  require(validate_preserved_utf8(arabic).valid());
  require(validate_preserved_utf8("line one\nline two\tend").valid());

  // Both spellings remain valid and byte-distinct: Core validates UTF-8 but
  // does not silently normalize authored text.
  const std::string nfc = "é";
  const std::string nfd = "e\xCC\x81";
  require(validate_preserved_utf8(nfc).valid());
  require(validate_preserved_utf8(nfd).valid());
  require(nfc != nfd);

  const std::string overlong{"\xC0\xAF", 2};
  require(validate_preserved_utf8(overlong).status ==
          Utf8ValidationStatus::invalid_encoding);
  const std::string truncated{"\xE2\x82", 2};
  require(validate_preserved_utf8(truncated).status ==
          Utf8ValidationStatus::invalid_encoding);
  const std::string nul{"a\0b", 3};
  require(validate_preserved_utf8(nul).status ==
          Utf8ValidationStatus::prohibited_control);
  const std::string c1{"\xC2\x85", 2};
  require(validate_preserved_utf8(c1).status ==
          Utf8ValidationStatus::prohibited_control);

  require(portable_ascii_identifier("lyr_title-01.v2"));
  require(portable_ascii_identifier("_reserved"));
  require(!portable_ascii_identifier("1_layer"));
  require(!portable_ascii_identifier("layer/path"));
  require(!portable_ascii_identifier("طبقة"));

  require(ascii_space(' '));
  require(!ascii_space(0xa0U));
  require(ascii_lower('A') == 'a');
  require(ascii_lower('z') == 'z');
}
