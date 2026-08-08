#include <memory>
#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "refusion/runtime/media/HardwareVideoDecodeScheduler.hpp"

namespace {

using namespace refusion::runtime;
using namespace refusion::runtime::media;

void require(const bool condition, const std::source_location location =
                                       std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error(
        "hardware decode scheduler requirement failed at line " +
        std::to_string(location.line()));
  }
}

class FakeSurfaceLease final : public NativeVideoSurfaceLease {
 public:
  FakeSurfaceLease(DecodedSurfaceInfo info) : info_(std::move(info)) {}

  [[nodiscard]] const DecodedSurfaceInfo& info() const noexcept override {
    return info_;
  }

 private:
  DecodedSurfaceInfo info_;
};

class FakeHardwareDecoder final : public HardwareVideoDecoder {
 public:
  explicit FakeHardwareDecoder(gpu::DeviceIdentity device)
      : device_(std::move(device)) {}

  [[nodiscard]] HardwareDecodeResult decode(
      const HardwareDecodeRequest&) override {
    return {
        .state = DecodeState::unsupported,
        .counters = counters_,
        .code = "RFX-TEST-UNUSED",
    };
  }

  [[nodiscard]] HardwareDecodeSequenceResult decode_sequence(
      const HardwareDecodeSequenceRequest& request) override {
    ++counters_.hardware_decoder_queries;
    ++counters_.hardware_decoder_admissions;
    ++counters_.hardware_decoder_sessions;
    ++counters_.hardware_decoder_flushes;
    ++counters_.surface_queues_published;
    counters_.compressed_samples_submitted += request.samples.size();
    counters_.hardware_frames_decoded += request.samples.size();
    counters_.native_surface_allocations += request.samples.size();
    counters_.native_surface_plane_bindings += request.samples.size() * 2;
    counters_.native_surface_leases_issued += request.samples.size();

    std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> surfaces;
    for (const auto& sample : request.samples) {
      surfaces.push_back(std::make_shared<FakeSurfaceLease>(DecodedSurfaceInfo{
          .lease_id = sample.access_unit_index + 1,
          .source_frame_index = sample.source_frame_index,
          .profile = request.expected_profile,
          .timing = sample.timing,
          .device = device_,
          .plane_count = 2,
      }));
    }
    return {
        .state = DecodeState::decoded,
        .hardware_decoder = true,
        .queue = DecodedSurfaceQueue::create(std::move(surfaces)),
        .counters = counters_,
        .code = "RFX-TEST-DECODED",
    };
  }

  [[nodiscard]] MediaPathCounters counters() const override {
    return counters_;
  }

 private:
  gpu::DeviceIdentity device_;
  MediaPathCounters counters_;
};

[[nodiscard]] gpu::DeviceIdentity test_device(
    const std::uint64_t generation = 1) {
  return {
      .backend = gpu::Backend::metal,
      .adapter_name = "portable-test-device",
      .adapter_id = 42,
      .generation = generation,
  };
}

[[nodiscard]] CompressedSampleDescriptor sample(
    const std::uint64_t access_unit, const std::uint64_t source_frame,
    const std::int64_t presentation_value, const std::int64_t decode_value,
    const std::int64_t duration, const bool sync) {
  return {
      .access_unit_index = access_unit,
      .source_frame_index = source_frame,
      .timing =
          {
              .presentation_time =
                  {.value = presentation_value, .timescale = 30'000},
              .duration = {.value = duration, .timescale = 30'000},
          },
      .decode_time = {.value = decode_value, .timescale = 30'000},
      .sync_sample = sync,
  };
}

[[nodiscard]] std::vector<CompressedSampleDescriptor> long_gop_samples() {
  return {
      sample(0, 0, 90'000, 89'000, 1'001, true),
      sample(1, 4, 94'000, 90'000, 1'000, false),
      sample(2, 2, 92'000, 91'000, 1'000, false),
      sample(3, 1, 91'001, 91'001, 1'999, false),
      sample(4, 3, 93'000, 93'000, 1'000, false),
      sample(5, 6, 96'000, 94'000, 1'000, false),
      sample(6, 5, 95'000, 95'000, 1'000, false),
      sample(7, 8, 98'000, 96'000, 1'000, false),
      sample(8, 7, 97'000, 97'000, 1'000, false),
      sample(9, 10, 100'000, 98'000, 1'000, false),
      sample(10, 9, 99'000, 99'000, 1'000, false),
      sample(11, 11, 101'000, 100'000, 1'000, true),
  };
}

[[nodiscard]] HardwareSeekDecodeRequest make_request(
    const ExactMediaTime target, const std::size_t maximum_residency = 3) {
  return {
      .source_path = "portable-fixture.h264",
      .expected_profile = {.coded_width = 320, .coded_height = 180},
      .samples = long_gop_samples(),
      .target_presentation_time = target,
      .stamp = {.transport_epoch_id = 7, .device = test_device()},
      .residency = {.maximum_surface_count = maximum_residency},
  };
}

void prove_decode_order_vfr_and_backpressure() {
  FakeHardwareDecoder decoder(test_device());
  HardwareVideoDecodeScheduler scheduler(decoder);
  auto result = scheduler.decode_seek(
      make_request({.value = 99'500, .timescale = 30'000}));
  require(result.admitted());
  require(result.target_source_frame_index == 9);
  require(result.dependency_start_access_unit == 0);
  require(result.target_access_unit == 10);
  require(result.telemetry.dependency_samples_submitted == 11);
  require(result.telemetry.decoded_surfaces_received == 11);
  require(result.telemetry.peak_surface_residency == 11);
  require(result.telemetry.surface_residency_limit == 3);
  require(result.telemetry.surfaces_retained == 2);
  require(result.telemetry.surfaces_evicted_for_backpressure == 9);
  require(result.queue->size() == 2);
  require(result.queue->frame(0)->info().source_frame_index == 9);
  require(result.queue->frame(1)->info().source_frame_index == 10);

  auto published = scheduler.publish_if_current(
      std::move(result),
      MediaEvaluationStamp{.transport_epoch_id = 7, .device = test_device()});
  require(published.accepted);
  require(published.selected_surface->info().source_frame_index == 9);
  require(published.counters.strict_path_clean());

  auto vfr = scheduler.decode_seek(
      make_request({.value = 92'999, .timescale = 30'000}, 2));
  require(vfr.admitted());
  require(vfr.target_source_frame_index == 2);
  require(vfr.target_access_unit == 2);
  require(vfr.telemetry.dependency_samples_submitted == 3);
  require(vfr.queue->select_at({.value = 92'999, .timescale = 30'000})
              ->info()
              .source_frame_index == 2);
}

void prove_epoch_and_device_invalidation() {
  FakeHardwareDecoder decoder(test_device());
  HardwareVideoDecodeScheduler scheduler(decoder);

  auto stale_epoch = scheduler.decode_seek(
      make_request({.value = 92'000, .timescale = 30'000}));
  require(stale_epoch.admitted());
  auto epoch_publication = scheduler.publish_if_current(
      std::move(stale_epoch),
      MediaEvaluationStamp{.transport_epoch_id = 8, .device = test_device()});
  require(!epoch_publication.accepted);
  require(epoch_publication.code == "RFX-MEDIA-SEEK-STALE-EPOCH");
  require(epoch_publication.queue == nullptr);
  require(epoch_publication.telemetry.stale_epoch_rejections == 1);

  auto stale_device = scheduler.decode_seek(
      make_request({.value = 92'000, .timescale = 30'000}));
  require(stale_device.admitted());
  auto device_publication = scheduler.publish_if_current(
      std::move(stale_device),
      MediaEvaluationStamp{.transport_epoch_id = 7,
                           .device = test_device(2)});
  require(!device_publication.accepted);
  require(device_publication.code ==
          "RFX-MEDIA-SEEK-STALE-DEVICE-GENERATION");
  require(device_publication.queue == nullptr);
  require(device_publication.telemetry
              .stale_device_generation_rejections == 1);
}

void prove_fail_closed_boundaries() {
  FakeHardwareDecoder decoder(test_device());
  HardwareVideoDecodeScheduler scheduler(decoder);

  auto before_stream = scheduler.decode_seek(
      make_request({.value = 89'999, .timescale = 30'000}));
  require(!before_stream.admitted());
  require(before_stream.state == SeekDecodeState::target_before_stream);

  auto missing_sync_request =
      make_request({.value = 92'000, .timescale = 30'000});
  missing_sync_request.samples.front().sync_sample = false;
  auto missing_sync = scheduler.decode_seek(missing_sync_request);
  require(!missing_sync.admitted());
  require(missing_sync.state == SeekDecodeState::dependency_sync_missing);

  auto wrong_stamp = make_request({.value = 92'000, .timescale = 30'000});
  wrong_stamp.stamp.device.generation = 2;
  auto mismatched = scheduler.decode_seek(wrong_stamp);
  require(!mismatched.admitted());
  require(mismatched.state == SeekDecodeState::device_stamp_mismatch);

  auto invalid_residency =
      make_request({.value = 92'000, .timescale = 30'000}, 0);
  auto invalid = scheduler.decode_seek(invalid_residency);
  require(!invalid.admitted());
  require(invalid.state == SeekDecodeState::invalid_request);
}

}  // namespace

int main() {
  prove_decode_order_vfr_and_backpressure();
  prove_epoch_and_device_invalidation();
  prove_fail_closed_boundaries();
}
