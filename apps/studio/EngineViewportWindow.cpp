#include "EngineViewportWindow.hpp"

#include "refusion/platform/PlatformViewportPresenter.hpp"

#include <QEvent>
#include <QExposeEvent>
#include <QPlatformSurfaceEvent>
#include <QResizeEvent>
#include <QSurface>

#include <utility>

EngineViewportWindow::EngineViewportWindow(
    QString adapter_name,
    refusion::runtime::presentation::ViewportRenderSession& render_session)
    : adapter_name_(std::move(adapter_name)), render_session_(render_session) {
  setTitle(QStringLiteral("ReFusion Engine Viewport"));
  setSurfaceType(QSurface::RasterSurface);
  setFlags(Qt::FramelessWindowHint);
}

EngineViewportWindow::~EngineViewportWindow() {
  render_session_.detach();
}

QString EngineViewportWindow::adapterName() const { return adapter_name_; }

QString EngineViewportWindow::diagnostic() const { return diagnostic_; }

qulonglong EngineViewportWindow::presentedFrames() const noexcept {
  return render_session_.telemetry().present_submissions;
}

bool EngineViewportWindow::zeroCopy() const noexcept {
  return render_session_.telemetry().zero_cpu_pixel_transfer();
}

bool EngineViewportWindow::event(QEvent* event) {
  if (event->type() == QEvent::PlatformSurface) {
    const auto* surface_event = static_cast<QPlatformSurfaceEvent*>(event);
    if (surface_event->surfaceEventType() ==
        QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
      render_session_.detach();
      attached_ = false;
    }
  }
  return QWindow::event(event);
}

void EngineViewportWindow::exposeEvent(QExposeEvent* event) {
  QWindow::exposeEvent(event);
  ensure_attached();
  render_session_.set_visible(isExposed());
  if (isExposed() && attached_) {
    update_extent();
    render_frame();
  }
}

void EngineViewportWindow::resizeEvent(QResizeEvent* event) {
  QWindow::resizeEvent(event);
  update_extent();
  if (isExposed()) {
    render_frame();
  }
}

void EngineViewportWindow::ensure_attached() {
  if (attached_) {
    return;
  }
  create();
  const auto result = render_session_.attach(
      refusion::runtime::presentation::NativeViewportHost{
          .window_system = refusion::platform::platform_native_window_system(),
          .handle = static_cast<std::uintptr_t>(winId()),
      });
  attached_ = result.succeeded();
  if (result.status == refusion::runtime::presentation::FrameStatus::rejected) {
    set_diagnostic(QString::fromStdString(result.diagnostic));
  }
}

void EngineViewportWindow::update_extent() {
  const auto result = render_session_.resize(
      refusion::runtime::presentation::ViewportExtent{
          .width_points = static_cast<std::uint32_t>(qMax(0, width())),
          .height_points = static_cast<std::uint32_t>(qMax(0, height())),
          .pixels_per_point = static_cast<float>(devicePixelRatio()),
      });
  if (result.status == refusion::runtime::presentation::FrameStatus::rejected) {
    set_diagnostic(QString::fromStdString(result.diagnostic));
  }
}

void EngineViewportWindow::render_frame() {
  if (!attached_ || !isExposed()) {
    return;
  }
  const auto result = render_session_.render_once();
  if (result.status == refusion::runtime::presentation::FrameStatus::rejected) {
    set_diagnostic(QString::fromStdString(result.diagnostic));
    return;
  }
  if (result.succeeded()) {
    if (!diagnostic_.isEmpty()) {
      set_diagnostic({});
    }
    emit telemetryChanged();
  }
}

void EngineViewportWindow::set_diagnostic(QString diagnostic) {
  if (diagnostic_ == diagnostic) {
    return;
  }
  diagnostic_ = std::move(diagnostic);
  emit diagnosticChanged();
}
