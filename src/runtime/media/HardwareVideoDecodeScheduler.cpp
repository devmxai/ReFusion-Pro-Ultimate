#include "refusion/runtime/media/HardwareVideoDecodeScheduler.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace refusion::runtime::media {
namespace {

constexpr std::size_t kMaximumResidentSurfaces = 16;

[[nodiscard]] bool devices_match(const gpu::DeviceIdentity& left,
                                 const gpu::DeviceIdentity& right) noexcept {
  return left.backend == right.backend && left.adapter_id == right.adapter_id &&
         left.generation == right.generation;
}

[[nodiscard]] const CompressedSampleDescriptor* select_sample_at(
    const std::vector<CompressedSampleDescriptor>& samples,
    const ExactMediaTime target) {
  const CompressedSampleDescriptor* selected = nullptr;
  for (const auto& sample : samples) {
    if (compare_exact_media_time(sample.timing.presentation_time, target) !=
            std::strong_ordering::greater &&
        (selected == nullptr ||
         compare_exact_media_time(selected->timing.presentation_time,
                                  sample.timing.presentation_time) ==
             std::strong_ordering::less)) {
      selected = &sample;
    }
  }
  return selected;
}

[[nodiscard]] HardwareSeekDecodeResult failed_result(
    const HardwareSeekDecodeRequest& request, const SeekDecodeState state,
    std::string code, std::string diagnostic,
    const MediaPathCounters counters = {},
    DecodeSchedulingTelemetry telemetry = {}) {
  return HardwareSeekDecodeResult{
      .state = state,
      .hardware_decoder = false,
      .stamp = request.stamp,
      .target_presentation_time = request.target_presentation_time,
      .counters = counters,
      .telemetry = telemetry,
      .code = std::move(code),
      .diagnostic = std::move(diagnostic),
  };
}

}  // namespace

bool MediaEvaluationStamp::valid() const noexcept {
  return transport_epoch_id != 0 && device.adapter_id != 0 &&
         device.generation != 0;
}

bool DecodeResidencyPolicy::valid() const noexcept {
  return maximum_surface_count > 0 &&
         maximum_surface_count <= kMaximumResidentSurfaces;
}

bool HardwareSeekDecodeRequest::valid() const noexcept {
  return target_presentation_time.valid() && stamp.valid() &&
         residency.valid() &&
         HardwareDecodeSequenceRequest{
             .source_path = source_path,
             .expected_profile = expected_profile,
             .samples = samples,
         }
             .valid();
}

bool HardwareSeekDecodeResult::admitted() const noexcept {
  return state == SeekDecodeState::decoded && hardware_decoder &&
         stamp.valid() && target_presentation_time.valid() && queue &&
         !queue->empty() && telemetry.seek_requests == 1 &&
         telemetry.dependency_samples_submitted > 0 &&
         telemetry.peak_surface_residency >= telemetry.surfaces_retained &&
         telemetry.surface_residency_limit > 0 &&
         telemetry.surfaces_retained <= telemetry.surface_residency_limit &&
         telemetry.surfaces_retained == queue->size() &&
         queue->size() > 0 && counters.strict_path_clean();
}

HardwareVideoDecodeScheduler::HardwareVideoDecodeScheduler(
    HardwareVideoDecoder& decoder)
    : decoder_(decoder) {}

HardwareSeekDecodeResult HardwareVideoDecodeScheduler::decode_seek(
    const HardwareSeekDecodeRequest& request) {
  DecodeSchedulingTelemetry telemetry{
      .seek_requests = 1,
      .surface_residency_limit = request.residency.maximum_surface_count,
  };
  if (!request.valid()) {
    return failed_result(
        request, SeekDecodeState::invalid_request,
        "RFX-MEDIA-SEEK-REQUEST-INVALID",
        "The seek decode request, stamp, sample index or residency policy is invalid",
        decoder_.counters(), telemetry);
  }

  const auto* target_sample =
      select_sample_at(request.samples, request.target_presentation_time);
  if (target_sample == nullptr) {
    return failed_result(request, SeekDecodeState::target_before_stream,
                         "RFX-MEDIA-SEEK-BEFORE-STREAM",
                         "The requested presentation time precedes the first sample",
                         decoder_.counters(), telemetry);
  }

  const auto target_iterator = std::find_if(
      request.samples.begin(), request.samples.end(),
      [target_sample](const auto& sample) {
        return sample.access_unit_index == target_sample->access_unit_index;
      });
  auto dependency_iterator = target_iterator;
  while (dependency_iterator != request.samples.begin() &&
         !dependency_iterator->sync_sample) {
    --dependency_iterator;
  }
  if (dependency_iterator == request.samples.end() ||
      !dependency_iterator->sync_sample) {
    return failed_result(
        request, SeekDecodeState::dependency_sync_missing,
        "RFX-MEDIA-SEEK-SYNC-MISSING",
        "No sync sample exists at or before the target access unit",
        decoder_.counters(), telemetry);
  }

  HardwareDecodeSequenceRequest dependency_request{
      .source_path = request.source_path,
      .expected_profile = request.expected_profile,
  };
  dependency_request.samples.assign(dependency_iterator,
                                    std::next(target_iterator));
  telemetry.dependency_samples_submitted =
      dependency_request.samples.size();

  auto decoded = decoder_.decode_sequence(dependency_request);
  if (!decoded.admitted()) {
    return failed_result(
        request, SeekDecodeState::hardware_decode_failed,
        decoded.code.empty() ? "RFX-MEDIA-SEEK-DECODE-FAILED" : decoded.code,
        decoded.diagnostic.empty()
            ? "The hardware decoder rejected the dependency window"
            : decoded.diagnostic,
        decoded.counters, telemetry);
  }
  if (!devices_match(decoded.queue->device_identity(), request.stamp.device)) {
    decoded.queue.reset();
    return failed_result(
        request, SeekDecodeState::device_stamp_mismatch,
        "RFX-MEDIA-SEEK-DEVICE-STAMP-MISMATCH",
        "The decoded queue device generation differs from the requested evaluation stamp",
        decoder_.counters(), telemetry);
  }

  const auto selected =
      decoded.queue->select_at(request.target_presentation_time);
  if (!selected || selected->info().source_frame_index !=
                       target_sample->source_frame_index) {
    decoded.queue.reset();
    return failed_result(
        request, SeekDecodeState::hardware_decode_failed,
        "RFX-MEDIA-SEEK-TARGET-MISSING",
        "The hardware dependency window did not produce the requested presentation sample",
        decoder_.counters(), telemetry);
  }

  telemetry.decoded_surfaces_received = decoded.queue->size();
  telemetry.peak_surface_residency = decoded.queue->size();
  std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> retained;
  retained.reserve(request.residency.maximum_surface_count);
  bool reached_target = false;
  for (std::size_t index = 0;
       index < decoded.queue->size() &&
       retained.size() < request.residency.maximum_surface_count;
       ++index) {
    const auto& surface = decoded.queue->frame(index);
    if (!reached_target && surface->info().lease_id == selected->info().lease_id) {
      reached_target = true;
    }
    if (reached_target) {
      retained.push_back(surface);
    }
  }
  if (retained.empty()) {
    decoded.queue.reset();
    return failed_result(request, SeekDecodeState::hardware_decode_failed,
                         "RFX-MEDIA-SEEK-RESIDENCY-EMPTY",
                         "Backpressure removed the requested presentation sample",
                         decoder_.counters(), telemetry);
  }

  auto bounded_queue = DecodedSurfaceQueue::create(std::move(retained));
  telemetry.surfaces_retained = bounded_queue->size();
  telemetry.surfaces_evicted_for_backpressure =
      telemetry.peak_surface_residency - telemetry.surfaces_retained;
  decoded.queue.reset();

  return HardwareSeekDecodeResult{
      .state = SeekDecodeState::decoded,
      .hardware_decoder = true,
      .stamp = request.stamp,
      .target_presentation_time = request.target_presentation_time,
      .target_source_frame_index = target_sample->source_frame_index,
      .dependency_start_access_unit = dependency_iterator->access_unit_index,
      .target_access_unit = target_sample->access_unit_index,
      .queue = std::move(bounded_queue),
      .counters = decoder_.counters(),
      .telemetry = telemetry,
      .code = "RFX-MEDIA-SEEK-WINDOW-DECODED",
      .diagnostic =
          "A sync-rooted decode-order window produced a bounded PTS-indexed GPU queue",
  };
}

HardwareSeekPublication HardwareVideoDecodeScheduler::publish_if_current(
    HardwareSeekDecodeResult result,
    const MediaEvaluationStamp& current_stamp) {
  if (!result.admitted()) {
    result.queue.reset();
    return HardwareSeekPublication{
        .accepted = false,
        .stamp = result.stamp,
        .counters = decoder_.counters(),
        .telemetry = result.telemetry,
        .code = "RFX-MEDIA-SEEK-RESULT-NOT-ADMITTED",
        .diagnostic = "Only an admitted hardware seek result may publish",
    };
  }
  if (!current_stamp.valid() ||
      result.stamp.transport_epoch_id != current_stamp.transport_epoch_id) {
    ++result.telemetry.stale_epoch_rejections;
    result.queue.reset();
    return HardwareSeekPublication{
        .accepted = false,
        .stamp = result.stamp,
        .counters = decoder_.counters(),
        .telemetry = result.telemetry,
        .code = "RFX-MEDIA-SEEK-STALE-EPOCH",
        .diagnostic =
            "The Core transport epoch changed before the decoded queue could publish",
    };
  }
  if (!devices_match(result.stamp.device, current_stamp.device)) {
    ++result.telemetry.stale_device_generation_rejections;
    result.queue.reset();
    return HardwareSeekPublication{
        .accepted = false,
        .stamp = result.stamp,
        .counters = decoder_.counters(),
        .telemetry = result.telemetry,
        .code = "RFX-MEDIA-SEEK-STALE-DEVICE-GENERATION",
        .diagnostic =
            "The engine GPU generation changed before the decoded queue could publish",
    };
  }

  auto selected = result.queue->select_at(result.target_presentation_time);
  if (!selected || selected->info().source_frame_index !=
                       result.target_source_frame_index) {
    result.queue.reset();
    return HardwareSeekPublication{
        .accepted = false,
        .stamp = result.stamp,
        .counters = decoder_.counters(),
        .telemetry = result.telemetry,
        .code = "RFX-MEDIA-SEEK-PUBLISH-TARGET-MISSING",
        .diagnostic = "The bounded queue no longer contains the target sample",
    };
  }
  return HardwareSeekPublication{
      .accepted = true,
      .stamp = result.stamp,
      .queue = std::move(result.queue),
      .selected_surface = std::move(selected),
      .counters = decoder_.counters(),
      .telemetry = result.telemetry,
      .code = "RFX-MEDIA-SEEK-PUBLISHED",
      .diagnostic =
          "The decoded GPU queue matches the current Core epoch and device generation",
  };
}

}  // namespace refusion::runtime::media
