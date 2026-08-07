#pragma once

#include "refusion/core/ProjectDocument.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <QString>
#include <QStringList>
#include <QWindow>

#include <atomic>
#include <cstdint>

class EngineViewportWindow final : public QWindow {
  Q_OBJECT
  Q_PROPERTY(QString adapterName READ adapterName CONSTANT)
  Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY diagnosticChanged)
  Q_PROPERTY(qulonglong presentedFrames READ presentedFrames NOTIFY telemetryChanged)
  Q_PROPERTY(bool zeroCopy READ zeroCopy NOTIFY telemetryChanged)
  Q_PROPERTY(qulonglong playbackPositionMs READ playbackPositionMs NOTIFY telemetryChanged)
  Q_PROPERTY(qulonglong playbackDurationMs READ playbackDurationMs CONSTANT)
  Q_PROPERTY(qulonglong playbackLoop READ playbackLoop NOTIFY telemetryChanged)
  Q_PROPERTY(bool playbackRunning READ playbackRunning NOTIFY telemetryChanged)
  Q_PROPERTY(uint compositionWidth READ compositionWidth CONSTANT)
  Q_PROPERTY(uint compositionHeight READ compositionHeight CONSTANT)
  Q_PROPERTY(QString compositionName READ compositionName CONSTANT)
  Q_PROPERTY(QString projectPath READ projectPath CONSTANT)
  Q_PROPERTY(QStringList layerNames READ layerNames CONSTANT)
  Q_PROPERTY(QString deviceStatus READ deviceStatus NOTIFY telemetryChanged)
  Q_PROPERTY(qulonglong deviceEventSequence READ deviceEventSequence NOTIFY telemetryChanged)
  Q_PROPERTY(qulonglong visibilitySuspends READ visibilitySuspends NOTIFY telemetryChanged)
  Q_PROPERTY(qulonglong visibilityResumes READ visibilityResumes NOTIFY telemetryChanged)
  Q_PROPERTY(qulonglong occlusionSuspends READ occlusionSuspends NOTIFY telemetryChanged)
  Q_PROPERTY(qulonglong occlusionResumes READ occlusionResumes NOTIFY telemetryChanged)
  Q_PROPERTY(qulonglong deviceLossRejections READ deviceLossRejections NOTIFY telemetryChanged)

 public:
  EngineViewportWindow(
      QString adapter_name,
      QString project_path,
      refusion::core::CompositionSnapshot composition,
      refusion::runtime::presentation::ViewportRenderSession& render_session);
  ~EngineViewportWindow() override;

  [[nodiscard]] QString adapterName() const;
  [[nodiscard]] QString diagnostic() const;
  [[nodiscard]] qulonglong presentedFrames() const noexcept;
  [[nodiscard]] bool zeroCopy() const noexcept;
  [[nodiscard]] qulonglong playbackPositionMs() const noexcept;
  [[nodiscard]] qulonglong playbackDurationMs() const noexcept;
  [[nodiscard]] qulonglong playbackLoop() const noexcept;
  [[nodiscard]] bool playbackRunning() const noexcept;
  [[nodiscard]] uint compositionWidth() const noexcept;
  [[nodiscard]] uint compositionHeight() const noexcept;
  [[nodiscard]] QString compositionName() const;
  [[nodiscard]] QString projectPath() const;
  [[nodiscard]] QStringList layerNames() const;
  [[nodiscard]] QString deviceStatus() const;
  [[nodiscard]] qulonglong deviceEventSequence() const noexcept;
  [[nodiscard]] qulonglong visibilitySuspends() const noexcept;
  [[nodiscard]] qulonglong visibilityResumes() const noexcept;
  [[nodiscard]] qulonglong occlusionSuspends() const noexcept;
  [[nodiscard]] qulonglong occlusionResumes() const noexcept;
  [[nodiscard]] qulonglong deviceLossRejections() const noexcept;

 signals:
  void diagnosticChanged();
  void telemetryChanged();

 protected:
  bool event(QEvent* event) override;
  void exposeEvent(QExposeEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

 private:
  void ensure_attached();
  void update_extent();
  void queue_telemetry_update();
  void set_diagnostic(QString diagnostic);

  QString adapter_name_;
  QString project_path_;
  refusion::core::CompositionSnapshot composition_;
  refusion::runtime::presentation::ViewportRenderSession& render_session_;
  QString diagnostic_;
  bool attached_{false};
  bool playback_started_{false};
  std::atomic_bool telemetry_update_pending_{false};
};
