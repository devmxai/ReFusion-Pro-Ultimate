#include "refusion/core/VisualContributionRegistry.hpp"

#include <stdexcept>
#include <string>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error(
        "visual contribution registry test requirement failed");
  }
}

}  // namespace

int main() {
  using namespace refusion::core;

  const auto& descriptors = visual_contribution_descriptors();
  require(descriptors.size() == 4);
  require(visual_contribution_registry_digest().starts_with(
      "rfx-vc-fnv1a64:"));
  const auto markdown = visual_contribution_registry_markdown();
  require(markdown.find(visual_contribution_registry_digest()) !=
          std::string::npos);
  require(markdown.find("visual.fx.gaussian_blur.v1") != std::string::npos);
  require(find_visual_contribution_descriptor("rounded_rect") != nullptr);
  require(find_visual_contribution_descriptor("unknown") == nullptr);

  auto blur = make_default_visual_effect("gaussian_blur", EffectId{"fx_blur"});
  auto shadow =
      make_default_visual_effect("drop_shadow", EffectId{"fx_shadow"});
  auto glow = make_default_visual_effect("glow", EffectId{"fx_glow"});
  require(blur && shadow && glow);
  require(visual_effect_kind(*blur) == "gaussian_blur");
  require(inspect_visual_effect_parameters(*blur).size() == 2);
  require(inspect_visual_effect_parameters(*shadow).size() == 5);
  require(inspect_visual_effect_parameters(*glow).size() == 2);
  require(validate_visual_effect(*blur).valid);

  require(set_visual_effect_parameter(*blur, "sigma_x", 24.0).valid);
  require(std::get<GaussianBlurEffect>(blur->parameters).sigma_x == 24.0);
  const auto invalid_sigma =
      set_visual_effect_parameter(*blur, "sigma_x", 300.0);
  require(!invalid_sigma.valid);
  require(invalid_sigma.code == "RFX-PROJECT-132");
  require(std::get<GaussianBlurEffect>(blur->parameters).sigma_x == 24.0);
  require(!set_visual_effect_parameter(
               *blur, "sigma_x", ColorRgba8{.red = 1})
               .valid);

  auto mask = make_default_visual_mask(
      "rounded_rect", MaskId{"mask_main"}, 400.0, 240.0, 24.0);
  require(mask.has_value());
  require(visual_mask_kind(*mask) == "rounded_rect");
  require(inspect_visual_mask_parameters(*mask).size() == 5);
  require(validate_visual_mask(*mask).valid);
  require(set_visual_mask_parameter(*mask, "position_x", 12.0).valid);
  require(mask->geometry.position_x == 12.0);
  const auto invalid_width =
      set_visual_mask_parameter(*mask, "width", 0.0);
  require(!invalid_width.valid);
  require(invalid_width.code == "RFX-PROJECT-140");
  require(mask->geometry.width == 400.0);

  require(!make_default_visual_effect("unknown", EffectId{"fx_unknown"}));
  require(!make_default_visual_mask(
      "unknown", MaskId{"mask_unknown"}, 10.0, 10.0, 0.0));
}
