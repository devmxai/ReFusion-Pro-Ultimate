#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace refusion::core {

// Returns the portable project spelling: "sha256:" followed by 64 lowercase
// hexadecimal digits. This implementation is dependency-free so project and
// asset admission use identical bytes on every supported toolchain.
[[nodiscard]] std::string sha256_content_digest(
    std::span<const std::uint8_t> bytes);

[[nodiscard]] bool content_digest_matches(
    std::span<const std::uint8_t> bytes,
    std::string_view expected_digest);

}  // namespace refusion::core
