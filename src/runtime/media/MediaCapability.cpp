#include "refusion/runtime/media/MediaCapability.hpp"

namespace refusion::runtime::media {

bool ExactMediaTime::valid() const noexcept { return timescale > 0; }

bool SourceFrameTiming::valid() const noexcept {
  return presentation_time.valid() && duration.valid() && duration.value > 0;
}

bool StrictDecodeProfile::valid() const noexcept {
  return coded_width > 0 && coded_height > 0;
}

bool MediaPathCounters::strict_path_clean() const noexcept {
  return software_decoder_selections == 0 && cpu_pixel_maps == 0 &&
         cpu_pixel_conversions == 0 && cpu_pixel_uploads == 0 &&
         gpu_readbacks == 0 && cross_adapter_events == 0 &&
         unattributed_gpu_copies == 0;
}

bool DecodeCapability::admitted() const noexcept {
  return state == CapabilityState::admitted && hardware_decoder &&
         native_gpu_surface && native_plane_count > 0 && device.has_value() &&
         counters.strict_path_clean();
}

}  // namespace refusion::runtime::media
