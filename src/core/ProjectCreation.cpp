#include "refusion/core/ProjectCreation.hpp"

#include "refusion/core/CanonicalText.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <limits>
#include <locale>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace refusion::core {
namespace {

constexpr std::uint64_t kNanosecondsPerSecond = 1'000'000'000;

[[nodiscard]] bool blank(const std::string& value) noexcept {
  return value.empty() ||
         std::all_of(value.begin(), value.end(), [](const unsigned char character) {
           return ascii_space(character);
         });
}

[[nodiscard]] CreateProjectResult rejected(std::string code,
                                           std::string message) {
  return CreateProjectResult{
      .code = std::move(code),
      .message = std::move(message),
  };
}

[[nodiscard]] std::string generated_id(const std::string_view prefix) {
  static std::atomic_uint64_t sequence{0};
  std::random_device entropy;
  const auto now = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const auto next = sequence.fetch_add(1, std::memory_order_relaxed) + 1;
  const std::array<std::uint32_t, 4> words{
      entropy(),
      entropy(),
      static_cast<std::uint32_t>(now ^ next),
      static_cast<std::uint32_t>((now >> 32U) ^ (next >> 32U)),
  };
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << prefix << std::hex << std::setfill('0');
  for (const auto word : words) {
    output << std::setw(8) << word;
  }
  return output.str();
}

[[nodiscard]] const ProjectResolutionDescriptor* find_resolution(
    const CreateProjectRequest& request) noexcept {
  const auto& presets = composition_presets();
  const auto preset = std::find_if(
      presets.begin(), presets.end(), [&request](const auto& candidate) {
        return candidate.id == request.composition_preset_id;
      });
  if (preset == presets.end()) {
    return nullptr;
  }
  const auto resolution = std::find_if(
      preset->resolutions.begin(), preset->resolutions.end(),
      [&request](const auto& candidate) {
        return candidate.id == request.resolution_id;
      });
  return resolution == preset->resolutions.end() ? nullptr : &*resolution;
}

}  // namespace

const std::vector<CompositionPresetDescriptor>& composition_presets() noexcept {
  static const std::vector<CompositionPresetDescriptor> presets{
      CompositionPresetDescriptor{
          .id = "reels-9x16",
          .display_name = "Reels",
          .aspect_label = "9:16",
          .resolutions = {
              {"1080p", "1080p", {1080, 1920}},
              {"2k", "2K", {1440, 2560}},
              {"4k", "4K", {2160, 3840}},
          },
      },
      CompositionPresetDescriptor{
          .id = "portrait-4x5",
          .display_name = "Portrait",
          .aspect_label = "4:5",
          .resolutions = {
              {"1080p", "1080p", {1080, 1350}},
              {"2k", "2K", {1440, 1800}},
              {"4k", "4K", {2160, 2700}},
          },
      },
      CompositionPresetDescriptor{
          .id = "youtube-16x9",
          .display_name = "YouTube",
          .aspect_label = "16:9",
          .resolutions = {
              {"1080p", "1080p", {1920, 1080}},
              {"2k", "2K", {2560, 1440}},
              {"4k", "4K", {3840, 2160}},
          },
      },
      CompositionPresetDescriptor{
          .id = "cinematic-239x100",
          .display_name = "Cinematic",
          .aspect_label = "2.39:1",
          .resolutions = {
              {"1080p", "1080p", {1920, 804}},
              {"2k", "2K", {2560, 1072}},
              {"4k", "4K", {3840, 1608}},
          },
      },
  };
  return presets;
}

const std::vector<std::uint32_t>& supported_project_frame_rates() noexcept {
  static const std::vector<std::uint32_t> rates{24, 25, 30, 50, 60, 90};
  return rates;
}

CreateProjectResult create_initial_project(
    const CreateProjectRequest& request) noexcept {
  try {
    if (blank(request.display_name)) {
      return rejected("RFX-CREATE-NAME-001", "project name is required");
    }
    if (request.display_name.size() > 160) {
      return rejected("RFX-CREATE-NAME-002",
                      "project name must not exceed 160 UTF-8 bytes");
    }
    const auto* resolution = find_resolution(request);
    if (resolution == nullptr) {
      return rejected("RFX-CREATE-PRESET-001",
                      "preset and resolution combination is unsupported");
    }
    const auto& frame_rates = supported_project_frame_rates();
    if (std::find(frame_rates.begin(), frame_rates.end(), request.frame_rate) ==
        frame_rates.end()) {
      return rejected("RFX-CREATE-RATE-001", "frame rate is unsupported");
    }
    if (request.duration_seconds == 0 || request.duration_seconds > 86'400) {
      return rejected("RFX-CREATE-DURATION-001",
                      "duration must be between 1 and 86400 seconds");
    }
    CompositionSnapshot composition{
        .composition_id = CompositionId{generated_id("cmp_")},
        .display_name = "Main Composition",
        .canvas = resolution->canvas,
        .frame_rate = RationalRate{
            .numerator = request.frame_rate,
            .denominator = 1,
        },
        .duration = static_cast<ProjectTimeNs>(request.duration_seconds) *
                    kNanosecondsPerSecond,
        .layers = {},
    };
    const auto validation = validate_composition(composition);
    if (!validation.valid) {
      return rejected(validation.code, validation.message);
    }
    return CreateProjectResult{
        .project = ProjectSnapshot{
            .project_id = ProjectId{generated_id("prj_")},
            .revision_id = RevisionId{1},
            .display_name = request.display_name,
            .composition = std::move(composition),
        },
    };
  } catch (const std::exception& error) {
    return rejected("RFX-CREATE-INTERNAL-001", error.what());
  } catch (...) {
    return rejected("RFX-CREATE-INTERNAL-002",
                    "unknown project creation failure");
  }
}

}  // namespace refusion::core
