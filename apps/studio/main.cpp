#include "ProjectLauncherBridge.hpp"
#include "ProjectLiveReloadController.hpp"
#include "StudioBridge.hpp"
#include "StudioRuntimeComposition.hpp"
#include "StudioTransportBridge.hpp"
#include "adapters/QtProjectFontAssetResolver.hpp"
#include "adapters/QtRfxProjectFileAdapter.hpp"

#include "refusion/application/ProjectCommandService.hpp"

#if defined(REFUSION_STUDIO_SKIA_MEASUREMENT)
#include "refusion/adapters/skia/SkiaTextLayout.hpp"
#endif

#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QWindow>

#include <exception>
#include <memory>
#include <utility>

namespace {

class ActiveStudioSession final {
 public:
  [[nodiscard]] bool initialize(const QString& project_path,
                                const QString& cli_path,
                                QQmlApplicationEngine& engine,
                                QString& diagnostic) {
    auto open_result = open_refusion_project(project_path);
    if (!open_result.succeeded()) {
      diagnostic = open_result.diagnostic;
      return false;
    }
    auto opened_project = std::move(*open_result.project);
    auto font_assets = std::make_shared<QtProjectFontAssetResolver>(
        QFileInfo(opened_project.canonical_path).absolutePath());
    std::shared_ptr<refusion::core::TextLayoutPort> text_layout_port;
#if defined(REFUSION_STUDIO_SKIA_MEASUREMENT)
    text_layout_port =
        refusion::adapters::skia::create_skia_text_layout_port(font_assets);
#endif
    commands_ = refusion::application::create_application_host(
        opened_project.snapshot, text_layout_port);
    bridge_ =
        std::make_unique<StudioBridge>(*commands_, text_layout_port);

    QWindow* viewport_window = nullptr;
    StudioTransportBridge* transport_bridge = nullptr;
    QString viewport_diagnostic;
    try {
      runtime_ = create_studio_runtime_composition(
          opened_project.snapshot, opened_project.canonical_path,
          font_assets);
      commands_->set_candidate_admission_port(runtime_);
      viewport_window = runtime_->viewport_window();
      transport_bridge = runtime_->transport_bridge();
      bridge_->setCompositionTimeProvider([transport_bridge] {
        return transport_bridge->compositionTimeNs();
      });
      QObject::connect(transport_bridge,
                       &StudioTransportBridge::snapshotChanged,
                       bridge_.get(),
                       &StudioBridge::refreshMeasurementProjection);
      live_reload_ = std::make_unique<ProjectLiveReloadController>(
          *commands_,
          *bridge_,
          opened_project.canonical_path,
          cli_path,
          opened_project.source_bytes);
    } catch (const std::exception& error) {
      viewport_diagnostic = QString::fromUtf8(error.what());
      qCritical() << "Engine viewport initialization failed:"
                  << viewport_diagnostic;
    }

    auto* context = engine.rootContext();
    context->setContextProperty(QStringLiteral("studioBridge"), bridge_.get());
    context->setContextProperty(QStringLiteral("engineViewportWindow"),
                                viewport_window);
    context->setContextProperty(QStringLiteral("transportBridge"),
                                transport_bridge);
    context->setContextProperty(QStringLiteral("engineViewportAvailable"),
                                viewport_window != nullptr);
    context->setContextProperty(QStringLiteral("engineViewportDiagnostic"),
                                viewport_diagnostic);
    const auto root_count = engine.rootObjects().size();
    engine.loadFromModule(QStringLiteral("ReFusion.Studio"),
                          QStringLiteral("Main"));
    if (engine.rootObjects().size() == root_count) {
      diagnostic = QStringLiteral("RFX-STUDIO-QML-001: Main UI failed to load");
      return false;
    }
    return true;
  }

 private:
  std::unique_ptr<refusion::application::ProjectCommandService> commands_;
  std::unique_ptr<StudioBridge> bridge_;
  std::shared_ptr<StudioRuntimeComposition> runtime_;
  std::unique_ptr<ProjectLiveReloadController> live_reload_;
};

}  // namespace

int main(int argc, char* argv[]) {
  QGuiApplication application(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("ReFusion Studio"));
  QGuiApplication::setOrganizationName(QStringLiteral("ReFusion"));

  ProjectLauncherBridge launcher;
  std::unique_ptr<ActiveStudioSession> active_session;
  // The QML engine must be destroyed before either context object. Otherwise
  // QML bindings can briefly observe dangling/null context properties while
  // the process is shutting down.
  QQmlApplicationEngine engine;
  QWindow* launcher_window = nullptr;
  const QString cli_path = QString::fromUtf8(REFUSION_CLI_PATH);

  auto open_session = [&](const QString& path) {
    if (active_session) {
      return;
    }
    auto candidate = std::make_unique<ActiveStudioSession>();
    QString diagnostic;
    if (!candidate->initialize(path, cli_path, engine, diagnostic)) {
      launcher.publishOpenFailure(diagnostic);
      return;
    }
    active_session = std::move(candidate);
    if (launcher_window != nullptr) {
      launcher_window->hide();
    }
  };

  QObject::connect(
      &launcher,
      &ProjectLauncherBridge::projectReady,
      &application,
      [&](const QString& path) {
        QTimer::singleShot(0, &application, [&, path] { open_session(path); });
      });

  engine.rootContext()->setContextProperty(QStringLiteral("projectLauncher"),
                                           &launcher);
  if (argc > 1) {
    open_session(QString::fromLocal8Bit(argv[1]));
    if (!active_session) {
      qWarning().noquote() << launcher.diagnostic();
      return 2;
    }
  } else {
    engine.loadFromModule(QStringLiteral("ReFusion.Studio"),
                          QStringLiteral("Launcher"));
    if (engine.rootObjects().isEmpty()) {
      return 1;
    }
    launcher_window = qobject_cast<QWindow*>(engine.rootObjects().constLast());
  }
  return application.exec();
}
