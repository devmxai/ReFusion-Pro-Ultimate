#include "StudioTransportBridge.hpp"

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

}  // namespace

TimelineTrackModel::TimelineTrackModel(
    const refusion::core::CompositionSnapshot& composition,
    const refusion::runtime::presentation::PlaybackSpec& playback_spec,
    QObject* parent)
    : QAbstractListModel(parent) {
  tracks_.reserve(composition.layers.size());
  for (auto layer = composition.layers.rbegin();
       layer != composition.layers.rend(); ++layer) {
    const auto clamped_end = std::min(range_end(layer->active_range),
                                      playback_spec.duration_ns);
    const auto start_frame = playback_spec.frame_at_time(
        std::min(layer->active_range.start, playback_spec.duration_ns));
    const auto end_frame = clamped_end >= playback_spec.duration_ns
                               ? playback_spec.frame_count()
                               : playback_spec.frame_at_time(clamped_end);
    tracks_.push_back(Track{
        .layer_id = QString::fromStdString(layer->layer_id.value),
        .display_name = QString::fromStdString(layer->display_name),
        .start_frame = start_frame,
        .end_frame = std::max(start_frame, end_frame),
    });
  }
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
    case layerIdRole:
      return track.layer_id;
    case displayNameRole:
      return track.display_name;
    case startFrameRole:
      return QVariant::fromValue(track.start_frame);
    case endFrameRole:
      return QVariant::fromValue(track.end_frame);
    case durationFramesRole:
      return QVariant::fromValue(track.end_frame - track.start_frame);
    default:
      return {};
  }
}

QHash<int, QByteArray> TimelineTrackModel::roleNames() const {
  return {
      {layerIdRole, "layerId"},
      {displayNameRole, "displayName"},
      {startFrameRole, "startFrame"},
      {endFrameRole, "endFrame"},
      {durationFramesRole, "durationFrames"},
  };
}

StudioTransportBridge::StudioTransportBridge(
    refusion::runtime::presentation::ViewportRenderSession& render_session,
    refusion::core::CompositionSnapshot composition,
    QObject* parent)
    : QObject(parent),
      render_session_(render_session),
      playback_spec_(playback_spec(composition)),
      tracks_(composition, playback_spec_, this) {}

bool StudioTransportBridge::running() const noexcept {
  return render_session_.playback_state().running;
}

qulonglong StudioTransportBridge::positionFrame() const noexcept {
  return render_session_.playback_state().frame_index;
}

qulonglong StudioTransportBridge::durationFrames() const noexcept {
  return playback_spec_.frame_count();
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

void StudioTransportBridge::refresh() { emit snapshotChanged(); }

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
