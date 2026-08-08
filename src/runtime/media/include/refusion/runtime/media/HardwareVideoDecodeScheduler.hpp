#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "refusion/runtime/media/HardwareVideoDecode.hpp"

namespace refusion::runtime::media {

// A portable publication stamp copied from the Core ProjectClock snapshot and
// the engine GPU service. It does not own either authority; it only prevents a
// completed decode from crossing a seek epoch or device generation boundary.
struct MediaEvaluationStamp final {
  std::uint64_t transport_epoch_id{0};
  gpu::DeviceIdentity device;

  [[nodiscard]] bool valid() const noexcept;
};

struct DecodeResidencyPolicy final {
  std::size_t maximum_surface_count{0};

  [[nodiscard]] bool valid() const noexcept;
};

struct HardwareSeekDecodeRequest final {
  std::string source_path;
  StrictDecodeProfile expected_profile;
  std::vector<CompressedSampleDescriptor> samples;
  ExactMediaTime target_presentation_time;
  MediaEvaluationStamp stamp;
  DecodeResidencyPolicy residency;

  [[nodiscard]] bool valid() const noexcept;
};

enum class SeekDecodeState : std::uint8_t {
  decoded,
  invalid_request,
  target_before_stream,
  dependency_sync_missing,
  hardware_decode_failed,
  device_stamp_mismatch,
};

struct DecodeSchedulingTelemetry final {
  std::uint64_t seek_requests{0};
  std::uint64_t dependency_samples_submitted{0};
  std::uint64_t decoded_surfaces_received{0};
  std::uint64_t peak_surface_residency{0};
  std::uint64_t surface_residency_limit{0};
  std::uint64_t surfaces_retained{0};
  std::uint64_t surfaces_evicted_for_backpressure{0};
  std::uint64_t stale_epoch_rejections{0};
  std::uint64_t stale_device_generation_rejections{0};
};

struct HardwareSeekDecodeResult final {
  SeekDecodeState state{SeekDecodeState::invalid_request};
  bool hardware_decoder{false};
  MediaEvaluationStamp stamp;
  ExactMediaTime target_presentation_time;
  std::uint64_t target_source_frame_index{0};
  std::uint64_t dependency_start_access_unit{0};
  std::uint64_t target_access_unit{0};
  std::shared_ptr<const DecodedSurfaceQueue> queue;
  MediaPathCounters counters;
  DecodeSchedulingTelemetry telemetry;
  std::string code;
  std::string diagnostic;

  [[nodiscard]] bool admitted() const noexcept;
};

struct HardwareSeekPublication final {
  bool accepted{false};
  MediaEvaluationStamp stamp;
  std::shared_ptr<const DecodedSurfaceQueue> queue;
  std::shared_ptr<const NativeVideoSurfaceLease> selected_surface;
  MediaPathCounters counters;
  DecodeSchedulingTelemetry telemetry;
  std::string code;
  std::string diagnostic;
};

// Plans a dependency window in decode order, delegates actual pixels to the
// platform hardware decoder, bounds published surface residency and admits the
// result only against a caller-provided current Core/GPU stamp.
class HardwareVideoDecodeScheduler final {
 public:
  explicit HardwareVideoDecodeScheduler(HardwareVideoDecoder& decoder);

  [[nodiscard]] HardwareSeekDecodeResult decode_seek(
      const HardwareSeekDecodeRequest& request);
  [[nodiscard]] HardwareSeekPublication publish_if_current(
      HardwareSeekDecodeResult result,
      const MediaEvaluationStamp& current_stamp);

 private:
  HardwareVideoDecoder& decoder_;
};

}  // namespace refusion::runtime::media
