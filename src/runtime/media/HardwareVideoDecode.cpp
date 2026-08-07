#include "refusion/runtime/media/HardwareVideoDecode.hpp"

namespace refusion::runtime::media {

bool HardwareDecodeRequest::valid() const noexcept {
  return !source_path.empty() && expected_profile.valid() &&
         packet_timing.valid();
}

bool DecodedSurfaceInfo::valid() const noexcept {
  return lease_id != 0 && profile.valid() && timing.valid() &&
         device.adapter_id != 0 && device.generation != 0 && plane_count > 0;
}

bool HardwareDecodeResult::admitted() const noexcept {
  return state == DecodeState::decoded && hardware_decoder && surface &&
         surface->info().valid() && counters.strict_path_clean();
}

}  // namespace refusion::runtime::media
