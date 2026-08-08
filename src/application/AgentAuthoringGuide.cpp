#include "refusion/application/AgentAuthoringGuide.hpp"

#include "refusion/core/ProjectAuthority.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"
#include "refusion/core/VisualPropertyRegistry.hpp"

#include <sstream>

namespace refusion::application {

std::string agent_command_catalog_markdown() {
  std::ostringstream output;
  output << "# ReFusion Agent command catalog\n\n"
         << "Generated from Visual Property Registry `"
         << core::visual_property_registry_digest()
         << "` plus Mask/FX contribution registry `"
         << core::visual_contribution_registry_digest()
         << "` and the Core authoring capability table. Do not hand-edit "
            "this file.\n\n"
         << "## Read-only digital eye\n\n"
         << "- `refusion-cli outline <Project.rfx>` — semantic roots, parent "
            "paths, Timeline rows, exact ranges and ownership.\n"
         << "- `refusion-cli inspect <Project.rfx> <layer:id|group:id>` — "
            "typed Registry properties and current values.\n"
         << "- `refusion-cli measure <Project.rfx> <time-ns> --json` — exact "
            "local/logical/ink/effect/world bounds and Font digest.\n"
         << "- `refusion-cli validate <Project.rfx> --json`, `lint`, `diff` "
            "and `capabilities` — validation, advisory semantics, revision "
            "difference and fail-closed capability discovery.\n\n"
         << "## Typed commits\n\n"
         << "- `commit group <Project.rfx> <group-id> <name> <node-ref>...`\n"
         << "- `commit add-glow <Project.rfx> <layer-id> <effect-id> "
            "<sigma-px> <#RRGGBBAA>`\n"
         << "- `commit align <Project.rfx> <subject-ref> <target-ref> "
            "<time-ns> <horizontal> <vertical> <geometry|logical|ink>`\n\n"
         << "A typed commit performs revision CAS, validates the candidate and "
            "publishes one canonical file replacement. A running Studio "
            "revalidates it through the same Application authority. Never "
            "guess glyph anchors, persist measured bounds, or approximate an "
            "FX request with duplicate Layers. Derived pixel coordinates are "
            "committed on the binary-exact 1/1024 px grid.\n\n"
         << "## Capability table\n\n"
         << "| Capability | Supported | Unavailable code |\n"
         << "|---|---:|---|\n";
  for (const auto& capability : core::authoring_capabilities()) {
    output << "| `" << capability.capability_id << "` | "
           << (capability.supported ? "yes" : "no") << " | ";
    if (!capability.unavailable_code.empty()) {
      output << '`' << capability.unavailable_code << '`';
    } else {
      output << "—";
    }
    output << " |\n";
  }
  output << "\n## Admitted Mask and FX contributions\n\n"
         << "| Contribution | Capability | Parameters |\n"
         << "|---|---|---|\n";
  for (const auto& contribution :
       core::visual_contribution_descriptors()) {
    output << "| `" << contribution.id << "` | `"
           << contribution.capability_id << "` | ";
    for (std::size_t index = 0; index < contribution.parameters.size();
         ++index) {
      if (index != 0) output << ", ";
      output << '`' << contribution.parameters[index].id << '`';
    }
    output << " |\n";
  }
  return output.str();
}

}  // namespace refusion::application
