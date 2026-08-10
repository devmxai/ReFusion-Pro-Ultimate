#include "StudioTransportBridge.hpp"

#include "refusion/core/VisualContributionRegistry.hpp"

#include <QByteArray>
#include <QHash>
#include <QVariant>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

namespace {

[[nodiscard]] refusion::runtime::presentation::PlaybackSpec playback_spec(
    const refusion::core::CompositionSnapshot& composition) {
  return refusion::runtime::presentation::PlaybackSpec{
      .duration_ns = composition.duration,
      .frame_rate_numerator = composition.frame_rate.numerator,
      .frame_rate_denominator = composition.frame_rate.denominator,
      .loop = true,
  };
}

[[nodiscard]] std::uint64_t range_end(
    const refusion::core::TimeRangeNs& range) noexcept {
  if (range.duration > std::numeric_limits<std::uint64_t>::max() - range.start) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  return range.start + range.duration;
}

[[nodiscard]] std::vector<refusion::core::VisualNodeRef> visible_nodes(
    const refusion::core::CompositionSnapshot& composition,
    const std::optional<refusion::core::LayerGroupId>& focused_group) {
  if (!focused_group) {
    return refusion::core::composition_root_nodes(composition);
  }
  const auto* group =
      refusion::core::find_layer_group(composition, *focused_group);
  return group == nullptr ? std::vector<refusion::core::VisualNodeRef>{}
                          : group->children;
}

[[nodiscard]] bool contains_group(
    const std::vector<refusion::core::VisualNodeRef>& nodes,
    const refusion::core::LayerGroupId& group_id) {
  return std::any_of(nodes.begin(), nodes.end(), [&group_id](const auto& node) {
    const auto* group = std::get_if<refusion::core::LayerGroupId>(&node);
    return group != nullptr && *group == group_id;
  });
}

[[nodiscard]] QString animation_property_name(
    const refusion::core::AnimatedProperty property) {
  using refusion::core::AnimatedProperty;
  switch (property) {
    case AnimatedProperty::position_x:
      return QStringLiteral("Position X");
    case AnimatedProperty::position_y:
      return QStringLiteral("Position Y");
    case AnimatedProperty::scale_x:
      return QStringLiteral("Scale X");
    case AnimatedProperty::scale_y:
      return QStringLiteral("Scale Y");
    case AnimatedProperty::rotation_degrees:
      return QStringLiteral("Rotation");
    case AnimatedProperty::opacity:
      return QStringLiteral("Opacity");
  }
  return QStringLiteral("Unknown");
}

[[nodiscard]] QString animation_property_id(
    const refusion::core::AnimatedProperty property) {
  return QStringLiteral("animation:%1").arg(
      static_cast<unsigned>(property));
}

[[nodiscard]] QString contribution_name(const std::string& kind) {
  const auto* descriptor =
      refusion::core::find_visual_contribution_descriptor(kind);
  return descriptor == nullptr
             ? QStringLiteral("Unsupported Contribution")
             : QString::fromStdString(descriptor->display_name);
}

[[nodiscard]] QString layer_visual_kind(
    const refusion::core::LayerContent& content) {
  return std::holds_alternative<refusion::core::TextLayerContent>(content)
             ? QStringLiteral("text")
             : QStringLiteral("shape");
}

}  // namespace

TimelineTrackModel::TimelineTrackModel(
    const refusion::core::CompositionSnapshot& composition,
    const refusion::runtime::presentation::PlaybackSpec& playback_spec,
    QObject* parent)
    : QAbstractListModel(parent) {
  replaceComposition(composition, playback_spec);
}

void TimelineTrackModel::replaceComposition(
    const refusion::core::CompositionSnapshot& composition,
    const refusion::runtime::presentation::PlaybackSpec& playback_spec,
    std::optional<refusion::core::LayerGroupId> focused_group) {
  publishTracks(prepareTracks(composition, playback_spec, focused_group));
}

std::vector<TimelineTrackModel::Track> TimelineTrackModel::prepareTracks(
    const refusion::core::CompositionSnapshot& composition,
    const refusion::runtime::presentation::PlaybackSpec& playback_spec,
    std::optional<refusion::core::LayerGroupId> focused_group) {
  std::vector<Track> tracks;
  auto nodes = visible_nodes(composition, focused_group);
  tracks.reserve(composition.layers.size() + composition.groups.size() +
                 (focused_group ? 0U : composition.video_clips.size() +
                                             composition.audio_clips.size()));
  const auto append_track = [&](QString node_id, QString display_name,
                                const refusion::core::TimeRangeNs& range,
                                QString row_kind, QString visual_kind,
                                QString owner_node_id,
                                const bool is_group,
                                const bool owner_is_group,
                                const bool is_property_row,
                                const int depth,
                                const qulonglong child_count) {
    const auto clamped_end =
        std::min(range_end(range), playback_spec.duration_ns);
    const auto start_frame = playback_spec.frame_at_time(
        std::min(range.start, playback_spec.duration_ns));
    const auto end_frame = clamped_end >= playback_spec.duration_ns
                               ? playback_spec.frame_count()
                               : playback_spec.frame_at_time(clamped_end);
    tracks.push_back(Track{
        .node_id = std::move(node_id),
        .display_name = std::move(display_name),
        .start_frame = start_frame,
        .end_frame = std::max(start_frame, end_frame),
        .row_kind = std::move(row_kind),
        .visual_kind = std::move(visual_kind),
        .owner_node_id = std::move(owner_node_id),
        .is_group = is_group,
        .owner_is_group = owner_is_group,
        .is_property_row = is_property_row,
        .depth = depth,
        .child_count = child_count,
    });
  };
  const auto append_animations = [&](const QString& owner_id,
                                     const bool owner_is_group,
                                     const refusion::core::TimeRangeNs& range,
                                     const auto& animations) {
    for (const auto& animation : animations) {
      append_track(animation_property_id(animation.property),
                   QStringLiteral("Animate · ") +
                       animation_property_name(animation.property),
                   range, QStringLiteral("animation"),
                   QStringLiteral("animation"), owner_id, false,
                   owner_is_group, true, 1,
                   static_cast<qulonglong>(animation.keyframes.size()));
    }
  };
  // Media Clips are composition-level timeline entities. They are not visual
  // hierarchy nodes and therefore must not be hidden merely because the visual
  // root is empty. Nested visual Group focus intentionally hides these NLE
  // rows; returning to the Composition restores them from accepted truth.
  if (!focused_group) {
    for (const auto& clip : composition.video_clips) {
      const auto clip_id = QString::fromStdString(clip.video_clip_id.value);
      append_track(clip_id, QString::fromStdString(clip.display_name),
                   clip.active_range, QStringLiteral("video_clip"),
                   QStringLiteral("video"), clip_id, false, false, false, 0,
                   0);
    }
    for (const auto& clip : composition.audio_clips) {
      const auto clip_id = QString::fromStdString(clip.audio_clip_id.value);
      append_track(clip_id, QString::fromStdString(clip.display_name),
                   clip.active_range, QStringLiteral("audio_clip"),
                   QStringLiteral("audio"), clip_id, false, false, false, 0,
                   0);
    }
  }
  for (auto node = nodes.rbegin(); node != nodes.rend(); ++node) {
    if (const auto* layer_id = std::get_if<refusion::core::LayerId>(&*node)) {
      const auto* layer = refusion::core::find_layer(composition, *layer_id);
      if (layer == nullptr) {
        continue;
      }
      const auto owner_id = QString::fromStdString(layer->layer_id.value);
      const auto property_count = layer->masks.size() + layer->effects.size() +
                                  layer->animations.size();
      append_track(owner_id, QString::fromStdString(layer->display_name),
                   layer->active_range, QStringLiteral("layer"),
                   layer_visual_kind(layer->content), owner_id, false, false,
                   false, 0,
                   static_cast<qulonglong>(property_count));
      for (const auto& mask : layer->masks) {
        append_track(QString::fromStdString(mask.mask_id.value),
                     QStringLiteral("Mask · ") + contribution_name(
                         refusion::core::visual_mask_kind(mask)),
                     layer->active_range, QStringLiteral("mask"),
                     QStringLiteral("mask"), owner_id, false, false, true, 1,
                     0);
      }
      for (const auto& effect : layer->effects) {
        append_track(QString::fromStdString(effect.effect_id.value),
                     QStringLiteral("FX · ") + contribution_name(
                         refusion::core::visual_effect_kind(effect)),
                     layer->active_range, QStringLiteral("effect"),
                     QStringLiteral("effect"), owner_id, false, false, true,
                     1, 0);
      }
      append_animations(owner_id, false, layer->active_range,
                        layer->animations);
    } else {
      const auto group_id = std::get<refusion::core::LayerGroupId>(*node);
      const auto* group =
          refusion::core::find_layer_group(composition, group_id);
      if (group == nullptr) {
        continue;
      }
      const auto owner_id = QString::fromStdString(group->group_id.value);
      append_track(owner_id, QString::fromStdString(group->display_name),
                   group->active_range, QStringLiteral("group"),
                   QStringLiteral("group"), owner_id, true, true, false, 0,
                   static_cast<qulonglong>(group->children.size()));
      append_animations(owner_id, true, group->active_range,
                        group->animations);
    }
  }
  return tracks;
}

void TimelineTrackModel::publishTracks(std::vector<Track> tracks) noexcept {
  beginResetModel();
  tracks_ = std::move(tracks);
  endResetModel();
}

int TimelineTrackModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(tracks_.size());
}

QVariant TimelineTrackModel::data(const QModelIndex& index, const int role) const {
  if (!index.isValid() || index.row() < 0 ||
      static_cast<std::size_t>(index.row()) >= tracks_.size()) {
    return {};
  }
  const auto& track = tracks_[static_cast<std::size_t>(index.row())];
  switch (role) {
    case nodeIdRole:
      return track.node_id;
    case displayNameRole:
      return track.display_name;
    case startFrameRole:
      return QVariant::fromValue(track.start_frame);
    case endFrameRole:
      return QVariant::fromValue(track.end_frame);
    case durationFramesRole:
      return QVariant::fromValue(track.end_frame - track.start_frame);
    case nodeKindRole:
      return track.row_kind;
    case visualKindRole:
      return track.visual_kind;
    case isGroupRole:
      return track.is_group;
    case childCountRole:
      return QVariant::fromValue(track.child_count);
    case ownerNodeIdRole:
      return track.owner_node_id;
    case ownerIsGroupRole:
      return track.owner_is_group;
    case depthRole:
      return track.depth;
    case isPropertyRowRole:
      return track.is_property_row;
    default:
      return {};
  }
}

QHash<int, QByteArray> TimelineTrackModel::roleNames() const {
  return {
      {nodeIdRole, "nodeId"},
      {displayNameRole, "displayName"},
      {startFrameRole, "startFrame"},
      {endFrameRole, "endFrame"},
      {durationFramesRole, "durationFrames"},
      {nodeKindRole, "nodeKind"},
      {visualKindRole, "visualKind"},
      {isGroupRole, "isGroup"},
      {childCountRole, "childCount"},
      {ownerNodeIdRole, "ownerNodeId"},
      {ownerIsGroupRole, "ownerIsGroup"},
      {depthRole, "depth"},
      {isPropertyRowRole, "isPropertyRow"},
  };
}

StudioTransportBridge::StudioTransportBridge(
    refusion::runtime::presentation::ViewportRenderSession& render_session,
    std::shared_ptr<const refusion::core::CompositionSnapshot> composition,
    QObject* parent)
    : QObject(parent),
      render_session_(render_session),
      playback_spec_(playback_spec(*composition)),
      composition_(std::move(composition)),
      tracks_(*composition_, playback_spec_, this) {}

bool StudioTransportBridge::running() const noexcept {
  return render_session_.playback_state().running;
}

qulonglong StudioTransportBridge::positionFrame() const noexcept {
  return render_session_.playback_state().frame_index;
}

refusion::core::ProjectTimeNs StudioTransportBridge::compositionTimeNs() const
    noexcept {
  const auto state = render_session_.playback_state();
  if (playback_spec_.duration_ns == 0) {
    return 0;
  }
  return std::min(state.position_ns, playback_spec_.duration_ns - 1);
}

qulonglong StudioTransportBridge::durationFrames() const noexcept {
  return playback_spec_.frame_count();
}

double StudioTransportBridge::durationSeconds() const noexcept {
  return static_cast<double>(playback_spec_.duration_ns) / 1'000'000'000.0;
}

double StudioTransportBridge::positionRatio() const noexcept {
  const auto total = durationFrames();
  if (total == 0) {
    return 0.0;
  }
  return std::clamp(static_cast<double>(positionFrame()) /
                        static_cast<double>(total),
                    0.0, 1.0);
}

QString StudioTransportBridge::positionTimecode() const {
  return formatTimecode(positionFrame());
}

QString StudioTransportBridge::durationTimecode() const {
  return formatTimecode(durationFrames());
}

QString StudioTransportBridge::diagnostic() const { return diagnostic_; }

QAbstractItemModel* StudioTransportBridge::tracks() noexcept { return &tracks_; }

QString StudioTransportBridge::timelinePath() const {
  QString path = QString::fromStdString(composition_->display_name);
  for (const auto& group_id : group_focus_) {
    const auto* group =
        refusion::core::find_layer_group(*composition_, group_id);
    if (group != nullptr) {
      path += QStringLiteral(" / ");
      path += QString::fromStdString(group->display_name);
    }
  }
  return path;
}

bool StudioTransportBridge::canNavigateUp() const noexcept {
  return !group_focus_.empty();
}

void StudioTransportBridge::togglePlayback() {
  if (running()) {
    pause();
  } else {
    play();
  }
}

void StudioTransportBridge::play() {
  submit({.kind =
              refusion::runtime::presentation::TransportCommandKind::play});
}

void StudioTransportBridge::pause() {
  submit({.kind =
              refusion::runtime::presentation::TransportCommandKind::pause});
}

void StudioTransportBridge::seekToFrame(const qulonglong frame_index) {
  submit({
      .kind =
          refusion::runtime::presentation::TransportCommandKind::seek_to_frame,
      .frame_index = frame_index,
  });
}

void StudioTransportBridge::seekFromTimelinePosition(const double local_x,
                                                     const double width) {
  if (!std::isfinite(local_x) || !std::isfinite(width) || width <= 0.0) {
    const QString next = QStringLiteral(
        "RFX-TRANSPORT-UI-GEOMETRY: timeline seek geometry is invalid");
    if (diagnostic_ != next) {
      diagnostic_ = next;
      emit diagnosticChanged();
    }
    return;
  }
  const double ratio = std::clamp(local_x / width, 0.0, 1.0);
  const auto frame = static_cast<std::uint64_t>(std::llround(
      ratio * static_cast<double>(playback_spec_.frame_count())));
  seekToFrame(frame);
}

QString StudioTransportBridge::timecodeAtRatio(const double ratio) const {
  if (!std::isfinite(ratio)) {
    return QStringLiteral("00:00:00:00");
  }
  const auto frame = static_cast<std::uint64_t>(std::llround(
      std::clamp(ratio, 0.0, 1.0) *
      static_cast<double>(playback_spec_.frame_count())));
  return formatTimecode(frame);
}

void StudioTransportBridge::enterGroup(const QString& group_id) {
  const refusion::core::LayerGroupId requested{group_id.toStdString()};
  const auto current = visible_nodes(
      *composition_, group_focus_.empty()
                        ? std::optional<refusion::core::LayerGroupId>{}
                        : std::optional<refusion::core::LayerGroupId>{
                              group_focus_.back()});
  if (!contains_group(current, requested)) {
    const QString next = QStringLiteral(
        "RFX-TIMELINE-GROUP-001: requested group is not visible in the current Timeline scope");
    if (diagnostic_ != next) {
      diagnostic_ = next;
      emit diagnosticChanged();
    }
    return;
  }
  group_focus_.push_back(requested);
  tracks_.replaceComposition(*composition_, playback_spec_, requested);
  if (!diagnostic_.isEmpty()) {
    diagnostic_.clear();
    emit diagnosticChanged();
  }
  emit timelineNavigationChanged();
  emit snapshotChanged();
}

void StudioTransportBridge::navigateUp() {
  if (group_focus_.empty()) {
    return;
  }
  group_focus_.pop_back();
  tracks_.replaceComposition(
      *composition_, playback_spec_,
      group_focus_.empty()
          ? std::optional<refusion::core::LayerGroupId>{}
          : std::optional<refusion::core::LayerGroupId>{group_focus_.back()});
  emit timelineNavigationChanged();
  emit snapshotChanged();
}

void StudioTransportBridge::refresh() { emit snapshotChanged(); }

StudioTransportBridge::PreparedCompositionProjection
StudioTransportBridge::prepareComposition(
    std::shared_ptr<const refusion::core::CompositionSnapshot>
        composition) const {
  std::vector<refusion::core::LayerGroupId> valid_focus;
  auto scope = refusion::core::composition_root_nodes(*composition);
  for (const auto& group_id : group_focus_) {
    if (!contains_group(scope, group_id)) {
      break;
    }
    valid_focus.push_back(group_id);
    scope = visible_nodes(*composition, group_id);
  }
  const auto focused_group = valid_focus.empty()
          ? std::optional<refusion::core::LayerGroupId>{}
          : std::optional<refusion::core::LayerGroupId>{valid_focus.back()};
  auto tracks = TimelineTrackModel::prepareTracks(
      *composition, playback_spec_, focused_group);
  return PreparedCompositionProjection{
      .composition = std::move(composition),
      .group_focus = std::move(valid_focus),
      .tracks = std::move(tracks),
  };
}

void StudioTransportBridge::publishComposition(
    PreparedCompositionProjection projection) noexcept {
  composition_ = std::move(projection.composition);
  group_focus_ = std::move(projection.group_focus);
  tracks_.publishTracks(std::move(projection.tracks));
  // The active path text can change when an Agent renames the Composition or a
  // focused Group even if the focus IDs remain valid.
  emit timelineNavigationChanged();
  emit snapshotChanged();
}

void StudioTransportBridge::submit(
    const refusion::runtime::presentation::TransportCommand command) {
  const auto result = render_session_.submit_transport_command(command);
  const QString next_diagnostic = result.accepted
                                      ? QString{}
                                      : QString::fromStdString(
                                            result.code + ": " +
                                            result.diagnostic);
  if (diagnostic_ != next_diagnostic) {
    diagnostic_ = next_diagnostic;
    emit diagnosticChanged();
  }
  emit snapshotChanged();
}

QString StudioTransportBridge::formatTimecode(
    const std::uint64_t frame_index) const {
  const auto numerator =
      static_cast<std::uint64_t>(playback_spec_.frame_rate_numerator);
  const auto denominator =
      static_cast<std::uint64_t>(playback_spec_.frame_rate_denominator);
  const auto nominal_fps = denominator == 0
                               ? 0
                               : (numerator + denominator / 2) / denominator;
  if (nominal_fps == 0) {
    return QStringLiteral("00:00:00:00");
  }
  const auto total_seconds = frame_index / nominal_fps;
  const auto frames = frame_index % nominal_fps;
  const auto seconds = total_seconds % 60;
  const auto total_minutes = total_seconds / 60;
  const auto minutes = total_minutes % 60;
  const auto hours = total_minutes / 60;
  return QStringLiteral("%1:%2:%3:%4")
      .arg(hours, 2, 10, QLatin1Char('0'))
      .arg(minutes, 2, 10, QLatin1Char('0'))
      .arg(seconds, 2, 10, QLatin1Char('0'))
      .arg(frames, 2, 10, QLatin1Char('0'));
}
