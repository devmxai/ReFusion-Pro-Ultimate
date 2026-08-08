#pragma once

#include <string>

namespace refusion::application {

// Generated client guide. Application owns the mapping from portable Core
// capability/Registry descriptors to concrete CLI/MCP recipes.
[[nodiscard]] std::string agent_command_catalog_markdown();

}  // namespace refusion::application
