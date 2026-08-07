#pragma once

#include "refusion/core/ProjectDocument.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <QAbstractListModel>
#include <QObject>
#include <QString>

#include <cstdint>
#include <vector>

class TimelineTrackModel final : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role : int {
    layerIdRole = Qt::UserRole + 1,
    displayNameRole,
    startFrameRole,
    endFrameRole,
    durationFramesRole,
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

 private:
  struct Track final {
    QString layer_id;
    QString display_name;
    qulonglong start_frame{0};
    qulonglong end_frame{0};
  };

  std::vector<Track> tracks_;
};

class StudioTransportBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool running READ running NOTIFY snapshotChanged)
  Q_PROPERTY(qulonglong positionFrame READ positionFrame NOTIFY snapshotChanged)
  Q_PROPERTY(qulonglong durationFrames READ durationFrames CONSTANT)
  Q_PROPERTY(double positionRatio READ positionRatio NOTIFY snapshotChanged)
  Q_PROPERTY(QString positionTimecode READ positionTimecode NOTIFY snapshotChanged)
  Q_PROPERTY(QString durationTimecode READ durationTimecode CONSTANT)
  Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY diagnosticChanged)
  Q_PROPERTY(QAbstractItemModel* tracks READ tracks CONSTANT)

 public:
  StudioTransportBridge(
      refusion::runtime::presentation::ViewportRenderSession& render_session,
      refusion::core::CompositionSnapshot composition,
      QObject* parent = nullptr);

  [[nodiscard]] bool running() const noexcept;
  [[nodiscard]] qulonglong positionFrame() const noexcept;
  [[nodiscard]] qulonglong durationFrames() const noexcept;
  [[nodiscard]] double positionRatio() const noexcept;
  [[nodiscard]] QString positionTimecode() const;
  [[nodiscard]] QString durationTimecode() const;
  [[nodiscard]] QString diagnostic() const;
  [[nodiscard]] QAbstractItemModel* tracks() noexcept;

  Q_INVOKABLE void togglePlayback();
  Q_INVOKABLE void play();
  Q_INVOKABLE void pause();
  Q_INVOKABLE void seekToFrame(qulonglong frame_index);
  Q_INVOKABLE void seekFromTimelinePosition(double local_x, double width);
  Q_INVOKABLE [[nodiscard]] QString timecodeAtRatio(double ratio) const;

 public slots:
  void refresh();

 signals:
  void snapshotChanged();
  void diagnosticChanged();

 private:
  void submit(refusion::runtime::presentation::TransportCommand command);
  [[nodiscard]] QString formatTimecode(std::uint64_t frame_index) const;

  refusion::runtime::presentation::ViewportRenderSession& render_session_;
  refusion::runtime::presentation::PlaybackSpec playback_spec_;
  TimelineTrackModel tracks_;
  QString diagnostic_;
};
