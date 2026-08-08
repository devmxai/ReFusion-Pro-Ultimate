#include "AtomicProjectFile.hpp"

#include "AtomicProjectFileNative.hpp"

#include <chrono>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

namespace refusion::cli {
namespace {

[[nodiscard]] std::string read_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] std::filesystem::path temporary_path_for(
    const std::filesystem::path& path) {
  const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
  return path.parent_path() /
         (path.filename().string() + ".agent-tmp-" + std::to_string(nonce));
}

}  // namespace

AtomicReplaceResult replace_project_file_if_unchanged(
    const std::filesystem::path& path,
    const std::string_view expected_source,
    const std::string_view replacement_source) {
  std::error_code error;
  if (!std::filesystem::is_regular_file(path, error)) {
    return {false, "RFX-AGENT-COMMIT-IO-001",
            "Project.rfx is not a readable regular file"};
  }
  if (read_file(path) != expected_source) {
    return {false, "RFX-AGENT-CAS-001",
            "Project.rfx changed after compilation; no bytes were written"};
  }
  return native_replace_project_file(path, temporary_path_for(path),
                                     replacement_source);
}

}  // namespace refusion::cli
