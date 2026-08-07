#include "refusion/runtime/media/MediaCapability.hpp"

#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("media capability test requirement failed");
  }
}

}  // namespace

int main() {
  using namespace refusion::runtime::media;

  require(ExactMediaTime{.value = -1, .timescale = 30}.valid());
  require(!ExactMediaTime{.value = 0, .timescale = 0}.valid());
  require(SourceFrameTiming{
      .presentation_time = {.value = 90'000, .timescale = 90'000},
      .duration = {.value = 3'003, .timescale = 90'000},
  }.valid());
  require(!SourceFrameTiming{
      .presentation_time = {.value = 0, .timescale = 30},
      .duration = {.value = 0, .timescale = 30},
  }.valid());

  const StrictDecodeProfile reference_profile{
      .coded_width = 1920,
      .coded_height = 1080,
  };
  require(reference_profile.valid());
  require(!StrictDecodeProfile{}.valid());

  MediaPathCounters counters;
  require(counters.strict_path_clean());
  counters.cpu_pixel_maps = 1;
  require(!counters.strict_path_clean());

  const DecodeCapability admitted{
      .state = CapabilityState::admitted,
      .hardware_decoder = true,
      .native_gpu_surface = true,
      .native_plane_count = 2,
      .device = refusion::runtime::gpu::DeviceIdentity{
          .backend = refusion::runtime::gpu::Backend::metal,
          .adapter_name = "test-adapter",
          .adapter_id = 1,
          .generation = 1,
      },
      .counters = {},
  };
  require(admitted.admitted());

  auto contaminated = admitted;
  contaminated.counters.software_decoder_selections = 1;
  require(!contaminated.admitted());
}
