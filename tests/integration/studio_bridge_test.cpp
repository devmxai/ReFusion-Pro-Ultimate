#include "StudioBridge.hpp"
#include "StudioTransportBridge.hpp"

#include "refusion/application/ProjectCommandService.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <QCoreApplication>

#include <algorithm>
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
  [[nodiscard]] refusion::runtime::gpu::DeviceIdentity device_identity()
      const noexcept override {
    return {.backend = refusion::runtime::gpu::Backend::metal,
            .adapter_name = "fake",
            .adapter_id = 1,
            .generation = 1};
  }
  [[nodiscard]] refusion::runtime::presentation::FrameResult attach(
      refusion::runtime::presentation::NativeViewportHostLease) override {
    return {.status = refusion::runtime::presentation::FrameStatus::accepted};
  }
  void detach() noexcept override {}
  [[nodiscard]] refusion::runtime::presentation::FrameResult resize(
      refusion::runtime::presentation::ViewportExtent) override {
    return {.status = refusion::runtime::presentation::FrameStatus::accepted};
  }
  void set_visible(bool) noexcept override {}
  [[nodiscard]] refusion::runtime::presentation::FrameResult present(
      const refusion::runtime::presentation::PresentationFrameRequest&)
      override {
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
      .content = ShapeLayerContent{
          .width = 1080.0,
          .height = 1920.0,
          .fill = ColorRgba8{.red = 5, .green = 6, .blue = 10},
      },
  });
  layers.push_back(LayerSnapshot{
      .layer_id = LayerId{"lyr_title"},
      .display_name = "Title",
      .active_range = {.start = 5'000'000'000, .duration = 10'000'000'000},
      .transform = {.position_x = 100.0, .position_y = 200.0},
      .content = TextLayerContent{
          .text = "Title",
          .font = FontIdentity{.family_name = "Inter"},
          .font_size = 72.0,
          .box = TextBox{.width = 900.0, .height = 100.0},
      },
  });
  return CompositionSnapshot{
      .composition_id = CompositionId{"cmp_transport"},
      .display_name = "Transport",
      .canvas = {.width_pixels = 1080, .height_pixels = 1920},
      .frame_rate = {.numerator = 30, .denominator = 1},
      .duration = 30'000'000'000,
      .layers = std::move(layers),
      .groups = {
          LayerGroupSnapshot{
              .group_id = LayerGroupId{"grp_title"},
              .display_name = "Title Group",
              .active_range = {
                  .start = 0,
                  .duration = 30'000'000'000,
              },
              .children = {LayerId{"lyr_title"}},
          },
      },
      .root_nodes = {
          LayerId{"lyr_background"},
          LayerGroupId{"grp_title"},
      },
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
          .composition = timeline_composition(),
      });
  StudioBridge bridge(*commands);
  require(bridge.compositionWidth() == 1080U);
  require(bridge.compositionHeight() == 1920U);
  require(bridge.portraitWorkspace());

  auto landscape = timeline_composition();
  landscape.canvas = {.width_pixels = 1920, .height_pixels = 1080};
  auto landscape_commands = refusion::application::create_application_host(
      refusion::core::ProjectSnapshot{
          .project_id = refusion::core::ProjectId{"prj_landscape_workspace"},
          .revision_id = refusion::core::RevisionId{1},
          .display_name = "Landscape",
          .composition = std::move(landscape),
      });
  StudioBridge landscape_bridge(*landscape_commands);
  require(landscape_bridge.compositionWidth() == 1920U);
  require(landscape_bridge.compositionHeight() == 1080U);
  require(!landscape_bridge.portraitWorkspace());

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

  bridge.selectVisualNode(QStringLiteral("lyr_title"), false);
  require(bridge.hasVisualSelection());
  require(bridge.selectedNodeId() == QStringLiteral("lyr_title"));
  require(bridge.selectedNodeKind() == QStringLiteral("Text"));
  require(bridge.selectedDisplayName() == QStringLiteral("Title"));
  require(bridge.selectedProperties().size() == 30);
  const auto initial_text_properties = bridge.selectedProperties();
  const auto font_source = std::find_if(
      initial_text_properties.begin(), initial_text_properties.end(),
      [](const QVariant& item) {
        return item.toMap().value(QStringLiteral("id")).toString() ==
               QStringLiteral("text.font.source");
      });
  require(font_source != initial_text_properties.end());
  require(!font_source->toMap().value(QStringLiteral("writable")).toBool());
  require(font_source->toMap().value(QStringLiteral("value")).toString() ==
          QStringLiteral("system_family"));
  require(snapshot_notifications == 2);

  int accepted_visual_snapshots = 0;
  bridge.setAcceptedObserver(
      [&accepted_visual_snapshots](const refusion::core::ProjectSnapshot&) {
        ++accepted_visual_snapshots;
      });
  bridge.submitSelectedTransform(600.0, 800.0, 10.0, 20.0, 1.2, 0.8,
                                 15.0, 0.75);
  require(bridge.revision() == 5);
  require(bridge.selectedPositionX() == 600.0);
  require(bridge.selectedPositionY() == 800.0);
  require(bridge.selectedAnchorX() == 10.0);
  require(bridge.selectedAnchorY() == 20.0);
  require(bridge.selectedScaleX() == 1.2);
  require(bridge.selectedScaleY() == 0.8);
  require(bridge.selectedRotation() == 15.0);
  require(bridge.selectedOpacity() == 0.75);
  require(bridge.diagnostic().isEmpty());
  require(accepted_visual_snapshots == 1);
  require(snapshot_notifications == 3);

  bridge.submitSelectedTransform(600.0, 800.0, 10.0, 20.0, 0.0, 0.8,
                                 15.0, 0.75);
  require(bridge.revision() == 5);
  require(bridge.diagnostic().startsWith(
      QStringLiteral("RFX-PROJECT-110")));
  require(accepted_visual_snapshots == 1);
  require(snapshot_notifications == 3);

  bridge.selectVisualNode(QStringLiteral("grp_title"), true);
  require(bridge.hasVisualSelection());
  require(bridge.selectedNodeKind() == QStringLiteral("Group"));
  require(snapshot_notifications == 4);
  bridge.submitSelectedTransform(40.0, 50.0, 5.0, 6.0, 1.0, 1.0,
                                 22.0, 1.0);
  require(bridge.revision() == 6);
  require(bridge.selectedRotation() == 22.0);
  require(accepted_visual_snapshots == 2);
  require(snapshot_notifications == 5);

  bridge.selectVisualNode(QStringLiteral("lyr_title"), false);
  require(snapshot_notifications == 6);
  bridge.submitSelectedProperty(QStringLiteral("text.fill"),
                                QStringLiteral("#FF2040FF"));
  require(bridge.revision() == 7);
  require(accepted_visual_snapshots == 3);
  require(snapshot_notifications == 7);
  const auto title_properties = bridge.selectedProperties();
  const auto fill = std::find_if(
      title_properties.begin(), title_properties.end(), [](const QVariant& item) {
        return item.toMap().value(QStringLiteral("id")).toString() ==
               QStringLiteral("text.fill");
      });
  require(fill != title_properties.end());
  require(fill->toMap().value(QStringLiteral("value")).toString() ==
          QStringLiteral("#FF2040FF"));

  bridge.submitSelectedProperty(QStringLiteral("text.fill"),
                                QStringLiteral("not-a-color"));
  require(bridge.revision() == 7);
  require(bridge.diagnostic().startsWith(
      QStringLiteral("RFX-PROPERTY-INPUT-400")));
  require(accepted_visual_snapshots == 3);
  require(snapshot_notifications == 7);

  require(bridge.selectedEffects().isEmpty());
  const auto available_effects = bridge.availableEffects();
  require(available_effects.size() == 3);
  const auto available_glow = std::find_if(
      available_effects.begin(), available_effects.end(),
      [](const QVariant& item) {
        return item.toMap().value(QStringLiteral("kind")).toString() ==
               QStringLiteral("glow");
      });
  require(available_glow != available_effects.end());
  require(available_glow->toMap()
              .value(QStringLiteral("capabilityId"))
              .toString() == QStringLiteral("visual.fx.glow.v1"));
  require(available_glow->toMap()
              .value(QStringLiteral("parameters"))
              .toList()
              .size() == 2);
  bridge.addSelectedEffect(QStringLiteral("glow"));
  require(bridge.revision() == 8);
  require(accepted_visual_snapshots == 4);
  require(bridge.selectedEffects().size() == 1);
  const auto glow_effect = bridge.selectedEffects().front().toMap();
  require(glow_effect.value(QStringLiteral("kind")).toString() ==
          QStringLiteral("glow"));
  const auto glow_id = glow_effect.value(QStringLiteral("id")).toString();

  bridge.updateSelectedEffect(
      glow_id, false,
      QVariantMap{
          {QStringLiteral("sigma"), QStringLiteral("24")},
          {QStringLiteral("color"), QStringLiteral("#20D0FFFF")},
      });
  require(bridge.revision() == 9);
  require(accepted_visual_snapshots == 5);
  const auto edited_glow = bridge.selectedEffects().front().toMap();
  require(!edited_glow.value(QStringLiteral("enabled")).toBool());
  require(edited_glow.value(QStringLiteral("sigma")).toDouble() == 24.0);
  require(edited_glow.value(QStringLiteral("color")).toString() ==
          QStringLiteral("#20D0FFFF"));

  bridge.updateSelectedEffect(
      glow_id, true,
      QVariantMap{
          {QStringLiteral("sigma"), QStringLiteral("not-a-number")},
          {QStringLiteral("color"), QStringLiteral("#20D0FFFF")},
      });
  require(bridge.revision() == 9);
  require(bridge.diagnostic().startsWith(
      QStringLiteral("RFX-EFFECT-INPUT-400")));
  require(accepted_visual_snapshots == 5);

  bridge.removeSelectedEffect(glow_id);
  require(bridge.revision() == 10);
  require(accepted_visual_snapshots == 6);
  require(bridge.selectedEffects().isEmpty());

  bridge.selectVisualNode(QStringLiteral("lyr_background"), false);
  require(bridge.selectedShapeFill().value(QStringLiteral("kind")).toString() ==
          QStringLiteral("solid"));
  bridge.submitSelectedShapeFill(
      QStringLiteral("linear_gradient"),
      QVariantMap{
          {QStringLiteral("colorA"), QStringLiteral("#101A40FF")},
          {QStringLiteral("colorB"), QStringLiteral("#20D0FFFF")},
          {QStringLiteral("startX"), QStringLiteral("-540")},
          {QStringLiteral("startY"), QStringLiteral("-960")},
          {QStringLiteral("endX"), QStringLiteral("540")},
          {QStringLiteral("endY"), QStringLiteral("960")},
      });
  require(bridge.revision() == 11);
  require(accepted_visual_snapshots == 7);
  require(bridge.selectedShapeFill().value(QStringLiteral("kind")).toString() ==
          QStringLiteral("linear_gradient"));
  require(bridge.selectedShapeFill().value(QStringLiteral("colorB")).toString() ==
          QStringLiteral("#20D0FFFF"));

  require(bridge.selectedMasks().isEmpty());
  require(bridge.availableMasks().size() == 1);
  require(bridge.availableMasks().front().toMap()
              .value(QStringLiteral("capabilityId"))
              .toString() == QStringLiteral("visual.mask.rounded_rect.v1"));
  bridge.addSelectedMask(QStringLiteral("rounded_rect"));
  require(bridge.revision() == 12);
  require(accepted_visual_snapshots == 8);
  require(bridge.selectedMasks().size() == 1);
  const auto mask = bridge.selectedMasks().front().toMap();
  const auto mask_id = mask.value(QStringLiteral("id")).toString();
  bridge.updateSelectedMask(
      mask_id, true, true,
      QVariantMap{
          {QStringLiteral("positionX"), QStringLiteral("20")},
          {QStringLiteral("positionY"), QStringLiteral("30")},
          {QStringLiteral("width"), QStringLiteral("900")},
          {QStringLiteral("height"), QStringLiteral("1600")},
          {QStringLiteral("cornerRadius"), QStringLiteral("80")},
      });
  require(bridge.revision() == 13);
  require(accepted_visual_snapshots == 9);
  const auto edited_mask = bridge.selectedMasks().front().toMap();
  require(edited_mask.value(QStringLiteral("inverted")).toBool());
  require(edited_mask.value(QStringLiteral("width")).toDouble() == 900.0);
  bridge.removeSelectedMask(mask_id);
  require(bridge.revision() == 14);
  require(accepted_visual_snapshots == 10);
  require(bridge.selectedMasks().isEmpty());

  bridge.addVisualLayer(QStringLiteral("SHP"));
  require(bridge.revision() == 15);
  require(accepted_visual_snapshots == 11);
  require(bridge.selectedNodeKind() == QStringLiteral("Shape"));
  require(commands->active_snapshot().composition->layers.size() == 3);
  bridge.addVisualLayer(QStringLiteral("VID"));
  require(bridge.revision() == 15);
  require(bridge.diagnostic().startsWith(
      QStringLiteral("RFX-LAYER-PRESET-501")));

  auto alignment_commands = refusion::application::create_application_host(
      refusion::core::ProjectSnapshot{
          .project_id = refusion::core::ProjectId{"prj_studio_alignment"},
          .revision_id = refusion::core::RevisionId{3},
          .display_name = "Alignment",
          .composition = timeline_composition(),
      });
  StudioBridge alignment_bridge(*alignment_commands);
  alignment_bridge.setCompositionTimeProvider([] {
    return refusion::core::ProjectTimeNs{6'000'000'000};
  });
  alignment_bridge.selectVisualNode(QStringLiteral("lyr_title"), false);
  require(alignment_bridge.selectedMeasuredBounds()
              .value(QStringLiteral("available"))
              .toBool());
  require(alignment_bridge.selectedMeasuredBounds()
              .value(QStringLiteral("geometry"))
              .toMap()
              .value(QStringLiteral("left"))
              .toDouble() == -350.0);
  require(alignment_bridge.alignmentTargets().size() == 1);
  alignment_bridge.submitSelectedAlignment(
      QStringLiteral("lyr_background"), false, QStringLiteral("Center"),
      QStringLiteral("Center"), QStringLiteral("Geometry"));
  require(alignment_bridge.revision() == 4);
  require(alignment_bridge.selectedPositionX() == 0.0);
  require(alignment_bridge.selectedPositionY() == 0.0);
  alignment_bridge.submitSelectedAlignment(
      QStringLiteral("lyr_background"), false, QStringLiteral("Center"),
      QStringLiteral("None"), QStringLiteral("Logical"));
  require(alignment_bridge.revision() == 4);
  require(alignment_bridge.diagnostic().startsWith(
      QStringLiteral("RFX-MEASURE-PORT-001")));

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
  StudioTransportBridge transport_bridge(
      render_session,
      std::make_shared<const refusion::core::CompositionSnapshot>(
          timeline_composition()));
  require(!transport_bridge.running());
  require(transport_bridge.durationFrames() == 900);
  require(transport_bridge.durationSeconds() == 30.0);
  require(transport_bridge.positionFrame() == 0);
  require(transport_bridge.positionTimecode() ==
          QStringLiteral("00:00:00:00"));
  require(transport_bridge.durationTimecode() ==
          QStringLiteral("00:00:30:00"));
  require(transport_bridge.timecodeAtRatio(0.5) ==
          QStringLiteral("00:00:15:00"));
  require(transport_bridge.tracks()->rowCount() == 2);
  const auto group_index = transport_bridge.tracks()->index(0, 0);
  require(group_index.data(TimelineTrackModel::nodeIdRole).toString() ==
          QStringLiteral("grp_title"));
  require(group_index.data(TimelineTrackModel::isGroupRole).toBool());
  require(group_index.data(TimelineTrackModel::visualKindRole).toString() ==
          QStringLiteral("group"));
  require(group_index.data(TimelineTrackModel::childCountRole).toULongLong() ==
          1);
  require(transport_bridge.timelinePath() == QStringLiteral("Transport"));
  require(!transport_bridge.canNavigateUp());

  auto nested_projection = timeline_composition();
  auto& projected_title = nested_projection.layers.back();
  projected_title.masks = {
      refusion::core::LayerMask{
          .mask_id = refusion::core::MaskId{"mask_title"},
          .geometry = {.width = 800.0, .height = 90.0},
      },
  };
  projected_title.effects = {
      refusion::core::LayerEffect{
          .effect_id = refusion::core::EffectId{"fx_title_glow"},
          .parameters = refusion::core::GlowEffect{.sigma = 12.0},
      },
  };
  projected_title.animations = {
      refusion::core::ScalarAnimation{
          .property = refusion::core::AnimatedProperty::position_x,
          .keyframes = {{.time = 5'000'000'000, .value = 100.0},
                        {.time = 15'000'000'000, .value = 300.0}},
      },
  };
  StudioTransportBridge nested_bridge(
      render_session,
      std::make_shared<const refusion::core::CompositionSnapshot>(
          nested_projection));
  nested_bridge.enterGroup(QStringLiteral("grp_title"));
  require(nested_bridge.tracks()->rowCount() == 4);
  const auto mask_lane = nested_bridge.tracks()->index(1, 0);
  const auto effect_lane = nested_bridge.tracks()->index(2, 0);
  const auto animation_lane = nested_bridge.tracks()->index(3, 0);
  require(mask_lane.data(TimelineTrackModel::nodeKindRole).toString() ==
          QStringLiteral("mask"));
  require(effect_lane.data(TimelineTrackModel::nodeKindRole).toString() ==
          QStringLiteral("effect"));
  require(animation_lane.data(TimelineTrackModel::nodeKindRole).toString() ==
          QStringLiteral("animation"));
  require(effect_lane.data(TimelineTrackModel::ownerNodeIdRole).toString() ==
          QStringLiteral("lyr_title"));
  require(effect_lane.data(TimelineTrackModel::isPropertyRowRole).toBool());
  require(effect_lane.data(TimelineTrackModel::depthRole).toInt() == 1);

  transport_bridge.enterGroup(QStringLiteral("grp_title"));
  require(transport_bridge.tracks()->rowCount() == 1);
  require(transport_bridge.canNavigateUp());
  require(transport_bridge.timelinePath() ==
          QStringLiteral("Transport / Title Group"));
  const auto title_index = transport_bridge.tracks()->index(0, 0);
  require(title_index.data(TimelineTrackModel::nodeIdRole).toString() ==
          QStringLiteral("lyr_title"));
  require(title_index.data(TimelineTrackModel::visualKindRole).toString() ==
          QStringLiteral("text"));
  require(title_index.data(TimelineTrackModel::startFrameRole).toULongLong() ==
          150);
  require(title_index.data(TimelineTrackModel::durationFramesRole).toULongLong() ==
          300);

  int navigation_notifications = 0;
  QObject::connect(
      &transport_bridge, &StudioTransportBridge::timelineNavigationChanged,
      [&navigation_notifications] { ++navigation_notifications; });
  auto replaced_composition = timeline_composition();
  replaced_composition.display_name = "Agent-updated composition";
  replaced_composition.groups.front().display_name = "Agent-updated group";
  replaced_composition.layers.back().display_name = "Agent-updated title";
  replaced_composition.layers.back().active_range = {
      .start = 10'000'000'000,
      .duration = 5'000'000'000,
  };
  auto prepared_projection = transport_bridge.prepareComposition(
      std::make_shared<const refusion::core::CompositionSnapshot>(
          replaced_composition));
  transport_bridge.publishComposition(std::move(prepared_projection));
  const auto replaced_title_index = transport_bridge.tracks()->index(0, 0);
  require(replaced_title_index.data(TimelineTrackModel::displayNameRole).toString() ==
          QStringLiteral("Agent-updated title"));
  require(replaced_title_index.data(TimelineTrackModel::startFrameRole).toULongLong() ==
          300);
  require(replaced_title_index.data(TimelineTrackModel::durationFramesRole).toULongLong() ==
          150);
  require(transport_bridge.timelinePath() == QStringLiteral(
              "Agent-updated composition / Agent-updated group"));
  require(navigation_notifications == 1);
  transport_bridge.navigateUp();
  require(!transport_bridge.canNavigateUp());
  require(transport_bridge.tracks()->rowCount() == 2);

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
