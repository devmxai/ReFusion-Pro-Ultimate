#include "EngineViewportWindow.hpp"

#include "refusion/platform/PlatformViewportPresenter.hpp"

#include <QEvent>
#include <QExposeEvent>
#include <QMetaObject>
#include <QPlatformSurfaceEvent>
#include <QResizeEvent>
#include <QSurface>

#include <utility>

EngineViewportWindow::EngineViewportWindow(
    QString adapter_name,
    QString project_path,
    std::shared_ptr<const refusion::core::CompositionSnapshot> composition,
    refusion::runtime::presentation::ViewportRenderSession& render_session)
    : adapter_name_(std::move(adapter_name)),
      project_path_(std::move(project_path)),
      composition_(std::move(composition)),
      render_session_(render_session) {
  setTitle(QStringLiteral("ReFusion Engine Viewport"));
  setSurfaceType(QSurface::RasterSurface);
  setFlags(Qt::FramelessWindowHint);
  render_session_.set_frame_observer([this] { queue_telemetry_update(); });
}

EngineViewportWindow::~EngineViewportWindow() {
  render_session_.stop_playback();
  render_session_.set_frame_observer({});
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

qulonglong EngineViewportWindow::playbackPositionMs() const noexcept {
  return render_session_.playback_state().position_ns / 1'000'000;
}

qulonglong EngineViewportWindow::playbackDurationMs() const noexcept {
  return render_session_.playback_state().duration_ns / 1'000'000;
}

qulonglong EngineViewportWindow::playbackLoop() const noexcept {
  return render_session_.playback_state().loop_index;
}

bool EngineViewportWindow::playbackRunning() const noexcept {
  return render_session_.playback_state().running;
}

uint EngineViewportWindow::compositionWidth() const noexcept {
  return composition_->canvas.width_pixels;
}

uint EngineViewportWindow::compositionHeight() const noexcept {
  return composition_->canvas.height_pixels;
}

QString EngineViewportWindow::compositionName() const {
  return QString::fromStdString(composition_->display_name);
}

QString EngineViewportWindow::projectPath() const { return project_path_; }

QStringList EngineViewportWindow::layerNames() const {
  QStringList names;
  const auto roots = refusion::core::composition_root_nodes(*composition_);
  names.reserve(static_cast<qsizetype>(roots.size()));
  for (auto node = roots.rbegin(); node != roots.rend(); ++node) {
    if (const auto* layer_id =
            std::get_if<refusion::core::LayerId>(&*node)) {
      const auto* layer = refusion::core::find_layer(*composition_, *layer_id);
      if (layer != nullptr) {
        names.push_back(QString::fromStdString(layer->display_name));
      }
    } else {
      const auto* group = refusion::core::find_layer_group(
          *composition_, std::get<refusion::core::LayerGroupId>(*node));
      if (group != nullptr) {
        names.push_back(QString::fromStdString(group->display_name));
      }
    }
  }
  return names;
}

QString EngineViewportWindow::deviceStatus() const {
  switch (render_session_.telemetry().device_status) {
    case refusion::runtime::gpu::DeviceStatus::ready:
      return QStringLiteral("READY");
    case refusion::runtime::gpu::DeviceStatus::suspended:
      return QStringLiteral("SUSPENDED");
    case refusion::runtime::gpu::DeviceStatus::lost:
      return QStringLiteral("LOST");
  }
  return QStringLiteral("UNKNOWN");
}

qulonglong EngineViewportWindow::deviceEventSequence() const noexcept {
  return render_session_.telemetry().device_event_sequence;
}

qulonglong EngineViewportWindow::visibilitySuspends() const noexcept {
  return render_session_.telemetry().visibility_suspends;
}

qulonglong EngineViewportWindow::visibilityResumes() const noexcept {
  return render_session_.telemetry().visibility_resumes;
}

qulonglong EngineViewportWindow::occlusionSuspends() const noexcept {
  return render_session_.telemetry().occlusion_suspends;
}

qulonglong EngineViewportWindow::occlusionResumes() const noexcept {
  return render_session_.telemetry().occlusion_resumes;
}

qulonglong EngineViewportWindow::deviceLossRejections() const noexcept {
  return render_session_.telemetry().device_loss_rejections;
}

void EngineViewportWindow::publishComposition(
    std::shared_ptr<const refusion::core::CompositionSnapshot>
        composition) noexcept {
  composition_ = std::move(composition);
  emit compositionChanged();
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
  emit telemetryChanged();
  if (isExposed() && attached_) {
    update_extent();
    if (!initial_frame_presented_) {
      const auto result = render_session_.render_once();
      initial_frame_presented_ = result.succeeded();
    }
  }
}

void EngineViewportWindow::resizeEvent(QResizeEvent* event) {
  QWindow::resizeEvent(event);
  update_extent();
}

void EngineViewportWindow::ensure_attached() {
  if (attached_) {
    return;
  }
  create();
  const auto result = render_session_.attach(
      refusion::platform::acquire_platform_viewport_host(
          static_cast<std::uintptr_t>(winId())));
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

void EngineViewportWindow::queue_telemetry_update() {
  if (telemetry_update_pending_.exchange(true)) {
    return;
  }
  QMetaObject::invokeMethod(
      this,
      [this] {
        telemetry_update_pending_.store(false);
        const auto playback = render_session_.playback_state();
        if (playback.last_frame_status ==
            refusion::runtime::presentation::FrameStatus::rejected) {
          set_diagnostic(QString::fromStdString(playback.diagnostic));
        } else if (!diagnostic_.isEmpty()) {
          set_diagnostic({});
        }
        emit telemetryChanged();
      },
      Qt::QueuedConnection);
}

void EngineViewportWindow::set_diagnostic(QString diagnostic) {
  if (diagnostic_ == diagnostic) {
    return;
  }
  diagnostic_ = std::move(diagnostic);
  emit diagnosticChanged();
}
