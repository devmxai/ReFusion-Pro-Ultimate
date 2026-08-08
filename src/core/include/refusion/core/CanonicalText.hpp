#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace refusion::core {

// Canonical textual primitives shared by Project.rfx, Agent JSON and schema
// digests. They are deliberately independent of the process/C++ locale.
[[nodiscard]] std::string canonical_float64(double value);
// Compatibility spelling for the existing visual-registry digest contract.
// It intentionally mirrors the former six-decimal schema representation.
[[nodiscard]] std::string canonical_fixed6_float64(double value);
[[nodiscard]] std::string canonical_uint64(std::uint64_t value);
[[nodiscard]] std::string canonical_hex64(std::uint64_t value);

enum class Utf8ValidationStatus : std::uint8_t {
  valid,
  invalid_encoding,
  prohibited_control,
};

struct Utf8ValidationResult final {
  Utf8ValidationStatus status{Utf8ValidationStatus::valid};
  std::size_t byte_offset{0};

  [[nodiscard]] bool valid() const noexcept {
    return status == Utf8ValidationStatus::valid;
  }
};

// Project strings preserve their exact normalized-or-not UTF-8 bytes. Core
// validates encoding but never silently applies NFC/NFD. TAB/LF/CR are the
// only accepted control characters; NUL, C0/C1 controls and DEL are rejected.
[[nodiscard]] Utf8ValidationResult validate_preserved_utf8(
    std::string_view value) noexcept;

// IDs are case-sensitive ASCII, 1..128 bytes. They start with an ASCII letter
// or underscore and continue with letters, digits, underscore, dot or hyphen.
// Slashes, backslashes, whitespace and locale-dependent case folding are never
// legal identifier semantics.
[[nodiscard]] bool portable_ascii_identifier(std::string_view value) noexcept;

[[nodiscard]] bool ascii_space(unsigned char value) noexcept;
[[nodiscard]] char ascii_lower(char value) noexcept;

}  // namespace refusion::core
