#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace refusion::cli {

struct AtomicReplaceResult final {
  bool replaced{false};
  std::string code;
  std::string message;
};

// Publishes a canonical Project.rfx candidate only when the on-disk source is
// still byte-identical to the source that was compiled. The final rename is
// native and atomic on supported desktop filesystems; Studio remains the live
// acceptance authority and revalidates the resulting revision.
[[nodiscard]] AtomicReplaceResult replace_project_file_if_unchanged(
    const std::filesystem::path& path,
    std::string_view expected_source,
    std::string_view replacement_source);

}  // namespace refusion::cli
