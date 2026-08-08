#pragma once

#include "AtomicProjectFile.hpp"

#include <filesystem>
#include <string_view>

namespace refusion::cli {

[[nodiscard]] AtomicReplaceResult native_replace_project_file(
    const std::filesystem::path& target,
    const std::filesystem::path& temporary,
    std::string_view bytes);

}  // namespace refusion::cli
