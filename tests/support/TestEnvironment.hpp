#pragma once

#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

namespace refusion::tests {

[[nodiscard]] inline std::optional<std::string> environment_variable(
    const char* name) {
#if defined(_WIN32)
  char* raw_value = nullptr;
  std::size_t value_size = 0;
  if (_dupenv_s(&raw_value, &value_size, name) != 0 ||
      raw_value == nullptr) {
    return std::nullopt;
  }
  const std::unique_ptr<char, decltype(&std::free)> value(raw_value,
                                                          &std::free);
  return std::string(value.get());
#else
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return std::nullopt;
  }
  return std::string(value);
#endif
}

}  // namespace refusion::tests
