#include "refusion/core/SemanticAuthoring.hpp"

#include "refusion/core/CanonicalText.hpp"

#include <algorithm>
#include <string>
#include <unordered_set>

namespace refusion::core {
namespace {

[[nodiscard]] std::string lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](const char character) {
                   return ascii_lower(character);
                 });
  return value;
}

[[nodiscard]] bool background_component_name(const std::string& name) {
  const auto lower = lowercase(name);
  return lower.find("background") != std::string::npos ||
         lower.find("glow") != std::string::npos ||
         lower.find("accent") != std::string::npos;
}

[[nodiscard]] bool has_glow(const LayerSnapshot& layer) {
  return std::any_of(layer.effects.begin(), layer.effects.end(),
                     [](const LayerEffect& effect) {
                       return std::holds_alternative<GlowEffect>(
                           effect.parameters);
                     });
}

[[nodiscard]] bool same_text_identity(const LayerSnapshot& lhs,
                                      const LayerSnapshot& rhs) {
  const auto* lhs_text = std::get_if<TextLayerContent>(&lhs.content);
  const auto* rhs_text = std::get_if<TextLayerContent>(&rhs.content);
  return lhs_text != nullptr && rhs_text != nullptr &&
         lhs_text->text == rhs_text->text &&
         lhs_text->font == rhs_text->font &&
         lhs_text->font_size == rhs_text->font_size &&
         lhs_text->box == rhs_text->box &&
         lhs_text->direction == rhs_text->direction &&
         lhs_text->horizontal_alignment == rhs_text->horizontal_alignment &&
         lhs_text->vertical_alignment == rhs_text->vertical_alignment &&
         lhs_text->wrap == rhs_text->wrap &&
         lhs_text->overflow == rhs_text->overflow &&
         lhs_text->line_height_ratio == rhs_text->line_height_ratio &&
         lhs_text->letter_spacing == rhs_text->letter_spacing &&
         lhs.active_range == rhs.active_range && lhs.transform == rhs.transform;
}

} // namespace

std::vector<SemanticAuthoringIssue>
semantic_authoring_lint(const CompositionSnapshot& composition) {
  std::vector<SemanticAuthoringIssue> issues;

  std::vector<VisualNodeRef> background_roots;
  for (const auto& root : composition_root_nodes(composition)) {
    const auto* layer_id = std::get_if<LayerId>(&root);
    if (layer_id == nullptr) {
      continue;
    }
    const auto* layer = find_layer(composition, *layer_id);
    if (layer != nullptr &&
        std::holds_alternative<ShapeLayerContent>(layer->content) &&
        layer->active_range.start == 0 &&
        layer->active_range.duration == composition.duration &&
        background_component_name(layer->display_name)) {
      background_roots.push_back(root);
    }
  }
  if (background_roots.size() >= 2) {
    issues.push_back(SemanticAuthoringIssue{
        .code = "RFX-LINT-TOPOLOGY-UNGROUPED-BACKGROUND",
        .message = "multiple full-duration Background components are root "
                   "Layers; consider one LayerGroup",
        .nodes = std::move(background_roots),
    });
  }

  std::unordered_set<std::string> reported_text_layers;
  for (std::size_t lhs_index = 0; lhs_index < composition.layers.size();
       ++lhs_index) {
    const auto& lhs = composition.layers[lhs_index];
    if (!std::holds_alternative<TextLayerContent>(lhs.content)) {
      continue;
    }
    std::vector<VisualNodeRef> duplicates;
    for (std::size_t rhs_index = lhs_index + 1;
         rhs_index < composition.layers.size(); ++rhs_index) {
      const auto& rhs = composition.layers[rhs_index];
      if (!reported_text_layers.contains(rhs.layer_id.value) &&
          same_text_identity(lhs, rhs) && (has_glow(lhs) || has_glow(rhs))) {
        if (duplicates.empty()) {
          duplicates.emplace_back(lhs.layer_id);
        }
        duplicates.emplace_back(rhs.layer_id);
        reported_text_layers.emplace(rhs.layer_id.value);
      }
    }
    if (!duplicates.empty()) {
      reported_text_layers.emplace(lhs.layer_id.value);
      issues.push_back(SemanticAuthoringIssue{
          .code = "RFX-LINT-FX-DUPLICATE-TEXT-LAYER",
          .message = "Text Layers share content and transform while Glow is "
                     "present; verify that an FX request was not approximated "
                     "by duplicate Layers",
          .nodes = std::move(duplicates),
      });
    }
  }

  return issues;
}

} // namespace refusion::core
