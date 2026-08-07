#include <memory>
#include <stdexcept>

#include "refusion/runtime/media/HardwareVideoDecode.hpp"
#include "refusion/runtime/media/MediaCapability.hpp"

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("media capability test requirement failed");
  }
}

}  // namespace

class TestSurfaceLease final
    : public refusion::runtime::media::NativeVideoSurfaceLease {
 public:
  explicit TestSurfaceLease(refusion::runtime::media::DecodedSurfaceInfo info)
      : info_(std::move(info)) {}

  [[nodiscard]] const refusion::runtime::media::DecodedSurfaceInfo& info()
      const noexcept override {
    return info_;
  }

 private:
  refusion::runtime::media::DecodedSurfaceInfo info_;
};

int main() {
  using namespace refusion::runtime::media;

  require(ExactMediaTime{.value = -1, .timescale = 30}.valid());
  require(!ExactMediaTime{.value = 0, .timescale = 0}.valid());
  require(SourceFrameTiming{
      .presentation_time = {.value = 90'000, .timescale = 90'000},
      .duration = {.value = 3'003, .timescale = 90'000},
  }
              .valid());
  require(!SourceFrameTiming{
      .presentation_time = {.value = 0, .timescale = 30},
      .duration = {.value = 0, .timescale = 30},
  }
               .valid());

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
      .device =
          refusion::runtime::gpu::DeviceIdentity{
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

  const HardwareDecodeRequest request{
      .source_path = "fixture.h264",
      .expected_profile = reference_profile,
      .source_frame_index = 3,
      .packet_timing =
          {
              .presentation_time = {.value = 3, .timescale = 30},
              .duration = {.value = 1, .timescale = 30},
          },
  };
  require(request.valid());
  require(!HardwareDecodeRequest{}.valid());

  const DecodedSurfaceInfo surface_info{
      .lease_id = 7,
      .source_frame_index = 3,
      .profile = reference_profile,
      .timing = request.packet_timing,
      .device =
          {
              .backend = refusion::runtime::gpu::Backend::metal,
              .adapter_name = "test-adapter",
              .adapter_id = 1,
              .generation = 1,
          },
      .plane_count = 2,
  };
  require(surface_info.valid());
  const HardwareDecodeResult decoded{
      .state = DecodeState::decoded,
      .hardware_decoder = true,
      .surface = std::make_shared<TestSurfaceLease>(surface_info),
      .counters = {},
  };
  require(decoded.admitted());
  auto mapped = decoded;
  mapped.counters.cpu_pixel_maps = 1;
  require(!mapped.admitted());

  const HardwareDecodeSequenceRequest sequence_request{
      .source_path = "fixture.h264",
      .expected_profile = reference_profile,
      .samples =
          {
              {
                  .access_unit_index = 0,
                  .source_frame_index = 0,
                  .timing =
                      {
                          .presentation_time = {.value = 0, .timescale = 30},
                          .duration = {.value = 1, .timescale = 30},
                      },
                  .decode_time = {.value = 0, .timescale = 30},
                  .sync_sample = true,
              },
              {
                  .access_unit_index = 1,
                  .source_frame_index = 1,
                  .timing =
                      {
                          .presentation_time = {.value = 1, .timescale = 30},
                          .duration = {.value = 1, .timescale = 30},
                      },
                  .decode_time = {.value = 1, .timescale = 30},
                  .sync_sample = true,
              },
          },
  };
  require(sequence_request.valid());
  auto repeated_access_unit = sequence_request;
  repeated_access_unit.samples[1].access_unit_index = 0;
  require(!repeated_access_unit.valid());

  require(compare_exact_media_time(
              {.value = 1, .timescale = 30},
              {.value = 33'333'333, .timescale = 1'000'000'000}) ==
          std::strong_ordering::greater);
  require(compare_exact_media_time(
              {.value = 1, .timescale = 30},
              {.value = 33'333'334, .timescale = 1'000'000'000}) ==
          std::strong_ordering::less);
  require(compare_exact_media_time({.value = -1, .timescale = 30},
                                   {.value = 0, .timescale = 1}) ==
          std::strong_ordering::less);

  auto frame_zero_info = surface_info;
  frame_zero_info.lease_id = 10;
  frame_zero_info.source_frame_index = 0;
  frame_zero_info.timing.presentation_time = {.value = 0, .timescale = 30};
  auto frame_one_info = surface_info;
  frame_one_info.lease_id = 11;
  frame_one_info.source_frame_index = 1;
  frame_one_info.timing.presentation_time = {.value = 1, .timescale = 30};
  auto frame_two_info = surface_info;
  frame_two_info.lease_id = 12;
  frame_two_info.source_frame_index = 2;
  frame_two_info.timing.presentation_time = {.value = 2, .timescale = 30};
  auto queue = DecodedSurfaceQueue::create({
      std::make_shared<TestSurfaceLease>(frame_two_info),
      std::make_shared<TestSurfaceLease>(frame_zero_info),
      std::make_shared<TestSurfaceLease>(frame_one_info),
  });
  require(queue->size() == 3);
  require(queue->frame(0)->info().source_frame_index == 0);
  require(queue->frame(2)->info().source_frame_index == 2);
  require(queue->select_at({.value = 33'333'333, .timescale = 1'000'000'000})
              ->info()
              .source_frame_index == 0);
  require(queue->select_at({.value = 33'333'334, .timescale = 1'000'000'000})
              ->info()
              .source_frame_index == 1);
  require(queue->select_at({.value = 100'000'000, .timescale = 1'000'000'000})
              ->info()
              .source_frame_index == 2);
  require(!queue->select_at({.value = -1, .timescale = 1'000'000'000}));

  const HardwareDecodeSequenceResult sequence_decoded{
      .state = DecodeState::decoded,
      .hardware_decoder = true,
      .queue = queue,
      .counters = {},
  };
  require(sequence_decoded.admitted());
}
