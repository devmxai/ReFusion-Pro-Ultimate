#pragma once

#include "refusion/core/ProjectDocument.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <QAbstractListModel>
#include <QObject>
#include <QString>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

class TimelineTrackModel final : public QAbstractListModel {
  Q_OBJECT

 public:
  struct Track final {
    QString node_id;
    QString display_name;
    qulonglong start_frame{0};
    qulonglong end_frame{0};
    QString row_kind;
    QString visual_kind;
    QString owner_node_id;
    bool is_group{false};
    bool owner_is_group{false};
    bool is_property_row{false};
    int depth{0};
    qulonglong child_count{0};
  };

  enum Role : int {
    nodeIdRole = Qt::UserRole + 1,
    displayNameRole,
    startFrameRole,
    endFrameRole,
    durationFramesRole,
    nodeKindRole,
    visualKindRole,
    isGroupRole,
    childCountRole,
    ownerNodeIdRole,
    ownerIsGroupRole,
    depthRole,
    isPropertyRowRole,
  };

  TimelineTrackModel(
      const refusion::core::CompositionSnapshot& composition,
      const refusion::runtime::presentation::PlaybackSpec& playback_spec,
      QObject* parent = nullptr);

  [[nodiscard]] int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
  void replaceComposition(
      const refusion::core::CompositionSnapshot& composition,
      const refusion::runtime::presentation::PlaybackSpec& playback_spec,
      std::optional<refusion::core::LayerGroupId> focused_group = std::nullopt);
  [[nodiscard]] static std::vector<Track> prepareTracks(
      const refusion::core::CompositionSnapshot& composition,
      const refusion::runtime::presentation::PlaybackSpec& playback_spec,
      std::optional<refusion::core::LayerGroupId> focused_group = std::nullopt);
  void publishTracks(std::vector<Track> tracks) noexcept;

 private:
  std::vector<Track> tracks_;
};

class StudioTransportBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool running READ running NOTIFY snapshotChanged)
  Q_PROPERTY(qulonglong positionFrame READ positionFrame NOTIFY snapshotChanged)
  Q_PROPERTY(qulonglong durationFrames READ durationFrames CONSTANT)
  Q_PROPERTY(double durationSeconds READ durationSeconds CONSTANT)
  Q_PROPERTY(double positionRatio READ positionRatio NOTIFY snapshotChanged)
  Q_PROPERTY(QString positionTimecode READ positionTimecode NOTIFY snapshotChanged)
  Q_PROPERTY(QString durationTimecode READ durationTimecode CONSTANT)
  Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY diagnosticChanged)
  Q_PROPERTY(QAbstractItemModel* tracks READ tracks CONSTANT)
  Q_PROPERTY(QString timelinePath READ timelinePath NOTIFY timelineNavigationChanged)
  Q_PROPERTY(bool canNavigateUp READ canNavigateUp NOTIFY timelineNavigationChanged)

 public:
  struct PreparedCompositionProjection final {
    std::shared_ptr<const refusion::core::CompositionSnapshot> composition;
    std::vector<refusion::core::LayerGroupId> group_focus;
    std::vector<TimelineTrackModel::Track> tracks;
  };

  StudioTransportBridge(
      refusion::runtime::presentation::ViewportRenderSession& render_session,
      std::shared_ptr<const refusion::core::CompositionSnapshot> composition,
      QObject* parent = nullptr);

  [[nodiscard]] bool running() const noexcept;
  [[nodiscard]] qulonglong positionFrame() const noexcept;
  [[nodiscard]] refusion::core::ProjectTimeNs compositionTimeNs() const
      noexcept;
  [[nodiscard]] qulonglong durationFrames() const noexcept;
  [[nodiscard]] double durationSeconds() const noexcept;
  [[nodiscard]] double positionRatio() const noexcept;
  [[nodiscard]] QString positionTimecode() const;
  [[nodiscard]] QString durationTimecode() const;
  [[nodiscard]] QString diagnostic() const;
  [[nodiscard]] QAbstractItemModel* tracks() noexcept;
  [[nodiscard]] QString timelinePath() const;
  [[nodiscard]] bool canNavigateUp() const noexcept;

  Q_INVOKABLE void togglePlayback();
  Q_INVOKABLE void play();
  Q_INVOKABLE void pause();
  Q_INVOKABLE void seekToFrame(qulonglong frame_index);
  Q_INVOKABLE void seekFromTimelinePosition(double local_x, double width);
  Q_INVOKABLE [[nodiscard]] QString timecodeAtRatio(double ratio) const;
  Q_INVOKABLE void enterGroup(const QString& group_id);
  Q_INVOKABLE void navigateUp();
  [[nodiscard]] PreparedCompositionProjection prepareComposition(
      std::shared_ptr<const refusion::core::CompositionSnapshot>
          composition) const;
  void publishComposition(PreparedCompositionProjection projection) noexcept;

 public slots:
  void refresh();

 signals:
  void snapshotChanged();
  void diagnosticChanged();
  void timelineNavigationChanged();

 private:
  void submit(refusion::runtime::presentation::TransportCommand command);
  [[nodiscard]] QString formatTimecode(std::uint64_t frame_index) const;

  refusion::runtime::presentation::ViewportRenderSession& render_session_;
  refusion::runtime::presentation::PlaybackSpec playback_spec_;
  std::shared_ptr<const refusion::core::CompositionSnapshot> composition_;
  TimelineTrackModel tracks_;
  std::vector<refusion::core::LayerGroupId> group_focus_;
  QString diagnostic_;
};
