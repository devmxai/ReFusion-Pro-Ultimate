#include "StudioBridge.hpp"
#include "StudioTransportBridge.hpp"

#include "refusion/application/ProjectCommandService.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <QCoreApplication>

#include <chrono>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("Studio bridge test requirement failed");
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
      const refusion::runtime::presentation::FixtureFrame&) override {
    return {.status = refusion::runtime::presentation::FrameStatus::presented};
  }
  [[nodiscard]] refusion::runtime::presentation::PresentationTelemetry telemetry()
      const noexcept override {
    return {};
  }
};

[[nodiscard]] refusion::core::CompositionSnapshot timeline_composition() {
  using namespace refusion::core;
  std::vector<LayerSnapshot> layers;
  layers.push_back(LayerSnapshot{
      .layer_id = LayerId{"lyr_background"},
      .display_name = "Background",
      .active_range = {.start = 0, .duration = 30'000'000'000},
      .content = ShapeLayerContent{},
  });
  layers.push_back(LayerSnapshot{
      .layer_id = LayerId{"lyr_title"},
      .display_name = "Title",
      .active_range = {.start = 5'000'000'000, .duration = 10'000'000'000},
      .content = TextLayerContent{},
  });
  return CompositionSnapshot{
      .composition_id = CompositionId{"cmp_transport"},
      .display_name = "Transport",
      .canvas = {.width_pixels = 1080, .height_pixels = 1920},
      .frame_rate = {.numerator = 30, .denominator = 1},
      .duration = 30'000'000'000,
      .layers = std::move(layers),
  };
}

}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  auto commands = refusion::application::create_application_host(
      refusion::core::ProjectSnapshot{
          .project_id = refusion::core::ProjectId{"prj_studio_bridge"},
          .revision_id = refusion::core::RevisionId{3},
          .display_name = "Before",
      });
  StudioBridge bridge(*commands);

  int snapshot_notifications = 0;
  int diagnostic_notifications = 0;
  QObject::connect(&bridge, &StudioBridge::snapshotChanged,
                   [&snapshot_notifications] { ++snapshot_notifications; });
  QObject::connect(&bridge, &StudioBridge::diagnosticChanged,
                   [&diagnostic_notifications] { ++diagnostic_notifications; });

  bridge.submitRename(QStringLiteral("After"));
  require(bridge.projectId() == QStringLiteral("prj_studio_bridge"));
  require(bridge.projectName() == QStringLiteral("After"));
  require(bridge.revision() == 4);
  require(bridge.diagnostic().isEmpty());
  require(snapshot_notifications == 1);
  require(diagnostic_notifications == 1);

  bridge.submitRename(QStringLiteral("   "));
  require(bridge.projectName() == QStringLiteral("After"));
  require(bridge.revision() == 4);
  require(bridge.diagnostic().startsWith(QStringLiteral("RFX-SCHEMA-001")));
  require(snapshot_notifications == 1);
  require(diagnostic_notifications == 2);

  auto fake_now = std::chrono::steady_clock::time_point{};
  FakePresenter presenter;
  refusion::runtime::presentation::ViewportRenderSession render_session(
      presenter,
      {
          .duration_ns = 30'000'000'000,
          .frame_rate_numerator = 30,
          .frame_rate_denominator = 1,
          .loop = true,
      },
      [&fake_now] { return fake_now; });
  StudioTransportBridge transport_bridge(render_session, timeline_composition());
  require(!transport_bridge.running());
  require(transport_bridge.durationFrames() == 900);
  require(transport_bridge.positionFrame() == 0);
  require(transport_bridge.positionTimecode() ==
          QStringLiteral("00:00:00:00"));
  require(transport_bridge.durationTimecode() ==
          QStringLiteral("00:00:30:00"));
  require(transport_bridge.timecodeAtRatio(0.5) ==
          QStringLiteral("00:00:15:00"));
  require(transport_bridge.tracks()->rowCount() == 2);
  const auto title_index = transport_bridge.tracks()->index(0, 0);
  require(title_index.data(TimelineTrackModel::layerIdRole).toString() ==
          QStringLiteral("lyr_title"));
  require(title_index.data(TimelineTrackModel::startFrameRole).toULongLong() ==
          150);
  require(title_index.data(TimelineTrackModel::durationFramesRole).toULongLong() ==
          300);

  int transport_notifications = 0;
  QObject::connect(&transport_bridge, &StudioTransportBridge::snapshotChanged,
                   [&transport_notifications] { ++transport_notifications; });
  transport_bridge.play();
  require(transport_bridge.running());
  fake_now += std::chrono::seconds(4);
  require(render_session.render_once().succeeded());
  require(transport_bridge.positionFrame() == 120);
  transport_bridge.pause();
  require(!transport_bridge.running());
  const auto paused_frame = transport_bridge.positionFrame();
  fake_now += std::chrono::seconds(3);
  require(render_session.render_once().succeeded());
  require(transport_bridge.positionFrame() == paused_frame);

  transport_bridge.seekFromTimelinePosition(50.0, 100.0);
  require(transport_bridge.positionFrame() == 450);
  require(transport_bridge.positionTimecode() ==
          QStringLiteral("00:00:15:00"));
  transport_bridge.togglePlayback();
  require(transport_bridge.running());
  require(transport_notifications >= 4);

  transport_bridge.seekFromTimelinePosition(10.0, 0.0);
  require(transport_bridge.diagnostic().startsWith(
      QStringLiteral("RFX-TRANSPORT-UI-GEOMETRY")));
}
