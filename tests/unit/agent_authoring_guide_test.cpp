#include "refusion/application/AgentAuthoringGuide.hpp"

#include "refusion/core/VisualContributionRegistry.hpp"
#include "refusion/core/VisualPropertyRegistry.hpp"

#include <stdexcept>
#include <string>

int main() {
  const auto catalog =
      refusion::application::agent_command_catalog_markdown();
  if (catalog.find(refusion::core::visual_property_registry_digest()) ==
          std::string::npos ||
      catalog.find(refusion::core::visual_contribution_registry_digest()) ==
          std::string::npos ||
      catalog.find("visual.fx.glow.v1") == std::string::npos ||
      catalog.find("commit group") == std::string::npos ||
      catalog.find("1/1024 px") == std::string::npos ||
      catalog.find("RFX-CAP-FX-ANIMATION-001") == std::string::npos) {
    throw std::runtime_error(
        "generated Agent catalog is not bound to Core descriptors");
  }
  return 0;
}
