#include "refusion/core/ContentDigest.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("content digest test requirement failed");
  }
}

}  // namespace

int main() {
  using refusion::core::content_digest_matches;
  using refusion::core::sha256_content_digest;

  const std::vector<std::uint8_t> empty;
  require(sha256_content_digest(empty) ==
          "sha256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  const std::string abc = "abc";
  const std::vector<std::uint8_t> bytes(abc.begin(), abc.end());
  const std::string expected =
      "sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
  require(sha256_content_digest(bytes) == expected);
  require(content_digest_matches(bytes, expected));
  require(!content_digest_matches(
      bytes,
      "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
}
