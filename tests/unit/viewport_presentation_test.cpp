#include <chrono>
#include <source_location>
#include <stdexcept>
#include <string>

#include "refusion/runtime/presentation/ViewportPresentation.hpp"

namespace {

void require(const bool condition, const std::source_location where =
                                       std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error(
        "viewport presentation test requirement failed at line " +
        std::to_string(where.line()));
  }
}

class FakePresenter final
    : public refusion::runtime::presentation::ViewportPresenter {
 public:
  [[nodiscard]] refusion::runtime::presentation::FrameResult attach(
      refusion::runtime::presentation::NativeViewportHost) override {
    return {.status = refusion::runtime::presentation::FrameStatus::accepted};
  }
  void detach() noexcept override {}
  [[nodiscard]] refusion::runtime::presentation::FrameResult resize(
      refusion::runtime::presentation::ViewportExtent) override {
    return {.status = refusion::runtime::presentation::FrameStatus::accepted};
  }
  void set_visible(bool) noexcept override {}
  [[nodiscard]] refusion::runtime::presentation::FrameResult present(
      const refusion::runtime::presentation::FixtureFrame& frame) override {
    last_frame = frame;
    ++state.frame_requests;
    if (reject_next_frame) {
      reject_next_frame = false;
      ++state.rejected_frames;
      return {
          .status = refusion::runtime::presentation::FrameStatus::rejected,
          .diagnostic = "injected frame rejection",
      };
    }
    ++state.present_submissions;
    return {.status = refusion::runtime::presentation::FrameStatus::presented};
  }
  [[nodiscard]] refusion::runtime::presentation::PresentationTelemetry
  telemetry() const noexcept override {
    return state;
  }

  refusion::runtime::presentation::FixtureFrame last_frame;
  refusion::runtime::presentation::PresentationTelemetry state;
  bool reject_next_frame{false};
};

}  // namespace

int main() {
  using namespace refusion::runtime::presentation;

  const ViewportExtent retina_extent{
      .width_points = 640,
      .height_points = 360,
      .pixels_per_point = 2.0F,
  };
  require(retina_extent.valid());
  require(retina_extent.width_pixels() == 1280);
  require(retina_extent.height_pixels() == 720);
  require(!ViewportExtent{}.valid());

  require(NativeViewportHost{
      .window_system = NativeWindowSystem::cocoa_view,
      .handle = 1,
  }
              .valid());
  require(!NativeViewportHost{}.valid());

  require(NativeFrameTarget{
      .backend = refusion::runtime::gpu::Backend::metal,
      .pixel_format = PixelFormat::bgra8_unorm,
      .texture = 1,
      .width_pixels = 1280,
      .height_pixels = 720,
      .device_generation = 1,
  }
              .valid());

  PresentationTelemetry telemetry;
  require(telemetry.zero_cpu_pixel_transfer());
  telemetry.gpu_readbacks = 1;
  require(!telemetry.zero_cpu_pixel_transfer());

  require(FrameResult{.status = FrameStatus::accepted}.succeeded());
  require(FrameResult{.status = FrameStatus::presented}.succeeded());
  require(!FrameResult{.status = FrameStatus::skipped}.succeeded());
  require(!FrameResult{.status = FrameStatus::rejected}.succeeded());

  const PlaybackSpec playback{
      .duration_ns = 30'000'000'000,
      .frame_rate_numerator = 30,
      .frame_rate_denominator = 1,
      .loop = true,
  };
  require(playback.valid());
  require(playback.frame_interval().count() == 33'333'333);
  require(playback.frame_count() == 900);
  require(playback.frame_at_time(10'000'000'000) == 300);
  require(playback.time_at_frame(300) == 10'000'000'000);
  require(!PlaybackSpec{.duration_ns = 0}.valid());

  const PlaybackSpec ntsc_playback{
      .duration_ns = 10'000'000'000,
      .frame_rate_numerator = 30'000,
      .frame_rate_denominator = 1'001,
      .loop = false,
  };
  require(ntsc_playback.valid());
  require(ntsc_playback.frame_interval().count() == 33'366'667);
  require(ntsc_playback.frame_count() == 300);
  require(ntsc_playback.time_at_frame(1) == 33'366'667);
  require(ntsc_playback.frame_at_time(33'366'666) == 0);
  require(ntsc_playback.frame_at_time(33'366'667) == 1);

  FakePresenter fake_presenter;
  ViewportRenderSession session(fake_presenter, playback);
  require(session.render_once().succeeded());
  require(session.render_once().succeeded());
  require(fake_presenter.last_frame.frame_index == 1);
  require(fake_presenter.last_frame.duration_ns == 30'000'000'000);
  require(session.telemetry().present_submissions == 2);
  session.start_playback();
  fake_presenter.reject_next_frame = true;
  require(session.render_once().status == FrameStatus::rejected);
  require(!session.playback_state().running);
  require(session.playback_state().diagnostic == "injected frame rejection");

  auto fake_now = std::chrono::steady_clock::time_point{};
  FakePresenter transport_presenter;
  ViewportRenderSession transport(transport_presenter, playback,
                                  [&fake_now] { return fake_now; });
  transport.start_playback();
  fake_now += std::chrono::seconds(10);
  require(transport.render_once().succeeded());
  require(transport.playback_state().frame_index == 300);
  require(transport.playback_state().position_ns == 10'000'000'000);
  require(transport.playback_state().clock_epoch_id == 1);
  require(transport.playback_state().clock_source_generation == 1);
  require(transport_presenter.last_frame.transport_epoch_id == 1);

  transport.pause_playback();
  const auto paused = transport.playback_state();
  require(!paused.running);
  require(paused.clock_epoch_id == 2);
  fake_now += std::chrono::seconds(5);
  require(transport.render_once().succeeded());
  require(transport.playback_state().frame_index == paused.frame_index);
  require(transport.playback_state().position_ns == paused.position_ns);

  const auto play_result = transport.submit_transport_command({
      .kind = TransportCommandKind::play,
  });
  require(play_result.accepted);
  fake_now += std::chrono::seconds(1);
  require(transport.render_once().succeeded());
  require(transport.playback_state().frame_index == 330);
  require(transport.playback_state().clock_epoch_id == 3);
  require(transport
              .attach(NativeViewportHost{
                  .window_system = NativeWindowSystem::cocoa_view,
                  .handle = 1,
              })
              .succeeded());

  const auto seek_result = transport.submit_transport_command({
      .kind = TransportCommandKind::seek_to_frame,
      .frame_index = 450,
  });
  require(seek_result.accepted);
  require(seek_result.snapshot.frame_index == 450);
  require(seek_result.snapshot.position_ns == 15'000'000'000);
  require(seek_result.snapshot.clock_epoch_id == 4);
  require(transport_presenter.last_frame.presentation_time_ns ==
          15'000'000'000);
  require(transport_presenter.last_frame.transport_epoch_id == 4);

  const auto pause_result = transport.submit_transport_command({
      .kind = TransportCommandKind::pause,
  });
  require(pause_result.accepted);
  require(!pause_result.snapshot.running);
  require(pause_result.snapshot.clock_epoch_id == 5);
  const auto rejected_seek = transport.submit_transport_command({
      .kind = TransportCommandKind::seek_to_frame,
      .frame_index = 901,
  });
  require(!rejected_seek.accepted);
  require(rejected_seek.diagnostic.find("RFX-TRANSPORT-SEEK-RANGE") !=
          std::string::npos);
}
