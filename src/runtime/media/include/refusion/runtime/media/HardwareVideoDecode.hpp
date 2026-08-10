#pragma once

#include <compare>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "refusion/runtime/media/MediaCapability.hpp"

namespace refusion::runtime::media {

enum class DecodeState : std::uint8_t {
  decoded,
  invalid_request,
  unsupported,
  device_unavailable,
  source_open_failed,
  source_invalid,
  session_failed,
  decode_failed,
  native_surface_interop_failed,
};

// A bounded elementary-stream request used only by the G1 hardware-path proof.
// Container demux and production seek/index contracts are intentionally
// deferred to G4; the timestamps here represent the exact packet timing a
// demuxer owns.
struct HardwareDecodeRequest final {
  std::string source_path;
  StrictDecodeProfile expected_profile;
  std::uint64_t source_frame_index{0};
  SourceFrameTiming packet_timing;

  [[nodiscard]] bool valid() const noexcept;
};

struct CompressedSampleDescriptor final {
  std::uint64_t access_unit_index{0};
  std::uint64_t source_frame_index{0};
  SourceFrameTiming timing;
  ExactMediaTime decode_time;
  bool sync_sample{false};
  // Populated by the production MediaIndex projection. The older bounded G1
  // elementary-stream proof leaves these zero and indexes its immutable test
  // access-unit table by access_unit_index.
  std::uint64_t source_byte_offset{0};
  std::uint32_t source_byte_size{0};
  std::uint32_t sample_description_index{0};

  [[nodiscard]] bool valid() const noexcept;
};

struct HardwareDecodeSequenceRequest final {
  std::string source_path;
  StrictDecodeProfile expected_profile;
  std::vector<CompressedSampleDescriptor> samples;

  [[nodiscard]] bool valid() const noexcept;
};

struct DecodedSurfaceInfo final {
  std::uint64_t lease_id{0};
  std::uint64_t source_frame_index{0};
  StrictDecodeProfile profile;
  SourceFrameTiming timing;
  gpu::DeviceIdentity device;
  std::uint32_t plane_count{0};

  [[nodiscard]] bool valid() const noexcept;
};

// Native pixels remain private to the platform implementation. Consumers may
// retain this opaque lease and inspect portable metadata, but common code never
// receives a CVPixelBuffer, Metal texture, D3D surface, or raw native handle.
class NativeVideoSurfaceLease {
 public:
  virtual ~NativeVideoSurfaceLease() = default;

  [[nodiscard]] virtual const DecodedSurfaceInfo& info() const noexcept = 0;
};

[[nodiscard]] std::strong_ordering compare_exact_media_time(
    ExactMediaTime left, ExactMediaTime right);

class DecodedSurfaceQueue final {
 public:
  [[nodiscard]] static std::shared_ptr<const DecodedSurfaceQueue> create(
      std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> surfaces);

  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] const gpu::DeviceIdentity& device_identity() const noexcept;
  [[nodiscard]] const std::shared_ptr<const NativeVideoSurfaceLease>& frame(
      std::size_t index) const;
  [[nodiscard]] std::shared_ptr<const NativeVideoSurfaceLease> select_at(
      ExactMediaTime source_time) const;

 private:
  explicit DecodedSurfaceQueue(
      std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> surfaces);

  std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> surfaces_;
  gpu::DeviceIdentity device_identity_;
};

struct HardwareDecodeResult final {
  DecodeState state{DecodeState::unsupported};
  bool hardware_decoder{false};
  std::shared_ptr<const NativeVideoSurfaceLease> surface;
  MediaPathCounters counters;
  std::string code;
  std::string diagnostic;

  [[nodiscard]] bool admitted() const noexcept;
};

struct HardwareDecodeSequenceResult final {
  DecodeState state{DecodeState::unsupported};
  bool hardware_decoder{false};
  std::shared_ptr<const DecodedSurfaceQueue> queue;
  MediaPathCounters counters;
  std::string code;
  std::string diagnostic;

  [[nodiscard]] bool admitted() const noexcept;
};

// Immutable compressed-source description prepared by the shared MediaIndex.
// Platform playback adapters consume byte ranges and codec configuration only;
// they never parse the container or reinterpret Project timing.
struct HardwareVideoPlaybackSource final {
  std::string source_path;
  std::uint64_t source_byte_size{0};
  StrictDecodeProfile expected_profile;
  std::vector<std::uint8_t> codec_configuration;
  std::vector<CompressedSampleDescriptor> samples_decode_order;

  [[nodiscard]] bool valid() const noexcept;
};

struct HardwareVideoPlaybackWindowRequest final {
  ExactMediaTime target_presentation_time;
  std::size_t maximum_surface_count{0};
  std::size_t lookahead_surface_count{0};

  [[nodiscard]] bool valid() const noexcept;
};

struct HardwareVideoPlaybackWindowResult final {
  DecodeState state{DecodeState::unsupported};
  bool hardware_decoder{false};
  std::shared_ptr<const DecodedSurfaceQueue> queue;
  MediaPathCounters counters;
  std::string code;
  std::string diagnostic;

  [[nodiscard]] bool admitted() const noexcept;
};

// Stateful native decoder used by forward playback and exact seeks. The
// session may preserve hardware reference-frame state between bounded windows,
// while every published native surface remains lifetime-bound and GPU-only.
class HardwareVideoPlaybackSession {
 public:
  virtual ~HardwareVideoPlaybackSession() = default;

  [[nodiscard]] virtual HardwareVideoPlaybackWindowResult decode_window(
      const HardwareVideoPlaybackWindowRequest& request) = 0;
  [[nodiscard]] virtual MediaPathCounters counters() const = 0;
};

class HardwareVideoDecoder {
 public:
  virtual ~HardwareVideoDecoder() = default;

  [[nodiscard]] virtual HardwareDecodeResult decode(
      const HardwareDecodeRequest& request) = 0;
  [[nodiscard]] virtual HardwareDecodeSequenceResult decode_sequence(
      const HardwareDecodeSequenceRequest& request) = 0;
  [[nodiscard]] virtual std::unique_ptr<HardwareVideoPlaybackSession>
  open_playback(const HardwareVideoPlaybackSource& source) = 0;
  [[nodiscard]] virtual MediaPathCounters counters() const = 0;
};

}  // namespace refusion::runtime::media
