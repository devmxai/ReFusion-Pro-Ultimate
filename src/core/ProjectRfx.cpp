#include "refusion/core/ProjectRfx.hpp"

#include "refusion/core/CanonicalText.hpp"
#include "refusion/core/ProjectClock.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"
#include "refusion/core/VisualPropertyRegistry.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace refusion::core {
namespace {

constexpr std::uint64_t kNanosecondsPerSecond = 1'000'000'000;

enum class TokenKind : std::uint8_t {
  identifier,
  string_literal,
  number,
  left_parenthesis,
  right_parenthesis,
  left_brace,
  right_brace,
  comma,
  semicolon,
  dot,
  end_of_file,
};

struct Token final {
  TokenKind kind{TokenKind::end_of_file};
  std::string text;
  RfxSourceLocation location;
};

class DiagnosticFailure final : public std::exception {
 public:
  explicit DiagnosticFailure(RfxDiagnostic diagnostic)
      : diagnostic_(std::move(diagnostic)) {}

  [[nodiscard]] const char* what() const noexcept override {
    return diagnostic_.message.c_str();
  }

  [[nodiscard]] RfxDiagnostic take() noexcept {
    return std::move(diagnostic_);
  }

 private:
  RfxDiagnostic diagnostic_;
};

[[noreturn]] void fail(std::string code,
                       std::string message,
                       const RfxSourceLocation location) {
  throw DiagnosticFailure(RfxDiagnostic{
      .code = std::move(code),
      .message = std::move(message),
      .location = location,
  });
}

[[nodiscard]] bool is_identifier_start(const char character) noexcept {
  return (character >= 'a' && character <= 'z') ||
         (character >= 'A' && character <= 'Z') || character == '_';
}

[[nodiscard]] bool is_identifier_continue(const char character) noexcept {
  return is_identifier_start(character) ||
         (character >= '0' && character <= '9');
}

[[nodiscard]] bool is_digit(const char character) noexcept {
  return character >= '0' && character <= '9';
}

class Lexer final {
 public:
  explicit Lexer(const std::string_view source) : source_(source) {}

  [[nodiscard]] Token next() {
    skip_whitespace();
    const auto location = current_location();
    if (offset_ == source_.size()) {
      return Token{.kind = TokenKind::end_of_file, .location = location};
    }

    const char character = source_[offset_];
    if (is_identifier_start(character)) {
      return lex_identifier(location);
    }
    if (character == '"') {
      return lex_string(location);
    }
    if (is_digit(character) || character == '-') {
      return lex_number(location);
    }

    advance();
    switch (character) {
      case '(':
        return punctuation(TokenKind::left_parenthesis, "(", location);
      case ')':
        return punctuation(TokenKind::right_parenthesis, ")", location);
      case '{':
        return punctuation(TokenKind::left_brace, "{", location);
      case '}':
        return punctuation(TokenKind::right_brace, "}", location);
      case ',':
        return punctuation(TokenKind::comma, ",", location);
      case ';':
        return punctuation(TokenKind::semicolon, ";", location);
      case '.':
        return punctuation(TokenKind::dot, ".", location);
      default:
        fail("RFX-RFX-LEX-001",
             "unexpected character in Project.rfx",
             location);
    }
  }

 private:
  [[nodiscard]] static Token punctuation(const TokenKind kind,
                                         std::string text,
                                         const RfxSourceLocation location) {
    return Token{.kind = kind, .text = std::move(text), .location = location};
  }

  [[nodiscard]] RfxSourceLocation current_location() const noexcept {
    return RfxSourceLocation{
        .byte_offset = offset_,
        .line = line_,
        .column = column_,
    };
  }

  void advance() noexcept {
    if (offset_ >= source_.size()) {
      return;
    }
    if (source_[offset_] == '\n') {
      ++line_;
      column_ = 1;
    } else {
      ++column_;
    }
    ++offset_;
  }

  void skip_whitespace() noexcept {
    while (offset_ < source_.size()) {
      const char character = source_[offset_];
      if (character != ' ' && character != '\t' && character != '\r' &&
          character != '\n') {
        return;
      }
      advance();
    }
  }

  [[nodiscard]] Token lex_identifier(const RfxSourceLocation location) {
    const auto begin = offset_;
    while (offset_ < source_.size() &&
           is_identifier_continue(source_[offset_])) {
      advance();
    }
    return Token{
        .kind = TokenKind::identifier,
        .text = std::string(source_.substr(begin, offset_ - begin)),
        .location = location,
    };
  }

  [[nodiscard]] Token lex_string(const RfxSourceLocation location) {
    advance();
    std::string decoded;
    while (offset_ < source_.size()) {
      const char character = source_[offset_];
      if (character == '"') {
        const auto utf8 = validate_preserved_utf8(decoded);
        if (!utf8.valid()) {
          fail(utf8.status == Utf8ValidationStatus::invalid_encoding
                   ? "RFX-RFX-UTF8-001"
                   : "RFX-RFX-UTF8-002",
               utf8.status == Utf8ValidationStatus::invalid_encoding
                   ? "string literal is not valid UTF-8"
                   : "string literal contains a prohibited control character",
               location);
        }
        advance();
        return Token{
            .kind = TokenKind::string_literal,
            .text = std::move(decoded),
            .location = location,
        };
      }
      if (character == '\n' || character == '\r') {
        fail("RFX-RFX-LEX-002",
             "string literal must not contain an unescaped newline",
             current_location());
      }
      if (character != '\\') {
        decoded.push_back(character);
        advance();
        continue;
      }

      const auto escape_location = current_location();
      advance();
      if (offset_ == source_.size()) {
        fail("RFX-RFX-LEX-003", "unterminated string escape", escape_location);
      }
      const char escaped = source_[offset_];
      switch (escaped) {
        case '"':
          decoded.push_back('"');
          break;
        case '\\':
          decoded.push_back('\\');
          break;
        case 'n':
          decoded.push_back('\n');
          break;
        case 'r':
          decoded.push_back('\r');
          break;
        case 't':
          decoded.push_back('\t');
          break;
        default:
          fail("RFX-RFX-LEX-004", "unsupported string escape", escape_location);
      }
      advance();
    }
    fail("RFX-RFX-LEX-005", "unterminated string literal", location);
  }

  [[nodiscard]] Token lex_number(const RfxSourceLocation location) {
    const auto begin = offset_;
    if (source_[offset_] == '-') {
      advance();
      if (offset_ == source_.size() || !is_digit(source_[offset_])) {
        fail("RFX-RFX-LEX-006", "minus must be followed by digits", location);
      }
    }
    while (offset_ < source_.size() && is_digit(source_[offset_])) {
      advance();
    }
    if (offset_ < source_.size() && source_[offset_] == '.') {
      advance();
      if (offset_ == source_.size() || !is_digit(source_[offset_])) {
        fail("RFX-RFX-LEX-007",
             "decimal point must be followed by digits",
             location);
      }
      while (offset_ < source_.size() && is_digit(source_[offset_])) {
        advance();
      }
    }
    if (offset_ < source_.size() &&
        (source_[offset_] == 'e' || source_[offset_] == 'E')) {
      advance();
      if (offset_ < source_.size() &&
          (source_[offset_] == '+' || source_[offset_] == '-')) {
        advance();
      }
      if (offset_ == source_.size() || !is_digit(source_[offset_])) {
        fail("RFX-RFX-LEX-008",
             "decimal exponent must contain digits",
             location);
      }
      while (offset_ < source_.size() && is_digit(source_[offset_])) {
        advance();
      }
    }
    return Token{
        .kind = TokenKind::number,
        .text = std::string(source_.substr(begin, offset_ - begin)),
        .location = location,
    };
  }

  std::string_view source_;
  std::size_t offset_{0};
  std::size_t line_{1};
  std::size_t column_{1};
};

[[nodiscard]] bool blank(const std::string& value) noexcept {
  return value.empty() ||
         std::all_of(value.begin(), value.end(), [](const unsigned char character) {
           return character == ' ' || character == '\t' || character == '\r' ||
                  character == '\n';
         });
}

[[nodiscard]] bool multiply_fits(const std::uint64_t lhs,
                                 const std::uint64_t rhs) noexcept {
  return lhs == 0 || rhs <= std::numeric_limits<std::uint64_t>::max() / lhs;
}

[[nodiscard]] ProjectTimeNs frame_to_time(const std::uint64_t frame,
                                          const RationalRate rate,
                                          const RfxSourceLocation location) {
  const auto denominator = static_cast<std::uint64_t>(rate.denominator);
  if (!rate.valid() || !multiply_fits(frame, denominator)) {
    fail("RFX-RFX-TIME-001", "frame index cannot be represented", location);
  }
  const auto scaled_frame = frame * denominator;
  if (!multiply_fits(scaled_frame, kNanosecondsPerSecond)) {
    fail("RFX-RFX-TIME-001", "frame index cannot be represented", location);
  }
  const auto numerator = static_cast<std::uint64_t>(rate.numerator);
  const auto scaled_time = scaled_frame * kNanosecondsPerSecond;
  return scaled_time / numerator +
         static_cast<std::uint64_t>(scaled_time % numerator != 0);
}

[[nodiscard]] std::uint64_t add_frames(const std::uint64_t lhs,
                                       const std::uint64_t rhs,
                                       const RfxSourceLocation location) {
  if (rhs > std::numeric_limits<std::uint64_t>::max() - lhs) {
    fail("RFX-RFX-TIME-002", "frame range overflows", location);
  }
  return lhs + rhs;
}

[[nodiscard]] std::uint8_t hex_digit(const char value,
                                     const RfxSourceLocation location) {
  if (value >= '0' && value <= '9') {
    return static_cast<std::uint8_t>(value - '0');
  }
  if (value >= 'a' && value <= 'f') {
    return static_cast<std::uint8_t>(10 + value - 'a');
  }
  if (value >= 'A' && value <= 'F') {
    return static_cast<std::uint8_t>(10 + value - 'A');
  }
  fail("RFX-RFX-COLOR-001", "rgba8 must contain hexadecimal digits", location);
}

[[nodiscard]] ColorRgba8 parse_color(const std::string& value,
                                     const RfxSourceLocation location) {
  if (value.size() != 9 || value.front() != '#') {
    fail("RFX-RFX-COLOR-001", "rgba8 must use #RRGGBBAA", location);
  }
  const auto channel = [&value, location](const std::size_t offset) {
    return static_cast<std::uint8_t>(
        (hex_digit(value[offset], location) << 4U) |
        hex_digit(value[offset + 1], location));
  };
  return ColorRgba8{
      .red = channel(1),
      .green = channel(3),
      .blue = channel(5),
      .alpha = channel(7),
  };
}

class Parser final {
 public:
  explicit Parser(const std::string_view source) : lexer_(source) {
    current_ = lexer_.next();
  }

  [[nodiscard]] ProjectSnapshot parse() {
    expect_identifier("rfx");
    const auto version_location = current_.location;
    const auto version = parse_unsigned_integer();
    if (version < 1 || version > 6) {
      fail("RFX-RFX-VERSION-001",
           "only Project.rfx language versions 1 through 6 are supported",
           version_location);
    }
    language_version_ = version;
    expect(TokenKind::semicolon, "';'");

    if (language_version_ >= 4) {
      expect_identifier("registry");
      expect_identifier("digest");
      const auto digest_location = current_.location;
      const auto digest = parse_call_string();
      expect(TokenKind::semicolon, "';'");
      if (digest != visual_property_registry_digest()) {
        fail("RFX-RFX-REGISTRY-001",
             "Project.rfx registry digest does not match this engine",
             digest_location);
      }
    }
    if (language_version_ >= 5) {
      expect_identifier("contributions");
      expect_identifier("digest");
      const auto digest_location = current_.location;
      const auto digest = parse_call_string();
      expect(TokenKind::semicolon, "';'");
      if (digest != visual_contribution_registry_digest()) {
        fail("RFX-RFX-CONTRIBUTION-REGISTRY-001",
             "Project.rfx contribution digest does not match this engine",
             digest_location);
      }
    }

    expect_identifier("project");
    expect_identifier("id");
    ProjectSnapshot project;
    project.project_id = ProjectId{parse_call_string()};
    expect_identifier("revision");
    project.revision_id = RevisionId{parse_call_integer()};
    expect_identifier("name");
    project.display_name = parse_call_string();
    expect(TokenKind::semicolon, "';'");

    if (language_version_ >= 6) {
      project.assets = parse_assets();
      project.media_sources = parse_media_sources();
      project.linked_imports = parse_linked_imports();
    }

    const auto composition_location = current_.location;
    project.composition = parse_composition();
    expect(TokenKind::end_of_file, "end of file");

    if (!portable_ascii_identifier(project.project_id.value)) {
      fail("RFX-RFX-PROJECT-001",
           "project ID must be a portable ASCII identifier",
           composition_location);
    }
    if (project.revision_id.value == 0) {
      fail("RFX-RFX-PROJECT-002",
           "project revision must be greater than zero",
           composition_location);
    }
    if (blank(project.display_name)) {
      fail("RFX-RFX-PROJECT-003",
           "project name is required",
           composition_location);
    }
    const auto validation = validate_project(project);
    if (!validation.valid) {
      fail(validation.code, validation.message, composition_location);
    }
    return project;
  }

 private:
  struct IntegerPair final {
    std::uint64_t first{0};
    std::uint64_t second{0};
  };

  struct SignedIntegerPair final {
    std::int64_t first{0};
    std::int64_t second{0};
  };

  struct SignedUnsignedPair final {
    std::int64_t first{0};
    std::uint64_t second{0};
  };

  struct NumberPair final {
    double first{0.0};
    double second{0.0};
  };

  struct NumberQuad final {
    double first{0.0};
    double second{0.0};
    double third{0.0};
    double fourth{0.0};
  };

  void advance() { current_ = lexer_.next(); }

  void expect(const TokenKind kind, const std::string_view spelling) {
    if (current_.kind != kind) {
      fail("RFX-RFX-PARSE-001",
           "expected " + std::string(spelling) + ", found '" + current_.text +
               "'",
           current_.location);
    }
    advance();
  }

  void expect_identifier(const std::string_view spelling) {
    if (current_.kind != TokenKind::identifier || current_.text != spelling) {
      fail("RFX-RFX-PARSE-001",
           "expected '" + std::string(spelling) + "', found '" + current_.text +
               "'",
           current_.location);
    }
    advance();
  }

  [[nodiscard]] bool is_identifier(const std::string_view spelling) const {
    return current_.kind == TokenKind::identifier && current_.text == spelling;
  }

  [[nodiscard]] std::string parse_string() {
    if (current_.kind != TokenKind::string_literal) {
      fail("RFX-RFX-PARSE-001", "expected string literal", current_.location);
    }
    auto value = std::move(current_.text);
    advance();
    return value;
  }

  [[nodiscard]] std::uint64_t parse_unsigned_integer() {
    const auto location = current_.location;
    if (current_.kind != TokenKind::number || current_.text.empty() ||
        current_.text.front() == '-' || current_.text.find('.') != std::string::npos) {
      fail("RFX-RFX-NUMBER-001", "expected non-negative integer", location);
    }
    std::uint64_t value = 0;
    const auto* begin = current_.text.data();
    const auto* end = begin + current_.text.size();
    const auto parsed = std::from_chars(begin, end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
      fail("RFX-RFX-NUMBER-002", "integer is outside uint64 range", location);
    }
    advance();
    return value;
  }

  [[nodiscard]] std::int64_t parse_signed_integer() {
    const auto location = current_.location;
    if (current_.kind != TokenKind::number || current_.text.empty() ||
        current_.text.find('.') != std::string::npos) {
      fail("RFX-RFX-NUMBER-004", "expected signed integer", location);
    }
    std::int64_t value = 0;
    const auto* begin = current_.text.data();
    const auto* end = begin + current_.text.size();
    const auto parsed = std::from_chars(begin, end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
      fail("RFX-RFX-NUMBER-005", "integer is outside int64 range", location);
    }
    advance();
    return value;
  }

  [[nodiscard]] double parse_number() {
    const auto location = current_.location;
    if (current_.kind != TokenKind::number) {
      fail("RFX-RFX-NUMBER-003", "expected finite number", location);
    }
    // Floating-point from_chars is not available on every admitted mobile
    // deployment target. A classic-locale, no-skip stream keeps the decimal
    // contract platform-neutral without falling back to the host locale.
    std::istringstream input(current_.text);
    input.imbue(std::locale::classic());
    input >> std::noskipws;
    double value = 0.0;
    input >> value;
    if (!input || input.peek() != std::char_traits<char>::eof() ||
        !std::isfinite(value)) {
      fail("RFX-RFX-NUMBER-003", "expected finite number", location);
    }
    advance();
    return value;
  }

  [[nodiscard]] std::string parse_call_string() {
    expect(TokenKind::left_parenthesis, "'('");
    auto value = parse_string();
    expect(TokenKind::right_parenthesis, "')'");
    return value;
  }

  [[nodiscard]] std::string parse_call_identifier() {
    expect(TokenKind::left_parenthesis, "'('");
    if (current_.kind != TokenKind::identifier) {
      fail("RFX-RFX-PARSE-001", "expected identifier", current_.location);
    }
    auto value = current_.text;
    advance();
    expect(TokenKind::right_parenthesis, "')'");
    return value;
  }

  [[nodiscard]] std::uint64_t parse_call_integer() {
    expect(TokenKind::left_parenthesis, "'('");
    const auto value = parse_unsigned_integer();
    expect(TokenKind::right_parenthesis, "')'");
    return value;
  }

  [[nodiscard]] std::uint32_t parse_call_uint32() {
    const auto location = current_.location;
    const auto value = parse_call_integer();
    if (value > std::numeric_limits<std::uint32_t>::max()) {
      fail("RFX-RFX-NUMBER-006", "integer exceeds uint32", location);
    }
    return static_cast<std::uint32_t>(value);
  }

  [[nodiscard]] std::int64_t parse_call_signed_integer() {
    expect(TokenKind::left_parenthesis, "'('");
    const auto value = parse_signed_integer();
    expect(TokenKind::right_parenthesis, "')'");
    return value;
  }

  [[nodiscard]] double parse_call_number() {
    expect(TokenKind::left_parenthesis, "'('");
    const auto value = parse_number();
    expect(TokenKind::right_parenthesis, "')'");
    return value;
  }

  [[nodiscard]] bool parse_call_boolean() {
    expect(TokenKind::left_parenthesis, "'('");
    if (!is_identifier("true") && !is_identifier("false")) {
      fail("RFX-RFX-BOOLEAN-001", "expected true or false", current_.location);
    }
    const bool value = is_identifier("true");
    advance();
    expect(TokenKind::right_parenthesis, "')'");
    return value;
  }

  [[nodiscard]] IntegerPair parse_call_integer_pair() {
    expect(TokenKind::left_parenthesis, "'('");
    const auto first = parse_unsigned_integer();
    expect(TokenKind::comma, "','");
    const auto second = parse_unsigned_integer();
    expect(TokenKind::right_parenthesis, "')'");
    return IntegerPair{.first = first, .second = second};
  }

  [[nodiscard]] SignedIntegerPair parse_call_signed_integer_pair() {
    expect(TokenKind::left_parenthesis, "'('");
    const auto first = parse_signed_integer();
    expect(TokenKind::comma, "','");
    const auto second = parse_signed_integer();
    expect(TokenKind::right_parenthesis, "')'");
    return SignedIntegerPair{.first = first, .second = second};
  }

  [[nodiscard]] SignedUnsignedPair parse_call_signed_unsigned_pair() {
    expect(TokenKind::left_parenthesis, "'('");
    const auto first = parse_signed_integer();
    expect(TokenKind::comma, "','");
    const auto second = parse_unsigned_integer();
    expect(TokenKind::right_parenthesis, "')'");
    return SignedUnsignedPair{.first = first, .second = second};
  }

  [[nodiscard]] NumberPair parse_call_number_pair() {
    expect(TokenKind::left_parenthesis, "'('");
    const auto first = parse_number();
    expect(TokenKind::comma, "','");
    const auto second = parse_number();
    expect(TokenKind::right_parenthesis, "')'");
    return NumberPair{.first = first, .second = second};
  }

  [[nodiscard]] NumberQuad parse_call_number_quad() {
    expect(TokenKind::left_parenthesis, "'('");
    const auto first = parse_number();
    expect(TokenKind::comma, "','");
    const auto second = parse_number();
    expect(TokenKind::comma, "','");
    const auto third = parse_number();
    expect(TokenKind::comma, "','");
    const auto fourth = parse_number();
    expect(TokenKind::right_parenthesis, "')'");
    return NumberQuad{
        .first = first,
        .second = second,
        .third = third,
        .fourth = fourth,
    };
  }

  [[nodiscard]] std::vector<AssetRecord> parse_assets() {
    expect_identifier("assets");
    expect(TokenKind::left_brace, "'{'");
    std::vector<AssetRecord> assets;
    while (is_identifier("asset")) {
      expect_identifier("asset");
      expect_identifier("id");
      AssetRecord asset;
      asset.asset_id = AssetId{parse_call_string()};
      expect_identifier("digest");
      asset.content_digest = parse_call_string();
      expect_identifier("bytes");
      asset.byte_size = parse_call_integer();
      expect_identifier("kind");
      if (parse_call_identifier() != "video_container") {
        fail("RFX-RFX-MEDIA-001", "asset kind must be video_container",
             current_.location);
      }
      expect_identifier("original");
      asset.project_relative_original = parse_call_string();
      expect_identifier("name");
      asset.original_display_name = parse_call_string();
      expect_identifier("provenance");
      if (parse_call_identifier() != "imported_copy") {
        fail("RFX-RFX-MEDIA-002", "asset provenance must be imported_copy",
             current_.location);
      }
      expect(TokenKind::semicolon, "';'");
      assets.push_back(std::move(asset));
    }
    expect(TokenKind::right_brace, "'}'");
    return assets;
  }

  [[nodiscard]] MediaResolutionState parse_media_resolution() {
    const auto location = current_.location;
    const auto value = parse_call_identifier();
    if (value == "resolved") return MediaResolutionState::resolved;
    if (value == "missing") return MediaResolutionState::missing;
    if (value == "digest_mismatch") return MediaResolutionState::digest_mismatch;
    if (value == "unsupported") return MediaResolutionState::unsupported;
    fail("RFX-RFX-MEDIA-003", "invalid MediaSource resolution state", location);
  }

  [[nodiscard]] MediaStreamDescriptor parse_media_stream() {
    expect_identifier("stream");
    const auto kind_location = current_.location;
    MediaStreamDescriptor stream;
    if (is_identifier("video")) {
      stream.kind = MediaStreamKind::video;
      advance();
    } else if (is_identifier("audio")) {
      stream.kind = MediaStreamKind::audio;
      advance();
    } else {
      fail("RFX-RFX-MEDIA-004", "stream kind must be video or audio",
           kind_location);
    }
    expect_identifier("id");
    stream.stream_id = MediaStreamId{parse_call_string()};
    expect_identifier("track");
    stream.container_track_id = parse_call_uint32();
    expect_identifier("codec");
    const auto codec_location = current_.location;
    const auto codec = parse_call_identifier();
    if (codec == "h264_avc") {
      stream.codec = MediaCodec::h264_avc;
    } else if (codec == "aac_lc") {
      stream.codec = MediaCodec::aac_lc;
    } else {
      fail("RFX-RFX-MEDIA-005", "codec must be h264_avc or aac_lc",
           codec_location);
    }
    expect_identifier("config_digest");
    stream.codec_configuration_digest = parse_call_string();
    expect_identifier("time_base");
    const auto time_base = parse_call_signed_integer_pair();
    stream.time_base = MediaTimeBase{
        .numerator = time_base.first,
        .denominator = time_base.second,
    };
    expect_identifier("start");
    stream.start = parse_call_signed_integer();
    expect_identifier("duration");
    stream.duration = parse_call_integer();
    expect(TokenKind::left_brace, "'{'");

    if (stream.kind == MediaStreamKind::video) {
      VideoStreamFormat format;
      expect_identifier("coded_px");
      const auto coded = parse_call_integer_pair();
      expect(TokenKind::semicolon, "';'");
      expect_identifier("display_px");
      const auto display = parse_call_integer_pair();
      expect(TokenKind::semicolon, "';'");
      expect_identifier("rate");
      const auto rate = parse_call_integer_pair();
      expect(TokenKind::semicolon, "';'");
      if (coded.first > std::numeric_limits<std::uint32_t>::max() ||
          coded.second > std::numeric_limits<std::uint32_t>::max() ||
          display.first > std::numeric_limits<std::uint32_t>::max() ||
          display.second > std::numeric_limits<std::uint32_t>::max() ||
          rate.first > std::numeric_limits<std::uint32_t>::max() ||
          rate.second > std::numeric_limits<std::uint32_t>::max()) {
        fail("RFX-RFX-MEDIA-006", "video dimensions or rate exceed uint32",
             kind_location);
      }
      format.coded_extent = CanvasExtent{
          .width_pixels = static_cast<std::uint32_t>(coded.first),
          .height_pixels = static_cast<std::uint32_t>(coded.second),
      };
      format.display_extent = CanvasExtent{
          .width_pixels = static_cast<std::uint32_t>(display.first),
          .height_pixels = static_cast<std::uint32_t>(display.second),
      };
      format.presentation_rate = RationalRate{
          .numerator = static_cast<std::uint32_t>(rate.first),
          .denominator = static_cast<std::uint32_t>(rate.second),
      };
      expect_identifier("bit_depth");
      const auto bit_depth = parse_call_integer();
      expect(TokenKind::semicolon, "';'");
      expect_identifier("chroma");
      const auto chroma = parse_call_integer_pair();
      expect(TokenKind::semicolon, "';'");
      if (bit_depth > std::numeric_limits<std::uint8_t>::max() ||
          chroma.first > std::numeric_limits<std::uint8_t>::max() ||
          chroma.second > std::numeric_limits<std::uint8_t>::max()) {
        fail("RFX-RFX-MEDIA-007", "video format values exceed uint8",
             kind_location);
      }
      format.bit_depth = static_cast<std::uint8_t>(bit_depth);
      format.chroma_subsampling_x = static_cast<std::uint8_t>(chroma.first);
      format.chroma_subsampling_y = static_cast<std::uint8_t>(chroma.second);
      expect_identifier("color_range");
      if (parse_call_identifier() != "video") {
        fail("RFX-RFX-MEDIA-008", "color range must be video",
             current_.location);
      }
      expect(TokenKind::semicolon, "';'");
      expect_identifier("primaries");
      format.color_primaries = parse_call_string();
      expect(TokenKind::semicolon, "';'");
      expect_identifier("transfer");
      format.color_transfer = parse_call_string();
      expect(TokenKind::semicolon, "';'");
      expect_identifier("matrix");
      format.color_matrix = parse_call_string();
      expect(TokenKind::semicolon, "';'");
      expect_identifier("orientation");
      const auto orientation = parse_call_signed_integer();
      expect(TokenKind::semicolon, "';'");
      if (orientation < std::numeric_limits<std::int16_t>::min() ||
          orientation > std::numeric_limits<std::int16_t>::max()) {
        fail("RFX-RFX-MEDIA-009", "orientation exceeds int16", kind_location);
      }
      format.orientation_degrees = static_cast<std::int16_t>(orientation);
      expect_identifier("sample_aspect");
      const auto aspect = parse_call_integer_pair();
      expect(TokenKind::semicolon, "';'");
      if (aspect.first > std::numeric_limits<std::uint32_t>::max() ||
          aspect.second > std::numeric_limits<std::uint32_t>::max()) {
        fail("RFX-RFX-MEDIA-010", "sample aspect exceeds uint32", kind_location);
      }
      format.sample_aspect_numerator = static_cast<std::uint32_t>(aspect.first);
      format.sample_aspect_denominator = static_cast<std::uint32_t>(aspect.second);
      stream.format = std::move(format);
    } else {
      AudioStreamFormat format;
      expect_identifier("sample_rate");
      format.sample_rate_hz = parse_call_uint32();
      expect(TokenKind::semicolon, "';'");
      expect_identifier("channels");
      const auto channels = parse_call_integer();
      expect(TokenKind::semicolon, "';'");
      if (channels > std::numeric_limits<std::uint8_t>::max()) {
        fail("RFX-RFX-MEDIA-011", "channel count exceeds uint8", kind_location);
      }
      format.channels = static_cast<std::uint8_t>(channels);
      stream.format = std::move(format);
    }
    expect(TokenKind::right_brace, "'}'");
    return stream;
  }

  [[nodiscard]] std::vector<MediaSource> parse_media_sources() {
    expect_identifier("media_sources");
    expect(TokenKind::left_brace, "'{'");
    std::vector<MediaSource> sources;
    while (is_identifier("media_source")) {
      expect_identifier("media_source");
      expect_identifier("id");
      MediaSource source;
      source.media_source_id = MediaSourceId{parse_call_string()};
      expect_identifier("asset");
      source.asset_id = AssetId{parse_call_string()};
      expect_identifier("index_version");
      source.media_index_contract_version = parse_call_uint32();
      expect_identifier("index_digest");
      source.media_index_digest = parse_call_string();
      expect_identifier("resolution");
      source.resolution = parse_media_resolution();
      expect(TokenKind::left_brace, "'{'");
      while (is_identifier("stream")) {
        source.streams.push_back(parse_media_stream());
      }
      expect_identifier("selected_video");
      source.selected_video_stream = MediaStreamId{parse_call_string()};
      expect(TokenKind::semicolon, "';'");
      if (is_identifier("selected_audio")) {
        expect_identifier("selected_audio");
        source.selected_audio_stream = MediaStreamId{parse_call_string()};
        expect(TokenKind::semicolon, "';'");
      }
      expect(TokenKind::right_brace, "'}'");
      sources.push_back(std::move(source));
    }
    expect(TokenKind::right_brace, "'}'");
    return sources;
  }

  [[nodiscard]] std::vector<LinkedImport> parse_linked_imports() {
    expect_identifier("linked_imports");
    expect(TokenKind::left_brace, "'{'");
    std::vector<LinkedImport> links;
    while (is_identifier("linked_import")) {
      expect_identifier("linked_import");
      expect_identifier("id");
      LinkedImport link;
      link.linked_import_id = LinkedImportId{parse_call_string()};
      expect_identifier("source");
      link.media_source_id = MediaSourceId{parse_call_string()};
      expect(TokenKind::left_brace, "'{'");
      if (is_identifier("video_clip")) {
        expect_identifier("video_clip");
        link.video_clip_id = VideoClipId{parse_call_string()};
        expect(TokenKind::semicolon, "';'");
      }
      if (is_identifier("audio_clip")) {
        expect_identifier("audio_clip");
        link.audio_clip_id = AudioClipId{parse_call_string()};
        expect(TokenKind::semicolon, "';'");
      }
      expect(TokenKind::right_brace, "'}'");
      links.push_back(std::move(link));
    }
    expect(TokenKind::right_brace, "'}'");
    return links;
  }

  [[nodiscard]] CompositionSnapshot parse_composition() {
    expect_identifier("composition");
    expect_identifier("id");
    CompositionSnapshot composition;
    composition.composition_id = CompositionId{parse_call_string()};
    expect_identifier("name");
    composition.display_name = parse_call_string();
    expect(TokenKind::left_brace, "'{'");

    expect_identifier("canvas");
    expect_identifier("px");
    const auto canvas_location = current_.location;
    const auto canvas = parse_call_integer_pair();
    if (canvas.first > std::numeric_limits<std::uint32_t>::max() ||
        canvas.second > std::numeric_limits<std::uint32_t>::max()) {
      fail("RFX-RFX-CANVAS-001", "canvas dimensions exceed uint32", canvas_location);
    }
    composition.canvas = CanvasExtent{
        .width_pixels = static_cast<std::uint32_t>(canvas.first),
        .height_pixels = static_cast<std::uint32_t>(canvas.second),
    };
    expect(TokenKind::semicolon, "';'");

    expect_identifier("frame_rate");
    expect_identifier("rational");
    const auto rate_location = current_.location;
    const auto rate = parse_call_integer_pair();
    if (rate.first == 0 || rate.second == 0 ||
        rate.first > std::numeric_limits<std::uint32_t>::max() ||
        rate.second > std::numeric_limits<std::uint32_t>::max()) {
      fail("RFX-RFX-RATE-001", "frame rate must fit a positive uint32 ratio", rate_location);
    }
    composition.frame_rate = RationalRate{
        .numerator = static_cast<std::uint32_t>(rate.first),
        .denominator = static_cast<std::uint32_t>(rate.second),
    };
    expect(TokenKind::semicolon, "';'");

    expect_identifier("duration");
    expect_identifier("frames");
    const auto duration_location = current_.location;
    const auto duration_frames = parse_call_integer();
    composition.duration = frame_to_time(
        duration_frames, composition.frame_rate, duration_location);
    expect(TokenKind::semicolon, "';'");

    std::unordered_set<std::string> visual_ids;
    while (is_identifier("layer") || is_identifier("group")) {
      const auto node_location = current_.location;
      if (is_identifier("layer")) {
        auto layer = parse_layer(composition.frame_rate);
        if (!visual_ids.emplace(layer.layer_id.value).second) {
          fail("RFX-PROJECT-107",
               "visual IDs must be non-empty and globally unique",
               node_location);
        }
        auto single_layer_composition = composition;
        single_layer_composition.layers = {layer};
        const auto layer_validation =
            validate_composition(single_layer_composition);
        if (!layer_validation.valid) {
          fail(layer_validation.code,
               layer_validation.message,
               node_location);
        }
        composition.layers.push_back(std::move(layer));
      } else {
        if (language_version_ < 2) {
          fail("RFX-RFX-VERSION-002",
               "groups require Project.rfx language version 2",
               node_location);
        }
        auto group = parse_group(composition.frame_rate);
        if (!visual_ids.emplace(group.group_id.value).second) {
          fail("RFX-PROJECT-116",
               "visual IDs must be non-empty and globally unique",
               node_location);
        }
        composition.groups.push_back(std::move(group));
      }
    }
    while (is_identifier("video_clip") || is_identifier("audio_clip")) {
      const auto clip_location = current_.location;
      if (language_version_ < 6) {
        fail("RFX-RFX-VERSION-012",
             "media clips require Project.rfx language version 6",
             clip_location);
      }
      if (is_identifier("video_clip")) {
        composition.video_clips.push_back(
            parse_video_clip(composition.frame_rate));
      } else {
        composition.audio_clips.push_back(
            parse_audio_clip(composition.frame_rate));
      }
    }
    if (language_version_ >= 2) {
      composition.root_nodes = parse_visual_ref_block("root");
    } else {
      composition.root_nodes.reserve(composition.layers.size());
      for (const auto& layer : composition.layers) {
        composition.root_nodes.emplace_back(layer.layer_id);
      }
    }
    expect(TokenKind::right_brace, "'}'");
    return composition;
  }

  [[nodiscard]] VideoClipSnapshot parse_video_clip(
      const RationalRate rate) {
    expect_identifier("video_clip");
    expect_identifier("id");
    VideoClipSnapshot clip;
    clip.video_clip_id = VideoClipId{parse_call_string()};
    expect_identifier("link");
    clip.linked_import_id = LinkedImportId{parse_call_string()};
    expect_identifier("source");
    clip.media_source_id = MediaSourceId{parse_call_string()};
    expect_identifier("stream");
    clip.stream_id = MediaStreamId{parse_call_string()};
    expect_identifier("name");
    clip.display_name = parse_call_string();
    expect(TokenKind::left_brace, "'{'");

    expect_identifier("range");
    const auto range_location = current_.location;
    if (is_identifier("ns")) {
      advance();
      const auto range = parse_call_integer_pair();
      clip.active_range =
          TimeRangeNs{.start = range.first, .duration = range.second};
    } else {
      // Read the initial experimental RFX6 frame spelling as a migration
      // input. Canonical RFX6 always writes nanoseconds so non-frame-aligned
      // A/V offsets survive save/reopen exactly.
      expect_identifier("frames");
      const auto range = parse_call_integer_pair();
      const auto end_frame =
          add_frames(range.first, range.second, range_location);
      const auto start_time = frame_to_time(range.first, rate, range_location);
      const auto end_time = frame_to_time(end_frame, rate, range_location);
      clip.active_range = TimeRangeNs{
          .start = start_time,
          .duration = end_time - start_time,
      };
    }
    expect(TokenKind::semicolon, "';'");

    expect_identifier("source_range");
    expect_identifier("ticks");
    const auto source_range = parse_call_signed_unsigned_pair();
    clip.source_range = MediaTickRange{
        .start = source_range.first,
        .duration = source_range.second,
    };
    expect(TokenKind::semicolon, "';'");
    expect_identifier("enabled");
    clip.enabled = parse_call_boolean();
    expect(TokenKind::semicolon, "';'");
    expect_identifier("locked");
    clip.locked = parse_call_boolean();
    expect(TokenKind::semicolon, "';'");
    expect(TokenKind::right_brace, "'}'");
    return clip;
  }

  [[nodiscard]] AudioClipSnapshot parse_audio_clip(
      const RationalRate rate) {
    expect_identifier("audio_clip");
    expect_identifier("id");
    AudioClipSnapshot clip;
    clip.audio_clip_id = AudioClipId{parse_call_string()};
    expect_identifier("link");
    clip.linked_import_id = LinkedImportId{parse_call_string()};
    expect_identifier("source");
    clip.media_source_id = MediaSourceId{parse_call_string()};
    expect_identifier("stream");
    clip.stream_id = MediaStreamId{parse_call_string()};
    expect_identifier("name");
    clip.display_name = parse_call_string();
    expect(TokenKind::left_brace, "'{'");

    expect_identifier("range");
    const auto range_location = current_.location;
    if (is_identifier("ns")) {
      advance();
      const auto range = parse_call_integer_pair();
      clip.active_range =
          TimeRangeNs{.start = range.first, .duration = range.second};
    } else {
      expect_identifier("frames");
      const auto range = parse_call_integer_pair();
      const auto end_frame =
          add_frames(range.first, range.second, range_location);
      const auto start_time = frame_to_time(range.first, rate, range_location);
      const auto end_time = frame_to_time(end_frame, rate, range_location);
      clip.active_range = TimeRangeNs{
          .start = start_time,
          .duration = end_time - start_time,
      };
    }
    expect(TokenKind::semicolon, "';'");

    expect_identifier("source_range");
    expect_identifier("ticks");
    const auto source_range = parse_call_signed_unsigned_pair();
    clip.source_range = MediaTickRange{
        .start = source_range.first,
        .duration = source_range.second,
    };
    expect(TokenKind::semicolon, "';'");
    expect_identifier("enabled");
    clip.enabled = parse_call_boolean();
    expect(TokenKind::semicolon, "';'");
    expect_identifier("locked");
    clip.locked = parse_call_boolean();
    expect(TokenKind::semicolon, "';'");
    expect_identifier("gain");
    clip.gain = parse_call_number();
    expect(TokenKind::semicolon, "';'");
    expect_identifier("muted");
    clip.muted = parse_call_boolean();
    expect(TokenKind::semicolon, "';'");
    expect_identifier("solo");
    clip.solo = parse_call_boolean();
    expect(TokenKind::semicolon, "';'");
    expect(TokenKind::right_brace, "'}'");
    return clip;
  }

  [[nodiscard]] LayerSnapshot parse_layer(const RationalRate rate) {
    expect_identifier("layer");
    const auto type_location = current_.location;
    if (!is_identifier("shape") && !is_identifier("text")) {
      fail("RFX-RFX-LAYER-001", "layer type must be shape or text", type_location);
    }
    const bool shape_layer = is_identifier("shape");
    advance();
    expect_identifier("id");
    LayerSnapshot layer;
    layer.layer_id = LayerId{parse_call_string()};
    expect_identifier("name");
    layer.display_name = parse_call_string();
    expect(TokenKind::left_brace, "'{'");

    expect_identifier("range");
    expect_identifier("frames");
    const auto range_location = current_.location;
    const auto range = parse_call_integer_pair();
    const auto end_frame = add_frames(range.first, range.second, range_location);
    const auto start_time = frame_to_time(range.first, rate, range_location);
    const auto end_time = frame_to_time(end_frame, rate, range_location);
    layer.active_range = TimeRangeNs{
        .start = start_time,
        .duration = end_time - start_time,
    };
    expect(TokenKind::semicolon, "';'");

    layer.transform = parse_transform();
    if (is_identifier("blend")) {
      if (language_version_ < 3) {
        fail("RFX-RFX-VERSION-006",
             "blend modes require Project.rfx language version 3",
             current_.location);
      }
      expect_identifier("blend");
      expect(TokenKind::left_parenthesis, "'('");
      if (is_identifier("normal")) {
        layer.blend_mode = BlendMode::normal;
      } else if (is_identifier("multiply")) {
        layer.blend_mode = BlendMode::multiply;
      } else if (is_identifier("screen")) {
        layer.blend_mode = BlendMode::screen;
      } else if (is_identifier("overlay")) {
        layer.blend_mode = BlendMode::overlay;
      } else {
        fail("RFX-RFX-BLEND-001",
             "blend mode must be normal, multiply, screen or overlay",
             current_.location);
      }
      advance();
      expect(TokenKind::right_parenthesis, "')'");
      expect(TokenKind::semicolon, "';'");
    }
    if (shape_layer) {
      layer.content = parse_shape_content();
    } else {
      layer.content = parse_text_content();
    }
    while (is_identifier("mask")) {
      if (language_version_ < 3) {
        fail("RFX-RFX-VERSION-007",
             "masks require Project.rfx language version 3",
             current_.location);
      }
      layer.masks.push_back(parse_mask());
    }
    while (is_identifier("effect")) {
      if (language_version_ < 3) {
        fail("RFX-RFX-VERSION-003",
             "effects require Project.rfx language version 3",
             current_.location);
      }
      layer.effects.push_back(parse_effect());
    }
    while (is_identifier("animate")) {
      layer.animations.push_back(parse_animation(rate));
    }
    expect(TokenKind::right_brace, "'}'");
    return layer;
  }

  [[nodiscard]] LayerMask parse_mask() {
    if (language_version_ >= 5) {
      return parse_registered_mask();
    }
    expect_identifier("mask");
    expect_identifier("rounded_rect");
    expect_identifier("id");
    LayerMask mask;
    mask.mask_id = MaskId{parse_call_string()};
    expect_identifier("enabled");
    mask.enabled = parse_call_boolean();
    expect_identifier("inverted");
    mask.inverted = parse_call_boolean();
    expect(TokenKind::left_brace, "'{'");
    expect_identifier("position");
    expect_identifier("local_px");
    const auto position = parse_call_number_pair();
    mask.geometry.position_x = position.first;
    mask.geometry.position_y = position.second;
    expect(TokenKind::semicolon, "';'");
    expect_identifier("size");
    expect_identifier("px");
    const auto size = parse_call_number_pair();
    mask.geometry.width = size.first;
    mask.geometry.height = size.second;
    expect(TokenKind::semicolon, "';'");
    expect_identifier("corner_radius");
    expect_identifier("px");
    mask.geometry.corner_radius = parse_call_number();
    expect(TokenKind::semicolon, "';'");
    expect(TokenKind::right_brace, "'}'");
    return mask;
  }

  [[nodiscard]] VisualParameterValue parse_registered_parameter(
      const VisualParameterDescriptor& parameter) {
    expect_identifier("parameter");
    expect_identifier(parameter.id);
    const auto value_location = current_.location;
    switch (parameter.value_kind) {
      case VisualParameterValueKind::number:
        expect_identifier("number");
        return parse_call_number();
      case VisualParameterValueKind::color_rgba8:
        expect_identifier("color_rgba8");
        return parse_color(parse_call_string(), value_location);
      case VisualParameterValueKind::boolean:
        expect_identifier("boolean");
        return parse_call_boolean();
    }
    fail("RFX-RFX-CONTRIBUTION-PARAMETER-001",
         "unsupported registered visual parameter type", value_location);
  }

  [[nodiscard]] LayerMask parse_registered_mask() {
    expect_identifier("mask");
    const auto kind_location = current_.location;
    if (current_.kind != TokenKind::identifier) {
      fail("RFX-RFX-MASK-001", "expected a registered mask descriptor ID",
           kind_location);
    }
    const auto kind = current_.text;
    advance();
    const auto* descriptor = find_visual_contribution_descriptor(kind);
    if (descriptor == nullptr ||
        descriptor->category != VisualContributionCategory::mask) {
      fail("RFX-RFX-MASK-001", "mask descriptor is not admitted",
           kind_location);
    }
    expect_identifier("id");
    const auto mask_id = MaskId{parse_call_string()};
    expect_identifier("enabled");
    const auto enabled = parse_call_boolean();
    expect_identifier("inverted");
    const auto inverted = parse_call_boolean();
    auto mask = make_default_visual_mask(kind, mask_id, 1.0, 1.0, 0.0);
    if (!mask) {
      fail("RFX-RFX-MASK-001", "mask descriptor has no admitted factory",
           kind_location);
    }
    mask->enabled = enabled;
    mask->inverted = inverted;
    expect(TokenKind::left_brace, "'{'");
    for (const auto& parameter : descriptor->parameters) {
      const auto value_location = current_.location;
      const auto value = parse_registered_parameter(parameter);
      expect(TokenKind::semicolon, "';'");
      const auto validation =
          set_visual_mask_parameter(*mask, parameter.id, value);
      if (!validation.valid) {
        fail(validation.code, validation.message, value_location);
      }
    }
    expect(TokenKind::right_brace, "'}'");
    return std::move(*mask);
  }

  [[nodiscard]] LayerEffect parse_effect() {
    if (language_version_ >= 5) {
      return parse_registered_effect();
    }
    expect_identifier("effect");
    const auto kind_location = current_.location;
    if (!is_identifier("gaussian_blur") &&
        !is_identifier("drop_shadow") && !is_identifier("glow")) {
      fail("RFX-RFX-EFFECT-001",
           "effect type must be gaussian_blur, drop_shadow or glow",
           kind_location);
    }
    const auto kind = current_.text;
    advance();
    expect_identifier("id");
    LayerEffect effect;
    effect.effect_id = EffectId{parse_call_string()};
    expect_identifier("enabled");
    effect.enabled = parse_call_boolean();
    expect(TokenKind::left_brace, "'{'");

    if (kind == "gaussian_blur") {
      expect_identifier("sigma");
      expect_identifier("px");
      const auto sigma = parse_call_number_pair();
      effect.parameters = GaussianBlurEffect{
          .sigma_x = sigma.first,
          .sigma_y = sigma.second,
      };
      expect(TokenKind::semicolon, "';'");
    } else if (kind == "drop_shadow") {
      DropShadowEffect shadow;
      expect_identifier("offset");
      expect_identifier("px");
      const auto offset = parse_call_number_pair();
      shadow.offset_x = offset.first;
      shadow.offset_y = offset.second;
      expect(TokenKind::semicolon, "';'");
      expect_identifier("sigma");
      expect_identifier("px");
      const auto sigma = parse_call_number_pair();
      shadow.sigma_x = sigma.first;
      shadow.sigma_y = sigma.second;
      expect(TokenKind::semicolon, "';'");
      expect_identifier("color");
      expect_identifier("rgba8");
      const auto color_location = current_.location;
      shadow.color = parse_color(parse_call_string(), color_location);
      expect(TokenKind::semicolon, "';'");
      effect.parameters = shadow;
    } else {
      GlowEffect glow;
      expect_identifier("sigma");
      expect_identifier("px");
      glow.sigma = parse_call_number();
      expect(TokenKind::semicolon, "';'");
      expect_identifier("color");
      expect_identifier("rgba8");
      const auto color_location = current_.location;
      glow.color = parse_color(parse_call_string(), color_location);
      expect(TokenKind::semicolon, "';'");
      effect.parameters = glow;
    }
    expect(TokenKind::right_brace, "'}'");
    return effect;
  }

  [[nodiscard]] LayerEffect parse_registered_effect() {
    expect_identifier("effect");
    const auto kind_location = current_.location;
    if (current_.kind != TokenKind::identifier) {
      fail("RFX-RFX-EFFECT-001", "expected a registered effect descriptor ID",
           kind_location);
    }
    const auto kind = current_.text;
    advance();
    const auto* descriptor = find_visual_contribution_descriptor(kind);
    if (descriptor == nullptr ||
        descriptor->category != VisualContributionCategory::effect) {
      fail("RFX-RFX-EFFECT-001", "effect descriptor is not admitted",
           kind_location);
    }
    expect_identifier("id");
    const auto effect_id = EffectId{parse_call_string()};
    expect_identifier("enabled");
    const auto enabled = parse_call_boolean();
    auto effect = make_default_visual_effect(kind, effect_id);
    if (!effect) {
      fail("RFX-RFX-EFFECT-001", "effect descriptor has no admitted factory",
           kind_location);
    }
    effect->enabled = enabled;
    expect(TokenKind::left_brace, "'{'");
    for (const auto& parameter : descriptor->parameters) {
      const auto value_location = current_.location;
      const auto value = parse_registered_parameter(parameter);
      expect(TokenKind::semicolon, "';'");
      const auto validation =
          set_visual_effect_parameter(*effect, parameter.id, value);
      if (!validation.valid) {
        fail(validation.code, validation.message, value_location);
      }
    }
    expect(TokenKind::right_brace, "'}'");
    return std::move(*effect);
  }

  [[nodiscard]] Transform2D parse_transform() {
    expect_identifier("transform");
    expect(TokenKind::left_brace, "'{'");
    Transform2D transform;

    expect_identifier("position");
    expect_identifier(language_version_ >= 4 ? "parent_px" : "canvas_px");
    const auto position = parse_call_number_pair();
    transform.position_x = position.first;
    transform.position_y = position.second;
    expect(TokenKind::semicolon, "';'");

    if (language_version_ >= 2) {
      expect_identifier("anchor");
      expect_identifier(language_version_ >= 4 ? "local_px" : "canvas_px");
      const auto anchor = parse_call_number_pair();
      transform.anchor_x = anchor.first;
      transform.anchor_y = anchor.second;
      expect(TokenKind::semicolon, "';'");
    }

    expect_identifier("scale");
    expect_identifier("ratio");
    const auto scale = parse_call_number_pair();
    transform.scale_x = scale.first;
    transform.scale_y = scale.second;
    expect(TokenKind::semicolon, "';'");

    expect_identifier("rotation");
    expect_identifier("degrees");
    transform.rotation_degrees = parse_call_number();
    expect(TokenKind::semicolon, "';'");

    expect_identifier("opacity");
    expect_identifier("ratio");
    transform.opacity = parse_call_number();
    expect(TokenKind::semicolon, "';'");
    expect(TokenKind::right_brace, "'}'");
    return transform;
  }

  [[nodiscard]] ShapeLayerContent parse_shape_content() {
    expect_identifier("content");
    expect_identifier("shape");
    expect(TokenKind::left_brace, "'{'");
    ShapeLayerContent content;
    expect_identifier("size");
    expect_identifier("px");
    const auto size = parse_call_number_pair();
    content.width = size.first;
    content.height = size.second;
    expect(TokenKind::semicolon, "';'");
    expect_identifier("corner_radius");
    expect_identifier("px");
    content.corner_radius = parse_call_number();
    expect(TokenKind::semicolon, "';'");
    expect_identifier("fill");
    if (is_identifier("rgba8")) {
      advance();
      const auto color_location = current_.location;
      content.fill = parse_color(parse_call_string(), color_location);
    } else if (is_identifier("linear_gradient")) {
      if (language_version_ < 3) {
        fail("RFX-RFX-VERSION-004",
             "gradient fills require Project.rfx language version 3",
             current_.location);
      }
      advance();
      expect(TokenKind::left_brace, "'{'");
      LinearGradientFill fill;
      expect_identifier("start");
      expect_identifier("local_px");
      const auto start = parse_call_number_pair();
      fill.start_x = start.first;
      fill.start_y = start.second;
      expect(TokenKind::semicolon, "';'");
      expect_identifier("end");
      expect_identifier("local_px");
      const auto end = parse_call_number_pair();
      fill.end_x = end.first;
      fill.end_y = end.second;
      expect(TokenKind::semicolon, "';'");
      fill.stops = parse_gradient_stops();
      expect(TokenKind::right_brace, "'}'");
      content.fill = std::move(fill);
    } else if (is_identifier("radial_gradient")) {
      if (language_version_ < 3) {
        fail("RFX-RFX-VERSION-004",
             "gradient fills require Project.rfx language version 3",
             current_.location);
      }
      advance();
      expect(TokenKind::left_brace, "'{'");
      RadialGradientFill fill;
      expect_identifier("center");
      expect_identifier("local_px");
      const auto center = parse_call_number_pair();
      fill.center_x = center.first;
      fill.center_y = center.second;
      expect(TokenKind::semicolon, "';'");
      expect_identifier("radius");
      expect_identifier("px");
      fill.radius = parse_call_number();
      expect(TokenKind::semicolon, "';'");
      fill.stops = parse_gradient_stops();
      expect(TokenKind::right_brace, "'}'");
      content.fill = std::move(fill);
    } else {
      fail("RFX-RFX-FILL-001",
           "shape fill must be rgba8, linear_gradient or radial_gradient",
           current_.location);
    }
    expect(TokenKind::semicolon, "';'");
    if (is_identifier("stroke")) {
      if (language_version_ < 3) {
        fail("RFX-RFX-VERSION-005",
             "shape stroke requires Project.rfx language version 3",
             current_.location);
      }
      expect_identifier("stroke");
      expect_identifier("width");
      expect_identifier("px");
      content.stroke_width = parse_call_number();
      expect_identifier("color");
      expect_identifier("rgba8");
      const auto color_location = current_.location;
      content.stroke_color = parse_color(parse_call_string(), color_location);
      expect(TokenKind::semicolon, "';'");
    }
    expect(TokenKind::right_brace, "'}'");
    return content;
  }

  [[nodiscard]] std::vector<GradientStop> parse_gradient_stops() {
    std::vector<GradientStop> stops;
    while (is_identifier("stop")) {
      expect_identifier("stop");
      expect_identifier("ratio");
      GradientStop stop;
      stop.offset = parse_call_number();
      expect_identifier("color");
      expect_identifier("rgba8");
      const auto color_location = current_.location;
      stop.color = parse_color(parse_call_string(), color_location);
      expect(TokenKind::semicolon, "';'");
      stops.push_back(stop);
    }
    return stops;
  }

  [[nodiscard]] VisualNodeRef parse_visual_ref() {
    const auto location = current_.location;
    if (is_identifier("layer")) {
      advance();
      return LayerId{parse_call_string()};
    }
    if (is_identifier("group")) {
      advance();
      return LayerGroupId{parse_call_string()};
    }
    fail("RFX-RFX-HIERARCHY-001",
         "expected layer(...) or group(...) visual reference",
         location);
  }

  [[nodiscard]] std::vector<VisualNodeRef> parse_visual_ref_block(
      const std::string_view name) {
    expect_identifier(name);
    expect(TokenKind::left_brace, "'{'");
    std::vector<VisualNodeRef> references;
    while (is_identifier("layer") || is_identifier("group")) {
      references.push_back(parse_visual_ref());
      expect(TokenKind::semicolon, "';'");
    }
    expect(TokenKind::right_brace, "'}'");
    return references;
  }

  [[nodiscard]] LayerGroupSnapshot parse_group(const RationalRate rate) {
    expect_identifier("group");
    expect_identifier("id");
    LayerGroupSnapshot group;
    group.group_id = LayerGroupId{parse_call_string()};
    expect_identifier("name");
    group.display_name = parse_call_string();
    expect(TokenKind::left_brace, "'{'");

    expect_identifier("range");
    expect_identifier("frames");
    const auto range_location = current_.location;
    const auto range = parse_call_integer_pair();
    const auto end_frame = add_frames(range.first, range.second, range_location);
    const auto start_time = frame_to_time(range.first, rate, range_location);
    const auto end_time = frame_to_time(end_frame, rate, range_location);
    group.active_range = TimeRangeNs{
        .start = start_time,
        .duration = end_time - start_time,
    };
    expect(TokenKind::semicolon, "';'");

    group.transform = parse_transform();
    group.children = parse_visual_ref_block("children");
    while (is_identifier("animate")) {
      group.animations.push_back(parse_animation(rate));
    }
    expect(TokenKind::right_brace, "'}'");
    return group;
  }

  [[nodiscard]] TextLayerContent parse_text_content() {
    expect_identifier("content");
    expect_identifier("text");
    expect(TokenKind::left_brace, "'{'");
    TextLayerContent content;
    expect_identifier("value");
    content.text = parse_call_string();
    expect(TokenKind::semicolon, "';'");
    if (language_version_ < 4) {
      expect_identifier("font_family");
      content.font = FontIdentity{
          .source = FontSourceKind::system_family,
          .family_name = parse_call_string(),
      };
      expect(TokenKind::semicolon, "';'");
      expect_identifier("font_size");
      expect_identifier("px");
      content.font_size = parse_call_number();
      expect(TokenKind::semicolon, "';'");
      expect_identifier("layout_width");
      expect_identifier("px");
      content.box.width = parse_call_number();
      content.box.height = content.font_size * 1.2;
      expect(TokenKind::semicolon, "';'");
      expect_identifier("direction");
      expect(TokenKind::left_parenthesis, "'('");
      if (!is_identifier("ltr") && !is_identifier("rtl")) {
        fail("RFX-RFX-TEXT-001", "text direction must be ltr or rtl",
             current_.location);
      }
      content.direction = is_identifier("ltr")
                              ? ParagraphDirection::left_to_right
                              : ParagraphDirection::right_to_left;
      advance();
      expect(TokenKind::right_parenthesis, "')'");
      expect(TokenKind::semicolon, "';'");
    } else {
      expect_identifier("font");
      if (is_identifier("system_family")) {
        advance();
        content.font = FontIdentity{
            .source = FontSourceKind::system_family,
            .family_name = parse_call_string(),
        };
      } else if (is_identifier("packaged_asset")) {
        advance();
        expect_identifier("id");
        content.font.source = FontSourceKind::packaged_asset;
        content.font.asset_id = parse_call_string();
        expect_identifier("family");
        content.font.family_name = parse_call_string();
        expect_identifier("digest");
        content.font.content_digest = parse_call_string();
      } else {
        fail("RFX-RFX-FONT-001",
             "Font source must be system_family or packaged_asset",
             current_.location);
      }
      expect(TokenKind::semicolon, "';'");
      expect_identifier("font_size");
      expect_identifier("local_px");
      content.font_size = parse_call_number();
      expect(TokenKind::semicolon, "';'");
      expect_identifier("box");
      expect_identifier("local_px");
      const auto box = parse_call_number_pair();
      content.box.width = box.first;
      content.box.height = box.second;
      expect_identifier("padding");
      expect_identifier("local_px");
      const auto padding = parse_call_number_quad();
      content.box.padding_top = padding.first;
      content.box.padding_right = padding.second;
      content.box.padding_bottom = padding.third;
      content.box.padding_left = padding.fourth;
      expect(TokenKind::semicolon, "';'");

      expect_identifier("direction");
      expect(TokenKind::left_parenthesis, "'('");
      if (!is_identifier("ltr") && !is_identifier("rtl")) {
        fail("RFX-RFX-TEXT-001", "text direction must be ltr or rtl",
             current_.location);
      }
      content.direction = is_identifier("ltr")
                              ? ParagraphDirection::left_to_right
                              : ParagraphDirection::right_to_left;
      advance();
      expect(TokenKind::right_parenthesis, "')'");
      expect(TokenKind::semicolon, "';'");

      expect_identifier("horizontal_align");
      expect(TokenKind::left_parenthesis, "'('");
      if (is_identifier("start")) {
        content.horizontal_alignment = TextHorizontalAlignment::start;
      } else if (is_identifier("center")) {
        content.horizontal_alignment = TextHorizontalAlignment::center;
      } else if (is_identifier("end")) {
        content.horizontal_alignment = TextHorizontalAlignment::end;
      } else if (is_identifier("left")) {
        content.horizontal_alignment = TextHorizontalAlignment::left;
      } else if (is_identifier("right")) {
        content.horizontal_alignment = TextHorizontalAlignment::right;
      } else {
        fail("RFX-RFX-TEXT-002", "invalid horizontal text alignment",
             current_.location);
      }
      advance();
      expect(TokenKind::right_parenthesis, "')'");
      expect(TokenKind::semicolon, "';'");

      expect_identifier("vertical_align");
      expect(TokenKind::left_parenthesis, "'('");
      if (is_identifier("top")) {
        content.vertical_alignment = TextVerticalAlignment::top;
      } else if (is_identifier("center")) {
        content.vertical_alignment = TextVerticalAlignment::center;
      } else if (is_identifier("bottom")) {
        content.vertical_alignment = TextVerticalAlignment::bottom;
      } else {
        fail("RFX-RFX-TEXT-003", "invalid vertical text alignment",
             current_.location);
      }
      advance();
      expect(TokenKind::right_parenthesis, "')'");
      expect(TokenKind::semicolon, "';'");

      expect_identifier("wrap");
      expect(TokenKind::left_parenthesis, "'('");
      if (is_identifier("no_wrap")) {
        content.wrap = TextWrapMode::no_wrap;
      } else if (is_identifier("word")) {
        content.wrap = TextWrapMode::word;
      } else {
        fail("RFX-RFX-TEXT-004", "invalid text wrap mode", current_.location);
      }
      advance();
      expect(TokenKind::right_parenthesis, "')'");
      expect(TokenKind::semicolon, "';'");

      expect_identifier("overflow");
      expect(TokenKind::left_parenthesis, "'('");
      if (is_identifier("clip")) {
        content.overflow = TextOverflowMode::clip;
      } else if (is_identifier("visible")) {
        content.overflow = TextOverflowMode::visible;
      } else {
        fail("RFX-RFX-TEXT-005", "invalid text overflow mode",
             current_.location);
      }
      advance();
      expect(TokenKind::right_parenthesis, "')'");
      expect(TokenKind::semicolon, "';'");

      expect_identifier("line_height");
      expect_identifier("ratio");
      content.line_height_ratio = parse_call_number();
      expect(TokenKind::semicolon, "';'");
      expect_identifier("letter_spacing");
      expect_identifier("local_px");
      content.letter_spacing = parse_call_number();
      expect(TokenKind::semicolon, "';'");
    }
    expect_identifier("fill");
    expect_identifier("rgba8");
    const auto color_location = current_.location;
    content.fill = parse_color(parse_call_string(), color_location);
    expect(TokenKind::semicolon, "';'");
    expect(TokenKind::right_brace, "'}'");
    return content;
  }

  [[nodiscard]] AnimatedProperty parse_property_path() {
    const auto location = current_.location;
    if (current_.kind != TokenKind::identifier) {
      fail("RFX-RFX-ANIMATION-001", "expected animation property path", location);
    }
    std::string path = current_.text;
    advance();
    while (current_.kind == TokenKind::dot) {
      advance();
      if (current_.kind != TokenKind::identifier) {
        fail("RFX-RFX-ANIMATION-001", "expected property name after '.'", current_.location);
      }
      path += '.';
      path += current_.text;
      advance();
    }
    if (path == "transform.position.x") {
      return AnimatedProperty::position_x;
    }
    if (path == "transform.position.y") {
      return AnimatedProperty::position_y;
    }
    if (path == "transform.scale.x") {
      return AnimatedProperty::scale_x;
    }
    if (path == "transform.scale.y") {
      return AnimatedProperty::scale_y;
    }
    if (path == "transform.rotation") {
      return AnimatedProperty::rotation_degrees;
    }
    if (path == "transform.opacity") {
      return AnimatedProperty::opacity;
    }
    fail("RFX-RFX-ANIMATION-002", "unsupported animation property '" + path + "'", location);
  }

  [[nodiscard]] ScalarAnimation parse_animation(const RationalRate rate) {
    expect_identifier("animate");
    ScalarAnimation animation;
    animation.property = parse_property_path();
    expect(TokenKind::left_brace, "'{'");
    if (!is_identifier("key")) {
      fail("RFX-RFX-ANIMATION-003", "animation requires at least one key", current_.location);
    }
    while (is_identifier("key")) {
      expect_identifier("key");
      expect_identifier("frame");
      const auto frame_location = current_.location;
      const auto frame = parse_call_integer();
      expect_identifier("value");
      const auto value = parse_call_number();
      expect(TokenKind::semicolon, "';'");
      animation.keyframes.push_back(ScalarKeyframe{
          .time = frame_to_time(frame, rate, frame_location),
          .value = value,
      });
    }
    expect(TokenKind::right_brace, "'}'");
    return animation;
  }

  Lexer lexer_;
  Token current_;
  std::uint64_t language_version_{1};
};

[[nodiscard]] std::string escaped_string(const std::string& value) {
  const auto utf8 = validate_preserved_utf8(value);
  if (!utf8.valid()) {
    throw std::invalid_argument(
        utf8.status == Utf8ValidationStatus::invalid_encoding
            ? "Project.rfx cannot serialize invalid UTF-8"
            : "Project.rfx cannot serialize prohibited control characters");
  }
  std::string escaped;
  escaped.reserve(value.size() + 2);
  escaped.push_back('"');
  for (const char character : value) {
    switch (character) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(character);
        break;
    }
  }
  escaped.push_back('"');
  return escaped;
}

[[nodiscard]] std::string formatted_number(const double value) {
  return canonical_float64(value);
}

[[nodiscard]] std::string color_string(const ColorRgba8 color) {
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << '"' << '#' << std::uppercase << std::hex << std::setfill('0')
         << std::setw(2) << static_cast<unsigned int>(color.red)
         << std::setw(2) << static_cast<unsigned int>(color.green)
         << std::setw(2) << static_cast<unsigned int>(color.blue)
         << std::setw(2) << static_cast<unsigned int>(color.alpha) << '"';
  return stream.str();
}

[[nodiscard]] std::string property_name(const AnimatedProperty property) {
  switch (property) {
    case AnimatedProperty::position_x:
      return "transform.position.x";
    case AnimatedProperty::position_y:
      return "transform.position.y";
    case AnimatedProperty::scale_x:
      return "transform.scale.x";
    case AnimatedProperty::scale_y:
      return "transform.scale.y";
    case AnimatedProperty::rotation_degrees:
      return "transform.rotation";
    case AnimatedProperty::opacity:
      return "transform.opacity";
  }
  throw std::invalid_argument("unknown animated property");
}

[[nodiscard]] std::string_view blend_mode_name(const BlendMode mode) {
  switch (mode) {
    case BlendMode::normal:
      return "normal";
    case BlendMode::multiply:
      return "multiply";
    case BlendMode::screen:
      return "screen";
    case BlendMode::overlay:
      return "overlay";
  }
  throw std::invalid_argument("unknown blend mode");
}

[[nodiscard]] std::string_view horizontal_alignment_name(
    const TextHorizontalAlignment alignment) {
  switch (alignment) {
    case TextHorizontalAlignment::start: return "start";
    case TextHorizontalAlignment::center: return "center";
    case TextHorizontalAlignment::end: return "end";
    case TextHorizontalAlignment::left: return "left";
    case TextHorizontalAlignment::right: return "right";
  }
  throw std::invalid_argument("unknown horizontal text alignment");
}

[[nodiscard]] std::string_view vertical_alignment_name(
    const TextVerticalAlignment alignment) {
  switch (alignment) {
    case TextVerticalAlignment::top: return "top";
    case TextVerticalAlignment::center: return "center";
    case TextVerticalAlignment::bottom: return "bottom";
  }
  throw std::invalid_argument("unknown vertical text alignment");
}

[[nodiscard]] std::string_view media_resolution_name(
    const MediaResolutionState state) {
  switch (state) {
    case MediaResolutionState::resolved: return "resolved";
    case MediaResolutionState::missing: return "missing";
    case MediaResolutionState::digest_mismatch: return "digest_mismatch";
    case MediaResolutionState::unsupported: return "unsupported";
  }
  throw std::invalid_argument("unknown media resolution state");
}

[[nodiscard]] std::string_view media_codec_name(const MediaCodec codec) {
  switch (codec) {
    case MediaCodec::h264_avc: return "h264_avc";
    case MediaCodec::aac_lc: return "aac_lc";
  }
  throw std::invalid_argument("unknown media codec");
}

[[nodiscard]] std::uint64_t exact_frame_at_time(const ProjectClockSpec& spec,
                                                const ProjectTimeNs time) {
  const auto frame = spec.frame_at_time(time);
  if (spec.time_at_frame(frame) != time) {
    throw std::invalid_argument(
        "Project.rfx serializer requires frame-aligned project time");
  }
  return frame;
}

void serialize_transform(std::ostringstream& output,
                         const Transform2D& transform,
                         const std::string_view indent) {
  output << indent << "transform {\n"
         << indent << "  position parent_px(" << formatted_number(transform.position_x)
         << ", " << formatted_number(transform.position_y) << ");\n"
         << indent << "  anchor local_px(" << formatted_number(transform.anchor_x)
         << ", " << formatted_number(transform.anchor_y) << ");\n"
         << indent << "  scale ratio(" << formatted_number(transform.scale_x) << ", "
         << formatted_number(transform.scale_y) << ");\n"
         << indent << "  rotation degrees(" << formatted_number(transform.rotation_degrees)
         << ");\n"
         << indent << "  opacity ratio(" << formatted_number(transform.opacity) << ");\n"
         << indent << "}\n";
}

void serialize_visual_ref(std::ostringstream& output,
                          const VisualNodeRef& node,
                          const std::string_view indent) {
  if (const auto* layer = std::get_if<LayerId>(&node)) {
    output << indent << "layer(" << escaped_string(layer->value) << ");\n";
  } else {
    output << indent << "group("
           << escaped_string(std::get<LayerGroupId>(node).value) << ");\n";
  }
}

void serialize_contribution_parameters(
    std::ostringstream& output,
    const std::vector<VisualParameterRecord>& parameters,
    const std::string_view indent) {
  for (const auto& parameter : parameters) {
    output << indent << "parameter " << parameter.descriptor.id << ' ';
    switch (parameter.descriptor.value_kind) {
      case VisualParameterValueKind::number:
        output << "number("
               << formatted_number(std::get<double>(parameter.value))
               << ");\n";
        break;
      case VisualParameterValueKind::color_rgba8:
        output << "color_rgba8("
               << color_string(std::get<ColorRgba8>(parameter.value))
               << ");\n";
        break;
      case VisualParameterValueKind::boolean:
        output << "boolean("
               << (std::get<bool>(parameter.value) ? "true" : "false")
               << ");\n";
        break;
    }
  }
}

void serialize_effect(std::ostringstream& output,
                      const LayerEffect& effect,
                      const std::string_view indent) {
  const auto kind = visual_effect_kind(effect);
  const auto parameters = inspect_visual_effect_parameters(effect);
  if (kind.empty() || parameters.empty()) {
    throw std::invalid_argument(
        "Project.rfx serializer requires a registered effect descriptor");
  }
  output << indent << "effect " << kind << " id("
         << escaped_string(effect.effect_id.value) << ") enabled("
         << (effect.enabled ? "true" : "false") << ") {\n";
  serialize_contribution_parameters(output, parameters,
                                    std::string(indent) + "  ");
  output << indent << "}\n";
}

void serialize_mask(std::ostringstream& output,
                    const LayerMask& mask,
                    const std::string_view indent) {
  const auto kind = visual_mask_kind(mask);
  const auto parameters = inspect_visual_mask_parameters(mask);
  if (kind.empty() || parameters.empty()) {
    throw std::invalid_argument(
        "Project.rfx serializer requires a registered mask descriptor");
  }
  output << indent << "mask " << kind << " id("
         << escaped_string(mask.mask_id.value) << ") enabled("
         << (mask.enabled ? "true" : "false") << ") inverted("
         << (mask.inverted ? "true" : "false") << ") {\n";
  serialize_contribution_parameters(output, parameters,
                                    std::string(indent) + "  ");
  output << indent << "}\n";
}

void serialize_media_contract(std::ostringstream& output,
                              const ProjectSnapshot& project) {
  output << "assets {\n";
  for (const auto& asset : project.assets) {
    output << "  asset id(" << escaped_string(asset.asset_id.value)
           << ") digest(" << escaped_string(asset.content_digest)
           << ") bytes(" << asset.byte_size
           << ") kind(video_container)\n"
           << "    original("
           << escaped_string(asset.project_relative_original) << ") name("
           << escaped_string(asset.original_display_name)
           << ") provenance(imported_copy);\n";
  }
  output << "}\n\nmedia_sources {\n";
  for (const auto& source : project.media_sources) {
    output << "  media_source id("
           << escaped_string(source.media_source_id.value) << ") asset("
           << escaped_string(source.asset_id.value) << ") index_version("
           << source.media_index_contract_version << ")\n"
           << "    index_digest(" << escaped_string(source.media_index_digest)
           << ") resolution(" << media_resolution_name(source.resolution)
           << ") {\n";
    for (const auto& stream : source.streams) {
      output << "    stream "
             << (stream.kind == MediaStreamKind::video ? "video" : "audio")
             << " id(" << escaped_string(stream.stream_id.value)
             << ") track(" << stream.container_track_id << ") codec("
             << media_codec_name(stream.codec) << ")\n"
             << "      config_digest("
             << escaped_string(stream.codec_configuration_digest)
             << ") time_base(" << stream.time_base.numerator << ", "
             << stream.time_base.denominator << ") start(" << stream.start
             << ") duration(" << stream.duration << ") {\n";
      if (const auto* video =
              std::get_if<VideoStreamFormat>(&stream.format)) {
        output << "      coded_px(" << video->coded_extent.width_pixels << ", "
               << video->coded_extent.height_pixels << ");\n"
               << "      display_px(" << video->display_extent.width_pixels
               << ", " << video->display_extent.height_pixels << ");\n"
               << "      rate(" << video->presentation_rate.numerator << ", "
               << video->presentation_rate.denominator << ");\n"
               << "      bit_depth(" << static_cast<unsigned>(video->bit_depth)
               << ");\n"
               << "      chroma("
               << static_cast<unsigned>(video->chroma_subsampling_x) << ", "
               << static_cast<unsigned>(video->chroma_subsampling_y) << ");\n"
               << "      color_range(video);\n"
               << "      primaries(" << escaped_string(video->color_primaries)
               << ");\n"
               << "      transfer(" << escaped_string(video->color_transfer)
               << ");\n"
               << "      matrix(" << escaped_string(video->color_matrix)
               << ");\n"
               << "      orientation(" << video->orientation_degrees << ");\n"
               << "      sample_aspect(" << video->sample_aspect_numerator
               << ", " << video->sample_aspect_denominator << ");\n";
      } else {
        const auto& audio = std::get<AudioStreamFormat>(stream.format);
        output << "      sample_rate(" << audio.sample_rate_hz << ");\n"
               << "      channels(" << static_cast<unsigned>(audio.channels)
               << ");\n";
      }
      output << "    }\n";
    }
    output << "    selected_video("
           << escaped_string(source.selected_video_stream->value) << ");\n";
    if (source.selected_audio_stream) {
      output << "    selected_audio("
             << escaped_string(source.selected_audio_stream->value) << ");\n";
    }
    output << "  }\n";
  }
  output << "}\n\nlinked_imports {\n";
  for (const auto& link : project.linked_imports) {
    output << "  linked_import id("
           << escaped_string(link.linked_import_id.value) << ") source("
           << escaped_string(link.media_source_id.value) << ") {\n";
    if (link.video_clip_id) {
      output << "    video_clip("
             << escaped_string(link.video_clip_id->value) << ");\n";
    }
    if (link.audio_clip_id) {
      output << "    audio_clip("
             << escaped_string(link.audio_clip_id->value) << ");\n";
    }
    output << "  }\n";
  }
  output << "}\n\n";
}

}  // namespace

RfxCompileResult compile_project_rfx(const std::string_view source) noexcept {
  RfxCompileResult result;
  try {
    result.project = Parser(source).parse();
  } catch (DiagnosticFailure& failure) {
    result.diagnostics.push_back(failure.take());
  } catch (const std::exception& failure) {
    result.diagnostics.push_back(RfxDiagnostic{
        .code = "RFX-RFX-INTERNAL-001",
        .message = failure.what(),
        .location = {},
    });
  } catch (...) {
    result.diagnostics.push_back(RfxDiagnostic{
        .code = "RFX-RFX-INTERNAL-002",
        .message = "unknown Project.rfx compiler failure",
        .location = {},
    });
  }
  return result;
}

std::string serialize_project_rfx(const ProjectSnapshot& project) {
  if (!portable_ascii_identifier(project.project_id.value) ||
      project.revision_id.value == 0 || blank(project.display_name) ||
      !validate_preserved_utf8(project.display_name).valid() ||
      !project.composition.has_value()) {
    throw std::invalid_argument("Project.rfx serializer requires a complete project");
  }
  const auto validation = validate_project(project);
  if (!validation.valid) {
    throw std::invalid_argument(validation.message);
  }

  const auto& composition = *project.composition;
  const ProjectClockSpec clock_spec{
      .duration_ns = composition.duration,
      .frame_rate = composition.frame_rate,
      .loop = false,
  };
  if (!clock_spec.valid()) {
    throw std::invalid_argument("Project.rfx serializer requires a valid clock spec");
  }

  std::ostringstream output;
  output.imbue(std::locale::classic());
  const bool has_media = !project.assets.empty() ||
                         !project.media_sources.empty() ||
                         !project.linked_imports.empty() ||
                         !composition.video_clips.empty() ||
                         !composition.audio_clips.empty();
  output << "rfx " << (has_media ? 6 : 5) << ";\n"
         << "registry digest("
         << escaped_string(visual_property_registry_digest()) << ");\n"
         << "contributions digest("
         << escaped_string(visual_contribution_registry_digest()) << ");\n\n"
         << "project id(" << escaped_string(project.project_id.value) << ") "
         << "revision(" << project.revision_id.value << ")\n"
         << "  name(" << escaped_string(project.display_name) << ");\n\n";
  if (has_media) {
    serialize_media_contract(output, project);
  }
  output << "composition id(" << escaped_string(composition.composition_id.value)
         << ") name(" << escaped_string(composition.display_name) << ") {\n"
         << "  canvas px(" << composition.canvas.width_pixels << ", "
         << composition.canvas.height_pixels << ");\n"
         << "  frame_rate rational(" << composition.frame_rate.numerator << ", "
         << composition.frame_rate.denominator << ");\n"
         << "  duration frames("
         << exact_frame_at_time(clock_spec, composition.duration) << ");\n\n";

  for (const auto& layer : composition.layers) {
    const bool is_shape = std::holds_alternative<ShapeLayerContent>(layer.content);
    const auto start_frame = exact_frame_at_time(clock_spec, layer.active_range.start);
    const auto end_frame = exact_frame_at_time(clock_spec, layer.active_range.end());
    output << "  layer " << (is_shape ? "shape" : "text") << " id("
           << escaped_string(layer.layer_id.value) << ") name("
           << escaped_string(layer.display_name) << ") {\n"
           << "    range frames(" << start_frame << ", "
           << end_frame - start_frame << ");\n";
    serialize_transform(output, layer.transform, "    ");
    output << "    blend(" << blend_mode_name(layer.blend_mode) << ");\n";

    if (const auto* shape = std::get_if<ShapeLayerContent>(&layer.content)) {
      output << "    content shape {\n"
             << "      size px(" << formatted_number(shape->width) << ", "
             << formatted_number(shape->height) << ");\n"
             << "      corner_radius px(" << formatted_number(shape->corner_radius)
             << ");\n";
      if (const auto* solid = std::get_if<ColorRgba8>(&shape->fill)) {
        output << "      fill rgba8(" << color_string(*solid) << ");\n";
      } else if (const auto* linear =
                     std::get_if<LinearGradientFill>(&shape->fill)) {
        output << "      fill linear_gradient {\n"
               << "        start local_px(" << formatted_number(linear->start_x)
               << ", " << formatted_number(linear->start_y) << ");\n"
               << "        end local_px(" << formatted_number(linear->end_x)
               << ", " << formatted_number(linear->end_y) << ");\n";
        for (const auto& stop : linear->stops) {
          output << "        stop ratio(" << formatted_number(stop.offset)
                 << ") color rgba8(" << color_string(stop.color) << ");\n";
        }
        output << "      };\n";
      } else {
        const auto& radial = std::get<RadialGradientFill>(shape->fill);
        output << "      fill radial_gradient {\n"
               << "        center local_px(" << formatted_number(radial.center_x)
               << ", " << formatted_number(radial.center_y) << ");\n"
               << "        radius px(" << formatted_number(radial.radius)
               << ");\n";
        for (const auto& stop : radial.stops) {
          output << "        stop ratio(" << formatted_number(stop.offset)
                 << ") color rgba8(" << color_string(stop.color) << ");\n";
        }
        output << "      };\n";
      }
      output << "      stroke width px(" << formatted_number(shape->stroke_width)
             << ") color rgba8(" << color_string(shape->stroke_color)
             << ");\n"
             << "    }\n";
    } else {
      const auto& text = std::get<TextLayerContent>(layer.content);
      output << "    content text {\n"
             << "      value(" << escaped_string(text.text) << ");\n";
      if (text.font.source == FontSourceKind::system_family) {
        output << "      font system_family("
               << escaped_string(text.font.family_name) << ");\n";
      } else {
        output << "      font packaged_asset id("
               << escaped_string(text.font.asset_id) << ") family("
               << escaped_string(text.font.family_name) << ") digest("
               << escaped_string(text.font.content_digest) << ");\n";
      }
      output << "      font_size local_px(" << formatted_number(text.font_size)
             << ");\n"
             << "      box local_px(" << formatted_number(text.box.width)
             << ", " << formatted_number(text.box.height)
             << ") padding local_px("
             << formatted_number(text.box.padding_top) << ", "
             << formatted_number(text.box.padding_right) << ", "
             << formatted_number(text.box.padding_bottom) << ", "
             << formatted_number(text.box.padding_left) << ");\n"
             << "      direction("
             << (text.direction == ParagraphDirection::left_to_right ? "ltr"
                                                                      : "rtl")
             << ");\n"
             << "      horizontal_align("
             << horizontal_alignment_name(text.horizontal_alignment)
             << ");\n"
             << "      vertical_align("
             << vertical_alignment_name(text.vertical_alignment) << ");\n"
             << "      wrap("
             << (text.wrap == TextWrapMode::word ? "word" : "no_wrap")
             << ");\n"
             << "      overflow("
             << (text.overflow == TextOverflowMode::clip ? "clip" : "visible")
             << ");\n"
             << "      line_height ratio("
             << formatted_number(text.line_height_ratio) << ");\n"
             << "      letter_spacing local_px("
             << formatted_number(text.letter_spacing) << ");\n"
             << "      fill rgba8(" << color_string(text.fill) << ");\n"
             << "    }\n";
    }

    for (const auto& mask : layer.masks) {
      serialize_mask(output, mask, "    ");
    }

    for (const auto& effect : layer.effects) {
      serialize_effect(output, effect, "    ");
    }

    for (const auto& animation : layer.animations) {
      output << "    animate " << property_name(animation.property) << " {\n";
      for (const auto& keyframe : animation.keyframes) {
        output << "      key frame("
               << exact_frame_at_time(clock_spec, keyframe.time) << ") value("
               << formatted_number(keyframe.value) << ");\n";
      }
      output << "    }\n";
    }
    output << "  }\n\n";
  }

  for (const auto& group : composition.groups) {
    const auto start_frame = exact_frame_at_time(
        clock_spec, group.active_range.start);
    const auto end_frame = exact_frame_at_time(
        clock_spec, group.active_range.end());
    output << "  group id(" << escaped_string(group.group_id.value)
           << ") name(" << escaped_string(group.display_name) << ") {\n"
           << "    range frames(" << start_frame << ", "
           << end_frame - start_frame << ");\n";
    serialize_transform(output, group.transform, "    ");
    output << "    children {\n";
    for (const auto& child : group.children) {
      serialize_visual_ref(output, child, "      ");
    }
    output << "    }\n";
    for (const auto& animation : group.animations) {
      output << "    animate " << property_name(animation.property) << " {\n";
      for (const auto& keyframe : animation.keyframes) {
        output << "      key frame("
               << exact_frame_at_time(clock_spec, keyframe.time) << ") value("
               << formatted_number(keyframe.value) << ");\n";
      }
      output << "    }\n";
    }
    output << "  }\n\n";
  }

  for (const auto& clip : composition.video_clips) {
    output << "  video_clip id("
           << escaped_string(clip.video_clip_id.value) << ") link("
           << escaped_string(clip.linked_import_id.value) << ") source("
           << escaped_string(clip.media_source_id.value) << ") stream("
           << escaped_string(clip.stream_id.value) << ") name("
           << escaped_string(clip.display_name) << ") {\n"
           << "    range ns(" << clip.active_range.start << ", "
           << clip.active_range.duration << ");\n"
           << "    source_range ticks(" << clip.source_range.start << ", "
           << clip.source_range.duration << ");\n"
           << "    enabled(" << (clip.enabled ? "true" : "false") << ");\n"
           << "    locked(" << (clip.locked ? "true" : "false") << ");\n"
           << "  }\n\n";
  }

  for (const auto& clip : composition.audio_clips) {
    output << "  audio_clip id("
           << escaped_string(clip.audio_clip_id.value) << ") link("
           << escaped_string(clip.linked_import_id.value) << ") source("
           << escaped_string(clip.media_source_id.value) << ") stream("
           << escaped_string(clip.stream_id.value) << ") name("
           << escaped_string(clip.display_name) << ") {\n"
           << "    range ns(" << clip.active_range.start << ", "
           << clip.active_range.duration << ");\n"
           << "    source_range ticks(" << clip.source_range.start << ", "
           << clip.source_range.duration << ");\n"
           << "    enabled(" << (clip.enabled ? "true" : "false") << ");\n"
           << "    locked(" << (clip.locked ? "true" : "false") << ");\n"
           << "    gain(" << formatted_number(clip.gain) << ");\n"
           << "    muted(" << (clip.muted ? "true" : "false") << ");\n"
           << "    solo(" << (clip.solo ? "true" : "false") << ");\n"
           << "  }\n\n";
  }

  output << "  root {\n";
  for (const auto& root : composition_root_nodes(composition)) {
    serialize_visual_ref(output, root, "    ");
  }
  output << "  }\n";
  output << "}\n";
  return output.str();
}

}  // namespace refusion::core
