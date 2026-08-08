#pragma once

#include "refusion/core/ProjectDocument.hpp"
#include "refusion/core/TextLayout.hpp"

#include <ostream>
#include <string>
#include <string_view>

namespace refusion::cli {

void write_error_json(std::ostream& output, std::string_view schema,
                      std::string_view code, std::string_view message);
void write_validate_json(const refusion::core::ProjectSnapshot& project,
                         std::ostream& output);
void write_outline_json(const refusion::core::ProjectSnapshot& project,
                        std::ostream& output);
[[nodiscard]] bool write_inspect_json(
    const refusion::core::ProjectSnapshot& project,
    const refusion::core::VisualNodeRef& node,
    std::ostream& output,
    std::string& error);
void write_capabilities_json(std::ostream& output);
void write_lint_json(const refusion::core::ProjectSnapshot& project,
                     std::ostream& output);
void write_diff_json(const refusion::core::ProjectSnapshot& before,
                     const refusion::core::ProjectSnapshot& after,
                     std::ostream& output);
[[nodiscard]] bool write_measure_json(
    const refusion::core::ProjectSnapshot& project,
    refusion::core::ProjectTimeNs composition_time,
    refusion::core::TextLayoutPort* text_layout,
    std::ostream& output,
    std::string& error);

}  // namespace refusion::cli
