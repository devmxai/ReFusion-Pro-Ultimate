#include "refusion/core/CanonicalText.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <system_error>

namespace refusion::core {
namespace {

[[nodiscard]] bool continuation(const unsigned char value) noexcept {
  return (value & 0xc0U) == 0x80U;
}

[[nodiscard]] bool prohibited_control(const std::uint32_t code_point) noexcept {
  if (code_point == 0x09U || code_point == 0x0aU || code_point == 0x0dU) {
    return false;
  }
  return code_point <= 0x1fU ||
         (code_point >= 0x7fU && code_point <= 0x9fU);
}

}  // namespace

std::string canonical_float64(const double value) {
  if (!std::isfinite(value)) {
    throw std::invalid_argument("canonical float64 requires a finite value");
  }

  // Collapse both IEEE zero encodings to one canonical spelling. The
  // max_digits10/general policy round-trips every finite binary64 value.
  const double normalized = value == 0.0 ? 0.0 : value;
  std::array<char, 128> buffer{};
  const auto converted = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), normalized,
      std::chars_format::general, std::numeric_limits<double>::max_digits10);
  if (converted.ec != std::errc{}) {
    throw std::runtime_error("canonical float64 conversion failed");
  }
  return std::string(buffer.data(), converted.ptr);
}

std::string canonical_fixed6_float64(const double value) {
  if (!std::isfinite(value)) {
    throw std::invalid_argument("canonical fixed float64 requires a finite value");
  }
  const double normalized = value == 0.0 ? 0.0 : value;
  std::array<char, 128> buffer{};
  const auto converted = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), normalized,
      std::chars_format::fixed, 6);
  if (converted.ec != std::errc{}) {
    throw std::runtime_error("canonical fixed float64 conversion failed");
  }
  return std::string(buffer.data(), converted.ptr);
}

std::string canonical_uint64(const std::uint64_t value) {
  std::array<char, 32> buffer{};
  const auto converted =
      std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, 10);
  if (converted.ec != std::errc{}) {
    throw std::runtime_error("canonical uint64 conversion failed");
  }
  return std::string(buffer.data(), converted.ptr);
}

std::string canonical_hex64(const std::uint64_t value) {
  constexpr char digits[] = "0123456789abcdef";
  std::string result(16, '0');
  for (std::size_t index = 0; index < result.size(); ++index) {
    const auto shift = static_cast<unsigned>((result.size() - index - 1) * 4U);
    result[index] = digits[(value >> shift) & 0x0fU];
  }
  return result;
}

Utf8ValidationResult validate_preserved_utf8(
    const std::string_view value) noexcept {
  std::size_t offset = 0;
  while (offset < value.size()) {
    const auto lead = static_cast<unsigned char>(value[offset]);
    std::uint32_t code_point = 0;
    std::size_t width = 0;

    if (lead <= 0x7fU) {
      code_point = lead;
      width = 1;
    } else if (lead >= 0xc2U && lead <= 0xdfU) {
      width = 2;
      if (offset + width > value.size() ||
          !continuation(static_cast<unsigned char>(value[offset + 1]))) {
        return {Utf8ValidationStatus::invalid_encoding, offset};
      }
      code_point = ((lead & 0x1fU) << 6U) |
                   (static_cast<unsigned char>(value[offset + 1]) & 0x3fU);
    } else if (lead >= 0xe0U && lead <= 0xefU) {
      width = 3;
      if (offset + width > value.size()) {
        return {Utf8ValidationStatus::invalid_encoding, offset};
      }
      const auto second = static_cast<unsigned char>(value[offset + 1]);
      const auto third = static_cast<unsigned char>(value[offset + 2]);
      if (!continuation(second) || !continuation(third) ||
          (lead == 0xe0U && second < 0xa0U) ||
          (lead == 0xedU && second > 0x9fU)) {
        return {Utf8ValidationStatus::invalid_encoding, offset};
      }
      code_point = ((lead & 0x0fU) << 12U) | ((second & 0x3fU) << 6U) |
                   (third & 0x3fU);
    } else if (lead >= 0xf0U && lead <= 0xf4U) {
      width = 4;
      if (offset + width > value.size()) {
        return {Utf8ValidationStatus::invalid_encoding, offset};
      }
      const auto second = static_cast<unsigned char>(value[offset + 1]);
      const auto third = static_cast<unsigned char>(value[offset + 2]);
      const auto fourth = static_cast<unsigned char>(value[offset + 3]);
      if (!continuation(second) || !continuation(third) ||
          !continuation(fourth) || (lead == 0xf0U && second < 0x90U) ||
          (lead == 0xf4U && second > 0x8fU)) {
        return {Utf8ValidationStatus::invalid_encoding, offset};
      }
      code_point = ((lead & 0x07U) << 18U) | ((second & 0x3fU) << 12U) |
                   ((third & 0x3fU) << 6U) | (fourth & 0x3fU);
    } else {
      return {Utf8ValidationStatus::invalid_encoding, offset};
    }

    if (prohibited_control(code_point)) {
      return {Utf8ValidationStatus::prohibited_control, offset};
    }
    offset += width;
  }
  return {};
}

bool portable_ascii_identifier(const std::string_view value) noexcept {
  if (value.empty() || value.size() > 128) {
    return false;
  }
  const auto ascii_alpha = [](const char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z');
  };
  if (!ascii_alpha(value.front()) && value.front() != '_') {
    return false;
  }
  for (const char character : value.substr(1)) {
    if (!ascii_alpha(character) &&
        !(character >= '0' && character <= '9') && character != '_' &&
        character != '.' && character != '-') {
      return false;
    }
  }
  return true;
}

bool ascii_space(const unsigned char value) noexcept {
  return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

char ascii_lower(const char value) noexcept {
  return value >= 'A' && value <= 'Z'
             ? static_cast<char>(value + ('a' - 'A'))
             : value;
}

}  // namespace refusion::core
