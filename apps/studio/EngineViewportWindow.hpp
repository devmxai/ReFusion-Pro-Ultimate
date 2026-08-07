#pragma once

#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <QString>
#include <QWindow>

#include <cstdint>

class EngineViewportWindow final : public QWindow {
  Q_OBJECT
  Q_PROPERTY(QString adapterName READ adapterName CONSTANT)
  Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY diagnosticChanged)
  Q_PROPERTY(qulonglong presentedFrames READ presentedFrames NOTIFY telemetryChanged)
  Q_PROPERTY(bool zeroCopy READ zeroCopy NOTIFY telemetryChanged)

 public:
  EngineViewportWindow(
      QString adapter_name,
      refusion::runtime::presentation::ViewportRenderSession& render_session);
  ~EngineViewportWindow() override;

  [[nodiscard]] QString adapterName() const;
  [[nodiscard]] QString diagnostic() const;
  [[nodiscard]] qulonglong presentedFrames() const noexcept;
  [[nodiscard]] bool zeroCopy() const noexcept;

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
  void render_frame();
  void set_diagnostic(QString diagnostic);

  QString adapter_name_;
  refusion::runtime::presentation::ViewportRenderSession& render_session_;
  QString diagnostic_;
  bool attached_{false};
};
