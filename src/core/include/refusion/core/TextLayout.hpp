#pragma once

#include "refusion/core/ProjectDocument.hpp"

#include <optional>
#include <string>

namespace refusion::core {

struct TextLayoutRequest final {
  TextLayerContent text;

  friend bool operator==(const TextLayoutRequest&,
                         const TextLayoutRequest&) = default;
};

struct TextLayoutDiagnostic final {
  std::string code;
  std::string message;

  friend bool operator==(const TextLayoutDiagnostic&,
                         const TextLayoutDiagnostic&) = default;
};

struct TextLayoutOutcome final {
  std::optional<TextLayoutResult> result;
  std::optional<TextLayoutDiagnostic> diagnostic;

  [[nodiscard]] bool succeeded() const noexcept {
    return result.has_value() && !diagnostic.has_value();
  }
};

class TextLayoutPort {
 public:
  virtual ~TextLayoutPort() = default;

  [[nodiscard]] virtual std::string layout_engine_digest() const = 0;
  [[nodiscard]] virtual TextLayoutOutcome layout(
      const TextLayoutRequest& request) = 0;
};

// Full authored text/style/box/font state plus the concrete layout-engine
// digest. Exact project time is intentionally absent: animated values must be
// evaluated before constructing this immutable request.
[[nodiscard]] std::string text_layout_cache_key(
    const TextLayoutRequest& request,
    const std::string& layout_engine_digest);

}  // namespace refusion::core
