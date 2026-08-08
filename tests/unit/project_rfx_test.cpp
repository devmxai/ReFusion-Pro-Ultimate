#include "refusion/core/ProjectRfx.hpp"

#include <cmath>
#include <locale>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view kValidProject = R"RFX(rfx 1;

project id("prj_exp_01") revision(7)
  name("Agent Authoring Experiment");

composition id("cmp_reel") name("Main Reel") {
  canvas px(1080, 1920);
  frame_rate rational(30, 1);
  duration frames(900);

  layer shape id("lyr_background") name("Background") {
    range frames(0, 900);
    transform {
      position canvas_px(540, 960);
      scale ratio(1, 1);
      rotation degrees(0);
      opacity ratio(1);
    }
    content shape {
      size px(1080, 1920);
      corner_radius px(0);
      fill rgba8("#05060AFF");
    }
  }

  layer text id("lyr_title") name("Title") {
    range frames(30, 840);
    transform {
      position canvas_px(540, 720);
      scale ratio(1, 1);
      rotation degrees(0);
      opacity ratio(1);
    }
    content text {
      value("ReFusion");
      font_family("Inter");
      font_size px(96);
      layout_width px(900);
      direction(ltr);
      fill rgba8("#FFFFFFFF");
    }
    animate transform.position.x {
      key frame(30) value(220);
      key frame(450) value(860);
      key frame(870) value(220);
    }
  }
}
)RFX";

constexpr std::string_view kGroupedProject = R"RFX(rfx 2;

project id("prj_grouped") revision(8) name("Grouped Authoring");

composition id("cmp_grouped") name("Main") {
  canvas px(1080, 1920);
  frame_rate rational(30, 1);
  duration frames(900);

  layer shape id("lyr_background") name("Background") {
    range frames(0, 900);
    transform {
      position canvas_px(540, 960);
      anchor canvas_px(0, 0);
      scale ratio(1, 1);
      rotation degrees(0);
      opacity ratio(1);
    }
    content shape {
      size px(1080, 1920);
      corner_radius px(0);
      fill rgba8("#05060AFF");
    }
  }

  layer shape id("lyr_button") name("Button Body") {
    range frames(0, 900);
    transform {
      position canvas_px(540, 960);
      anchor canvas_px(0, 0);
      scale ratio(1, 1);
      rotation degrees(0);
      opacity ratio(1);
    }
    content shape {
      size px(600, 180);
      corner_radius px(90);
      fill rgba8("#7C5CFFFF");
    }
  }

  layer text id("lyr_label") name("Button Label") {
    range frames(0, 900);
    transform {
      position canvas_px(540, 980);
      anchor canvas_px(0, 0);
      scale ratio(1, 1);
      rotation degrees(0);
      opacity ratio(1);
    }
    content text {
      value("SUBSCRIBE");
      font_family("Arial");
      font_size px(64);
      layout_width px(520);
      direction(ltr);
      fill rgba8("#FFFFFFFF");
    }
  }

  group id("grp_subscribe") name("Subscribe Group") {
    range frames(0, 900);
    transform {
      position canvas_px(0, 0);
      anchor canvas_px(0, 0);
      scale ratio(1, 1);
      rotation degrees(0);
      opacity ratio(1);
    }
    children {
      layer("lyr_button");
      layer("lyr_label");
    }
    animate transform.position.x {
      key frame(0) value(0);
      key frame(450) value(100);
      key frame(900) value(0);
    }
  }

  root {
    layer("lyr_background");
    group("grp_subscribe");
  }
}
)RFX";

constexpr std::string_view kEffectsProject = R"RFX(rfx 3;

project id("prj_effects") revision(3) name("Effects Authoring");
composition id("cmp_effects") name("Main") {
  canvas px(1080, 1920);
  frame_rate rational(60, 1);
  duration frames(1800);
  layer shape id("lyr_card") name("Glass Card") {
    range frames(0, 1800);
    transform {
      position canvas_px(540, 960);
      anchor canvas_px(0, 0);
      scale ratio(1, 1);
      rotation degrees(0);
      opacity ratio(1);
    }
    blend(overlay);
    content shape {
      size px(720, 480);
      corner_radius px(64);
      fill linear_gradient {
        start local_px(-360, -240);
        end local_px(360, 240);
        stop ratio(0) color rgba8("#18213DCC");
        stop ratio(0.45) color rgba8("#7C5CFFFF");
        stop ratio(1) color rgba8("#20D0FFFF");
      };
      stroke width px(3) color rgba8("#FFFFFFFF");
    }
    mask rounded_rect id("mask_card") enabled(true) inverted(false) {
      position local_px(0, 0);
      size px(680, 440);
      corner_radius px(48);
    }
    effect drop_shadow id("fx_shadow") enabled(true) {
      offset px(0, 24);
      sigma px(18, 18);
      color rgba8("#00000080");
    }
    effect glow id("fx_glow") enabled(false) {
      sigma px(24);
      color rgba8("#7C5CFFB0");
    }
    effect gaussian_blur id("fx_blur") enabled(true) {
      sigma px(4, 6);
    }
  }
  root {
    layer("lyr_card");
  }
}
)RFX";

void require(const bool condition, const std::string_view message) {
  if (!condition) {
    throw std::runtime_error(std::string(message));
  }
}

class HostileNumericPunctuation final : public std::numpunct<char> {
 protected:
  [[nodiscard]] char do_decimal_point() const override { return ','; }
  [[nodiscard]] char do_thousands_sep() const override { return '_'; }
  [[nodiscard]] std::string do_grouping() const override { return "\3"; }
};

}  // namespace

int main() {
  using namespace refusion::core;

  const auto compiled = compile_project_rfx(kValidProject);
  require(compiled.succeeded(), "valid Project.rfx did not compile");
  const auto& project = *compiled.project;
  require(project.project_id.value == "prj_exp_01", "project ID changed");
  require(project.revision_id.value == 7, "revision changed");
  require(project.composition.has_value(), "composition missing");
  const auto& composition = *project.composition;
  require(composition.canvas.width_pixels == 1080, "canvas width changed");
  require(composition.canvas.height_pixels == 1920, "canvas height changed");
  require(composition.duration == 30'000'000'000, "duration is not 30 seconds");
  require(composition.layers.size() == 2, "layer count changed");

  const auto& title = composition.layers.at(1);
  require(title.active_range.start == 1'000'000'000, "layer start is not frame 30");
  require(title.active_range.end() == 29'000'000'000,
          "layer end is not frame 870");
  require(title.animations.size() == 1, "animation missing");
  require(title.animations.front().keyframes.at(1).time == 15'000'000'000,
          "middle key is not frame 450");
  require(std::abs(evaluate_animated_property(
                       title, AnimatedProperty::position_x, 15'000'000'000) -
                   860.0) < 0.0001,
          "animation evaluator does not consume compiled Project.rfx");

  const auto canonical = serialize_project_rfx(project);
  const auto round_trip = compile_project_rfx(canonical);
  require(round_trip.succeeded(), "canonical Project.rfx did not compile");
  require(round_trip.project->project_id == project.project_id,
          "round-trip project ID changed");
  require(round_trip.project->revision_id == project.revision_id,
          "round-trip revision changed");
  require(round_trip.project->composition->duration == composition.duration,
          "round-trip duration changed");
  require(round_trip.project->composition->layers.at(1).active_range.start ==
              title.active_range.start,
          "round-trip layer start changed");
  require(canonical.starts_with("rfx 5;"),
          "canonical migration did not emit language v5");
  require(canonical.find("registry digest(\"") != std::string::npos,
          "canonical RFX5 did not bind the property registry digest");
  require(canonical.find("contributions digest(\"") != std::string::npos,
          "canonical RFX5 did not bind the contribution registry digest");
  require(canonical.find("position parent_px(") != std::string::npos,
          "RFX5 did not preserve parent position pixels");
  require(canonical.find("anchor local_px(") != std::string::npos,
          "RFX5 did not preserve local anchor pixels");
  require(canonical.find("box local_px(900, ") != std::string::npos &&
              std::abs(std::get<TextLayerContent>(title.content).box.height -
                       115.2) < 0.0001,
          "legacy Text width did not migrate to a centered TextBox");

  const auto original_locale = std::locale();
  std::locale::global(
      std::locale(original_locale, new HostileNumericPunctuation));
  const auto hostile_locale_canonical = serialize_project_rfx(project);
  std::locale::global(original_locale);
  require(hostile_locale_canonical == canonical,
          "host locale changed canonical Project.rfx bytes");

  auto exponent_source = canonical;
  const auto exponent_number = exponent_source.find("opacity ratio(1);");
  require(exponent_number != std::string::npos,
          "canonical opacity literal is absent");
  exponent_source.replace(exponent_number,
                          std::string("opacity ratio(1);").size(),
                          "opacity ratio(1e0);");
  require(compile_project_rfx(exponent_source).succeeded(),
          "portable decimal exponent did not compile");

  auto unicode_project = project;
  unicode_project.display_name = "مشروع ReFusion";
  std::get<TextLayerContent>(
      unicode_project.composition->layers.at(1).content)
      .text = "عنوان عربي e\xCC\x81";
  const auto unicode_source = serialize_project_rfx(unicode_project);
  const auto unicode_round_trip = compile_project_rfx(unicode_source);
  require(unicode_round_trip.succeeded(),
          "valid preserved UTF-8 did not compile");
  require(*unicode_round_trip.project == unicode_project,
          "UTF-8 bytes were normalized or changed during round-trip");

  auto invalid_utf8_source = std::string(kValidProject);
  const auto invalid_utf8_offset =
      invalid_utf8_source.find("Agent Authoring Experiment");
  invalid_utf8_source.replace(invalid_utf8_offset, 1,
                              std::string{"\xC0\xAF", 2});
  const auto invalid_utf8 = compile_project_rfx(invalid_utf8_source);
  require(!invalid_utf8.succeeded(), "invalid UTF-8 was accepted");
  require(invalid_utf8.diagnostics.front().code == "RFX-RFX-UTF8-001",
          "invalid UTF-8 returned the wrong diagnostic");

  auto control_source = std::string(kValidProject);
  const auto control_offset = control_source.find("Agent Authoring Experiment");
  control_source.insert(control_offset + 1, 1, '\x01');
  const auto prohibited_control = compile_project_rfx(control_source);
  require(!prohibited_control.succeeded(),
          "prohibited Project.rfx control character was accepted");
  require(prohibited_control.diagnostics.front().code == "RFX-RFX-UTF8-002",
          "prohibited control returned the wrong diagnostic");

  auto nonportable_id_source = std::string(kValidProject);
  const auto project_id_offset = nonportable_id_source.find("prj_exp_01");
  nonportable_id_source.replace(project_id_offset,
                                std::string("prj_exp_01").size(),
                                "prj/host/path");
  const auto nonportable_id = compile_project_rfx(nonportable_id_source);
  require(!nonportable_id.succeeded(), "nonportable project ID was accepted");
  require(nonportable_id.diagnostics.front().code ==
              "RFX-RFX-PROJECT-001",
          "nonportable project ID returned the wrong diagnostic");

  const auto grouped = compile_project_rfx(kGroupedProject);
  require(grouped.succeeded(), "grouped Project.rfx did not compile");
  const auto& grouped_composition = *grouped.project->composition;
  require(grouped_composition.groups.size() == 1, "group count changed");
  require(grouped_composition.root_nodes.size() == 2,
          "explicit root order changed");
  require(grouped_composition.groups.front().children.size() == 2,
          "group child order changed");
  const auto grouped_layers =
      evaluate_visual_layers(grouped_composition, 15'000'000'000);
  require(grouped_layers.size() == 3,
          "grouped evaluation lost a visual layer");
  require(grouped_layers.at(1).layer_id == LayerId{"lyr_button"},
          "group child drawing order changed");
  require(std::abs(grouped_layers.at(1).world_transform.m02 - 640.0) <
              0.0001,
          "group parent animation did not affect the child world transform");

  const auto grouped_canonical = serialize_project_rfx(*grouped.project);
  const auto grouped_round_trip = compile_project_rfx(grouped_canonical);
  require(grouped_round_trip.succeeded(),
          "canonical grouped Project.rfx did not compile");
  require(*grouped_round_trip.project == *grouped.project,
          "grouped Project.rfx did not preserve exact project semantics");

  const auto effects = compile_project_rfx(kEffectsProject);
  require(effects.succeeded(), "effects Project.rfx did not compile");
  const auto& effect_stack =
      effects.project->composition->layers.front().effects;
  const auto& gradient = std::get<LinearGradientFill>(
      std::get<ShapeLayerContent>(
          effects.project->composition->layers.front().content)
          .fill);
  require(gradient.stops.size() == 3,
          "linear gradient stop count changed");
  require(gradient.stops.at(1).offset == 0.45,
          "linear gradient stop position changed");
  require(effects.project->composition->layers.front().blend_mode ==
              BlendMode::overlay,
          "blend mode changed");
  require(std::get<ShapeLayerContent>(
              effects.project->composition->layers.front().content)
              .stroke_width == 3.0,
          "shape stroke width changed");
  require(effects.project->composition->layers.front().masks.size() == 1,
          "mask stack changed size");
  require(effects.project->composition->layers.front().masks.front().mask_id ==
              MaskId{"mask_card"},
          "mask ID changed");
  require(effect_stack.size() == 3, "ordered effect stack changed size");
  require(effect_stack.at(0).effect_id == EffectId{"fx_shadow"},
          "effect order changed");
  require(!effect_stack.at(1).enabled, "disabled effect state changed");
  require(std::get<GlowEffect>(effect_stack.at(1).parameters).sigma == 24.0,
          "glow parameters changed");
  const auto effects_canonical = serialize_project_rfx(*effects.project);
  const auto effects_round_trip = compile_project_rfx(effects_canonical);
  require(effects_round_trip.succeeded(),
          "canonical effects Project.rfx did not compile");
  require(*effects_round_trip.project == *effects.project,
          "effects Project.rfx did not preserve exact semantics");

  auto packaged_font_project = project;
  auto& packaged_text = std::get<TextLayerContent>(
      packaged_font_project.composition->layers.at(1).content);
  packaged_text.font = FontIdentity{
      .source = FontSourceKind::packaged_asset,
      .family_name = "Inter",
      .asset_id = "font_inter_regular",
      .content_digest =
          "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
  };
  packaged_text.box = TextBox{
      .width = 900.0,
      .height = 180.0,
      .padding_top = 12.0,
      .padding_right = 24.0,
      .padding_bottom = 12.0,
      .padding_left = 24.0,
  };
  packaged_text.horizontal_alignment = TextHorizontalAlignment::center;
  packaged_text.vertical_alignment = TextVerticalAlignment::center;
  packaged_text.wrap = TextWrapMode::word;
  packaged_text.overflow = TextOverflowMode::clip;
  packaged_text.line_height_ratio = 1.35;
  packaged_text.letter_spacing = 1.5;
  const auto packaged_source = serialize_project_rfx(packaged_font_project);
  const auto packaged_round_trip = compile_project_rfx(packaged_source);
  require(packaged_round_trip.succeeded(),
          "qualified packaged Font RFX5 did not compile");
  require(*packaged_round_trip.project == packaged_font_project,
          "TextBox/Font RFX5 did not preserve exact semantics");
  require(packaged_source.find("font packaged_asset id(") != std::string::npos,
          "packaged Font identity was not serialized explicitly");

  auto mismatched_registry = packaged_source;
  const auto registry_prefix = mismatched_registry.find("rfx-vp-fnv1a64:");
  require(registry_prefix != std::string::npos,
          "registry digest prefix is absent");
  mismatched_registry.at(registry_prefix + 17) =
      mismatched_registry.at(registry_prefix + 17) == '0' ? '1' : '0';
  const auto registry_failure = compile_project_rfx(mismatched_registry);
  require(!registry_failure.succeeded(),
          "mismatched registry digest was accepted");
  require(registry_failure.diagnostics.front().code ==
              "RFX-RFX-REGISTRY-001",
          "mismatched registry returned the wrong diagnostic");

  auto mismatched_contributions = packaged_source;
  const auto contribution_prefix =
      mismatched_contributions.find("rfx-vc-fnv1a64:");
  require(contribution_prefix != std::string::npos,
          "contribution digest prefix is absent");
  mismatched_contributions.at(contribution_prefix + 17) =
      mismatched_contributions.at(contribution_prefix + 17) == '0' ? '1' : '0';
  const auto contribution_failure =
      compile_project_rfx(mismatched_contributions);
  require(!contribution_failure.succeeded(),
          "mismatched contribution registry digest was accepted");
  require(contribution_failure.diagnostics.front().code ==
              "RFX-RFX-CONTRIBUTION-REGISTRY-001",
          "mismatched contribution registry returned the wrong diagnostic");

  auto unknown_contribution_parameter = effects_canonical;
  const auto sigma_x_parameter =
      unknown_contribution_parameter.find("parameter sigma_x number(");
  require(sigma_x_parameter != std::string::npos,
          "canonical RFX5 effect parameter is absent");
  unknown_contribution_parameter.replace(
      sigma_x_parameter + std::string("parameter ").size(),
      std::string("sigma_x").size(), "unknown");
  require(!compile_project_rfx(unknown_contribution_parameter).succeeded(),
          "unknown RFX5 contribution parameter was accepted");

  auto missing_contribution_parameter = effects_canonical;
  const auto glow_parameter =
      missing_contribution_parameter.find("parameter sigma number(");
  require(glow_parameter != std::string::npos,
          "canonical RFX5 Glow parameter is absent");
  const auto glow_parameter_end =
      missing_contribution_parameter.find('\n', glow_parameter);
  require(glow_parameter_end != std::string::npos,
          "canonical RFX5 Glow parameter is not line terminated");
  missing_contribution_parameter.erase(
      glow_parameter, glow_parameter_end - glow_parameter + 1);
  require(!compile_project_rfx(missing_contribution_parameter).succeeded(),
          "missing RFX5 contribution parameter was accepted");

  auto invalid_font_project = packaged_font_project;
  std::get<TextLayerContent>(
      invalid_font_project.composition->layers.at(1).content)
      .font.content_digest = "sha256:missing";
  const auto invalid_font =
      validate_composition(*invalid_font_project.composition);
  require(!invalid_font.valid,
          "invalid packaged Font identity was accepted");
  require(invalid_font.code == "RFX-PROJECT-142",
          "invalid packaged Font returned the wrong diagnostic");

  auto invalid_effect = std::string(kEffectsProject);
  invalid_effect.replace(invalid_effect.find("sigma px(4, 6)"),
                         std::string("sigma px(4, 6)").size(),
                         "sigma px(257, 6)");
  const auto invalid_effect_result = compile_project_rfx(invalid_effect);
  require(!invalid_effect_result.succeeded(),
          "out-of-range effect sigma was accepted");
  require(invalid_effect_result.diagnostics.front().code ==
              "RFX-PROJECT-132",
          "invalid effect returned the wrong diagnostic");

  auto unknown_child_source = std::string(kGroupedProject);
  const auto child_offset = unknown_child_source.find("layer(\"lyr_button\")");
  unknown_child_source.replace(child_offset,
                               std::string("layer(\"lyr_button\")").size(),
                               "layer(\"lyr_missing\")");
  const auto unknown_child_failure =
      compile_project_rfx(unknown_child_source);
  require(!unknown_child_failure.succeeded(),
          "unknown group child was accepted");
  require(unknown_child_failure.diagnostics.front().code ==
              "RFX-PROJECT-123",
          "unknown group child returned the wrong diagnostic");

  auto invalid_group_opacity = std::string(kGroupedProject);
  const auto group_offset = invalid_group_opacity.find(
      "group id(\"grp_subscribe\")");
  const auto opacity_offset = invalid_group_opacity.find(
      "opacity ratio(1)", group_offset);
  invalid_group_opacity.replace(opacity_offset,
                                std::string("opacity ratio(1)").size(),
                                "opacity ratio(0.5)");
  const auto group_opacity_failure =
      compile_project_rfx(invalid_group_opacity);
  require(!group_opacity_failure.succeeded(),
          "non-isolated group opacity was accepted");
  require(group_opacity_failure.diagnostics.front().code ==
              "RFX-PROJECT-121",
          "group opacity returned the wrong diagnostic");

  const auto syntax_failure = compile_project_rfx(
      "rfx 1;\nproject id(\"p\") revision(1) name(\"P\");\nunknown");
  require(!syntax_failure.succeeded(), "invalid syntax was accepted");
  require(!syntax_failure.diagnostics.empty(), "syntax diagnostic missing");
  require(syntax_failure.diagnostics.front().location.line == 3,
          "syntax diagnostic line is inaccurate");

  auto duplicate_layer_source = std::string(kValidProject);
  duplicate_layer_source.replace(
      duplicate_layer_source.find("lyr_title"), std::string("lyr_title").size(),
      "lyr_background");
  const auto semantic_failure = compile_project_rfx(duplicate_layer_source);
  require(!semantic_failure.succeeded(), "duplicate layer ID was accepted");
  require(semantic_failure.diagnostics.front().code == "RFX-PROJECT-107",
          "duplicate layer ID returned the wrong diagnostic");

  auto unsupported_property_source = std::string(kValidProject);
  unsupported_property_source.replace(
      unsupported_property_source.find("transform.position.x"),
      std::string("transform.position.x").size(), "transform.skew.x");
  const auto property_failure = compile_project_rfx(unsupported_property_source);
  require(!property_failure.succeeded(), "unsupported property was accepted");
  require(property_failure.diagnostics.front().code == "RFX-RFX-ANIMATION-002",
          "unsupported property returned the wrong diagnostic");

  const auto empty_project = compile_project_rfx(R"RFX(rfx 1;
project id("prj_empty") revision(1) name("Empty");
composition id("cmp_empty") name("Main") {
  canvas px(1920, 1080);
  frame_rate rational(60, 1);
  duration frames(1800);
}
)RFX");
  require(empty_project.succeeded(), "empty composition did not compile");
  require(empty_project.project->composition->layers.empty(),
          "empty composition gained a placeholder layer");
}
