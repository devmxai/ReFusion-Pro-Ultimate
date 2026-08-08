#include "refusion/application/VisualContributionCommands.hpp"

#include <algorithm>
#include <cstddef>
#include <tuple>
#include <utility>

namespace refusion::application {
namespace {

[[nodiscard]] core::ApplyResult rejected(
    const core::ProjectSnapshot& active,
    const core::CommandId& command_id,
    std::string code,
    std::string message) {
  return core::ApplyResult{
      .status = core::ApplyStatus::rejected,
      .command_id = command_id,
      .committed_revision = active.revision_id,
      .active_snapshot = active,
      .diagnostic = core::Diagnostic{
          .code = std::move(code),
          .message = std::move(message),
          .blocking = false,
      },
  };
}

[[nodiscard]] const core::LayerSnapshot* target_layer(
    const core::ProjectSnapshot& active,
    const core::LayerId& layer_id) {
  return active.composition ? core::find_layer(*active.composition, layer_id)
                            : nullptr;
}

[[nodiscard]] std::tuple<double, double, double> default_mask_geometry(
    const core::LayerSnapshot& layer) {
  if (const auto* shape =
          std::get_if<core::ShapeLayerContent>(&layer.content)) {
    return {shape->width, shape->height, shape->corner_radius};
  }
  const auto& text = std::get<core::TextLayerContent>(layer.content);
  return {text.box.width, text.box.height, 0.0};
}

}  // namespace

core::ApplyResult submit_visual_contribution(
    ProjectCommandService& commands,
    const AddVisualContributionRequest& request) {
  const auto active = commands.active_snapshot();
  const auto* layer = target_layer(active, request.layer_id);
  if (layer == nullptr) {
    return rejected(active, request.envelope.command_id,
                    "RFX-CONTRIBUTION-LAYER-404",
                    "visual contribution target Layer does not exist");
  }
  const auto* descriptor =
      core::find_visual_contribution_descriptor(request.descriptor_id);
  if (descriptor == nullptr || descriptor->category != request.category) {
    return rejected(active, request.envelope.command_id,
                    "RFX-CONTRIBUTION-DESCRIPTOR-404",
                    "visual contribution descriptor/category is not admitted");
  }
  if (request.category == core::VisualContributionCategory::effect) {
    auto effect = core::make_default_visual_effect(
        request.descriptor_id, core::EffectId{request.instance_id});
    if (!effect) {
      return rejected(active, request.envelope.command_id,
                      "RFX-CONTRIBUTION-DESCRIPTOR-404",
                      "effect descriptor has no admitted factory");
    }
    auto effects = layer->effects;
    const auto insertion = request.insertion_index.value_or(effects.size());
    if (insertion > effects.size()) {
      return rejected(active, request.envelope.command_id,
                      "RFX-CONTRIBUTION-INDEX-400",
                      "effect insertion index exceeds the stack size");
    }
    effects.insert(effects.begin() + static_cast<std::ptrdiff_t>(insertion),
                   std::move(*effect));
    return commands.submit(core::SetLayerEffectsCommand{
        .envelope = request.envelope,
        .layer_id = request.layer_id,
        .effects = std::move(effects),
    });
  }

  const auto [width, height, corner_radius] = default_mask_geometry(*layer);
  auto mask = core::make_default_visual_mask(
      request.descriptor_id, core::MaskId{request.instance_id}, width, height,
      corner_radius);
  if (!mask) {
    return rejected(active, request.envelope.command_id,
                    "RFX-CONTRIBUTION-DESCRIPTOR-404",
                    "mask descriptor has no admitted factory");
  }
  auto masks = layer->masks;
  const auto insertion = request.insertion_index.value_or(masks.size());
  if (insertion > masks.size()) {
    return rejected(active, request.envelope.command_id,
                    "RFX-CONTRIBUTION-INDEX-400",
                    "mask insertion index exceeds the stack size");
  }
  masks.insert(masks.begin() + static_cast<std::ptrdiff_t>(insertion),
               std::move(*mask));
  return commands.submit(core::SetLayerMasksCommand{
      .envelope = request.envelope,
      .layer_id = request.layer_id,
      .masks = std::move(masks),
  });
}

core::ApplyResult submit_visual_contribution(
    ProjectCommandService& commands,
    const UpdateVisualContributionRequest& request) {
  const auto active = commands.active_snapshot();
  const auto* layer = target_layer(active, request.layer_id);
  if (layer == nullptr) {
    return rejected(active, request.envelope.command_id,
                    "RFX-CONTRIBUTION-LAYER-404",
                    "visual contribution target Layer does not exist");
  }
  if (request.category == core::VisualContributionCategory::effect) {
    auto effects = layer->effects;
    const auto found = std::find_if(
        effects.begin(), effects.end(), [&request](const auto& effect) {
          return effect.effect_id.value == request.instance_id;
        });
    if (found == effects.end()) {
      return rejected(active, request.envelope.command_id,
                      "RFX-CONTRIBUTION-INSTANCE-404",
                      "effect contribution instance does not exist");
    }
    found->enabled = request.enabled;
    for (const auto& assignment : request.parameters) {
      const auto validation = core::set_visual_effect_parameter(
          *found, assignment.parameter_id, assignment.value);
      if (!validation.valid) {
        return rejected(active, request.envelope.command_id, validation.code,
                        validation.message);
      }
    }
    return commands.submit(core::SetLayerEffectsCommand{
        .envelope = request.envelope,
        .layer_id = request.layer_id,
        .effects = std::move(effects),
    });
  }

  auto masks = layer->masks;
  const auto found = std::find_if(
      masks.begin(), masks.end(), [&request](const auto& mask) {
        return mask.mask_id.value == request.instance_id;
      });
  if (found == masks.end()) {
    return rejected(active, request.envelope.command_id,
                    "RFX-CONTRIBUTION-INSTANCE-404",
                    "mask contribution instance does not exist");
  }
  found->enabled = request.enabled;
  found->inverted = request.inverted;
  for (const auto& assignment : request.parameters) {
    const auto validation = core::set_visual_mask_parameter(
        *found, assignment.parameter_id, assignment.value);
    if (!validation.valid) {
      return rejected(active, request.envelope.command_id, validation.code,
                      validation.message);
    }
  }
  return commands.submit(core::SetLayerMasksCommand{
      .envelope = request.envelope,
      .layer_id = request.layer_id,
      .masks = std::move(masks),
  });
}

core::ApplyResult submit_visual_contribution(
    ProjectCommandService& commands,
    const RemoveVisualContributionRequest& request) {
  const auto active = commands.active_snapshot();
  const auto* layer = target_layer(active, request.layer_id);
  if (layer == nullptr) {
    return rejected(active, request.envelope.command_id,
                    "RFX-CONTRIBUTION-LAYER-404",
                    "visual contribution target Layer does not exist");
  }
  if (request.category == core::VisualContributionCategory::effect) {
    auto effects = layer->effects;
    const auto end = std::remove_if(
        effects.begin(), effects.end(), [&request](const auto& effect) {
          return effect.effect_id.value == request.instance_id;
        });
    if (end == effects.end()) {
      return rejected(active, request.envelope.command_id,
                      "RFX-CONTRIBUTION-INSTANCE-404",
                      "effect contribution instance does not exist");
    }
    effects.erase(end, effects.end());
    return commands.submit(core::SetLayerEffectsCommand{
        .envelope = request.envelope,
        .layer_id = request.layer_id,
        .effects = std::move(effects),
    });
  }

  auto masks = layer->masks;
  const auto end = std::remove_if(
      masks.begin(), masks.end(), [&request](const auto& mask) {
        return mask.mask_id.value == request.instance_id;
      });
  if (end == masks.end()) {
    return rejected(active, request.envelope.command_id,
                    "RFX-CONTRIBUTION-INSTANCE-404",
                    "mask contribution instance does not exist");
  }
  masks.erase(end, masks.end());
  return commands.submit(core::SetLayerMasksCommand{
      .envelope = request.envelope,
      .layer_id = request.layer_id,
      .masks = std::move(masks),
  });
}

}  // namespace refusion::application
