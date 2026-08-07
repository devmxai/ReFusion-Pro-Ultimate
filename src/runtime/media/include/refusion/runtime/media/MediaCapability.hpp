#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "refusion/runtime/gpu/GpuDeviceService.hpp"

namespace refusion::runtime::media {

enum class VideoCodec : std::uint8_t {
  h264_avc,
};

enum class VideoPixelFormat : std::uint8_t {
  nv12_8bit_video_range,
};

enum class ColorPrimaries : std::uint8_t {
  bt709,
};

enum class TransferFunction : std::uint8_t {
  bt709,
};

enum class MatrixCoefficients : std::uint8_t {
  bt709,
};

enum class CapabilityState : std::uint8_t {
  admitted,
  invalid_request,
  unsupported,
  device_unavailable,
  native_surface_interop_failed,
};

struct ExactMediaTime final {
  std::int64_t value{0};
  std::int32_t timescale{0};

  [[nodiscard]] bool valid() const noexcept;
};

struct SourceFrameTiming final {
  ExactMediaTime presentation_time;
  ExactMediaTime duration;

  [[nodiscard]] bool valid() const noexcept;
};

struct VideoColorDescription final {
  ColorPrimaries primaries{ColorPrimaries::bt709};
  TransferFunction transfer{TransferFunction::bt709};
  MatrixCoefficients matrix{MatrixCoefficients::bt709};
  bool full_range{false};
};

struct StrictDecodeProfile final {
  VideoCodec codec{VideoCodec::h264_avc};
  VideoPixelFormat pixel_format{VideoPixelFormat::nv12_8bit_video_range};
  std::uint32_t coded_width{0};
  std::uint32_t coded_height{0};
  VideoColorDescription color;

  [[nodiscard]] bool valid() const noexcept;
};

struct MediaPathCounters final {
  std::uint64_t hardware_decoder_queries{0};
  std::uint64_t hardware_decoder_admissions{0};
  std::uint64_t hardware_decoder_sessions{0};
  std::uint64_t compressed_samples_submitted{0};
  std::uint64_t hardware_frames_decoded{0};
  std::uint64_t native_surface_allocations{0};
  std::uint64_t native_surface_plane_bindings{0};
  std::uint64_t native_surface_leases_issued{0};
  std::uint64_t native_surface_leases_released{0};
  std::uint64_t software_decoder_selections{0};
  std::uint64_t cpu_pixel_maps{0};
  std::uint64_t cpu_pixel_conversions{0};
  std::uint64_t cpu_pixel_uploads{0};
  std::uint64_t gpu_readbacks{0};
  std::uint64_t cross_adapter_events{0};
  std::uint64_t unattributed_gpu_copies{0};

  [[nodiscard]] bool strict_path_clean() const noexcept;
};

struct DecodeCapability final {
  CapabilityState state{CapabilityState::unsupported};
  bool hardware_decoder{false};
  bool native_gpu_surface{false};
  std::uint32_t native_plane_count{0};
  std::optional<gpu::DeviceIdentity> device;
  MediaPathCounters counters;
  std::string code;
  std::string diagnostic;

  [[nodiscard]] bool admitted() const noexcept;
};

class MediaCapabilityProbe {
 public:
  virtual ~MediaCapabilityProbe() = default;

  [[nodiscard]] virtual DecodeCapability probe(
      const StrictDecodeProfile& profile) = 0;
  [[nodiscard]] virtual MediaPathCounters counters() const = 0;
};

}  // namespace refusion::runtime::media
