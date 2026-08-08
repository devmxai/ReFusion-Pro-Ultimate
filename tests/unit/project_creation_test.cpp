#include "refusion/core/ProjectCreation.hpp"
#include "refusion/core/ProjectRfx.hpp"

#include <stdexcept>
#include <string_view>

namespace {

void require(const bool condition, const std::string_view message) {
  if (!condition) {
    throw std::runtime_error(std::string(message));
  }
}

}  // namespace

int main() {
  using namespace refusion::core;

  const auto& presets = composition_presets();
  require(presets.size() == 4, "preset registry size changed");
  require(presets.front().id == "reels-9x16", "reels preset missing");
  require(presets.front().resolutions.at(0).canvas ==
              CanvasExtent{.width_pixels = 1080, .height_pixels = 1920},
          "reels 1080p mapping changed");
  require(presets.at(2).resolutions.at(2).canvas ==
              CanvasExtent{.width_pixels = 3840, .height_pixels = 2160},
          "YouTube 4K mapping changed");

  const CreateProjectRequest request{
      .display_name = "مشروع تجريبي",
      .composition_preset_id = "reels-9x16",
      .resolution_id = "1080p",
      .frame_rate = 60,
      .duration_seconds = 30,
  };
  const auto first = create_initial_project(request);
  const auto second = create_initial_project(request);
  require(first.succeeded(), "valid create request failed");
  require(second.succeeded(), "second valid create request failed");
  require(first.project->project_id != second.project->project_id,
          "engine-generated project IDs collided");
  require(first.project->revision_id == RevisionId{1},
          "new project did not start at revision 1");
  require(first.project->composition.has_value(), "composition missing");
  require(first.project->composition->layers.empty(),
          "new composition must have an empty Timeline");
  require(first.project->composition->duration == 30'000'000'000,
          "new composition duration changed");
  require(first.project->composition->frame_rate ==
              RationalRate{.numerator = 60, .denominator = 1},
          "new composition frame rate changed");
  require(validate_composition(*first.project->composition).valid,
          "empty composition is not valid");

  const auto source = serialize_project_rfx(*first.project);
  const auto reopened = compile_project_rfx(source);
  require(reopened.succeeded(), "generated empty Project.rfx did not reopen");
  require(*reopened.project == *first.project,
          "generated empty Project.rfx did not round-trip");

  auto invalid_preset = request;
  invalid_preset.composition_preset_id = "guessed-preset";
  require(create_initial_project(invalid_preset).code ==
              "RFX-CREATE-PRESET-001",
          "unknown preset was accepted");
  auto invalid_rate = request;
  invalid_rate.frame_rate = 59;
  require(create_initial_project(invalid_rate).code == "RFX-CREATE-RATE-001",
          "unknown frame rate was accepted");
  auto invalid_duration = request;
  invalid_duration.duration_seconds = 0;
  require(create_initial_project(invalid_duration).code ==
              "RFX-CREATE-DURATION-001",
          "zero duration was accepted");
  auto invalid_name = request;
  invalid_name.display_name = "  \t";
  require(create_initial_project(invalid_name).code == "RFX-CREATE-NAME-001",
          "blank project name was accepted");
}
