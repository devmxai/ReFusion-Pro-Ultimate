#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("viewport presentation test requirement failed");
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
    ++state.present_submissions;
    return {.status = refusion::runtime::presentation::FrameStatus::presented};
  }
  [[nodiscard]] refusion::runtime::presentation::PresentationTelemetry telemetry()
      const noexcept override {
    return state;
  }

  refusion::runtime::presentation::FixtureFrame last_frame;
  refusion::runtime::presentation::PresentationTelemetry state;
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
  }.valid());
  require(!NativeViewportHost{}.valid());

  require(NativeFrameTarget{
      .backend = refusion::runtime::gpu::Backend::metal,
      .pixel_format = PixelFormat::bgra8_unorm,
      .texture = 1,
      .width_pixels = 1280,
      .height_pixels = 720,
      .device_generation = 1,
  }.valid());

  PresentationTelemetry telemetry;
  require(telemetry.zero_cpu_pixel_transfer());
  telemetry.gpu_readbacks = 1;
  require(!telemetry.zero_cpu_pixel_transfer());

  require(FrameResult{.status = FrameStatus::accepted}.succeeded());
  require(FrameResult{.status = FrameStatus::presented}.succeeded());
  require(!FrameResult{.status = FrameStatus::skipped}.succeeded());
  require(!FrameResult{.status = FrameStatus::rejected}.succeeded());

  FakePresenter fake_presenter;
  ViewportRenderSession session(fake_presenter);
  require(session.render_once().succeeded());
  require(session.render_once().succeeded());
  require(fake_presenter.last_frame.frame_index == 1);
  require(session.telemetry().present_submissions == 2);
}
