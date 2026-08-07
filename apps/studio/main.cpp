#include "StudioBridge.hpp"
#include "StudioRuntimeComposition.hpp"
#include "StudioTransportBridge.hpp"
#include "adapters/QtJsonProjectFileAdapter.hpp"

#include "refusion/application/ProjectCommandService.hpp"

#include <QGuiApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>

#include <exception>
#include <memory>
#include <utility>

int main(int argc, char* argv[]) {
  QGuiApplication application(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("ReFusion Studio"));
  QGuiApplication::setOrganizationName(QStringLiteral("ReFusion"));

  const QString project_path = argc > 1
                                   ? QString::fromLocal8Bit(argv[1])
                                   : QString::fromUtf8(REFUSION_DEFAULT_PROJECT_PATH);
  auto open_result = open_refusion_project(project_path);
  if (!open_result.succeeded()) {
    qWarning().noquote() << "Project open failed:" << open_result.diagnostic;
    return 2;
  }
  auto opened_project = std::move(*open_result.project);

  auto commands =
      refusion::application::create_application_host(opened_project.snapshot);
  StudioBridge bridge(*commands);
  std::unique_ptr<StudioRuntimeComposition> runtime_composition;
  QWindow* viewport_window = nullptr;
  StudioTransportBridge* transport_bridge = nullptr;
  QString viewport_diagnostic;
  try {
    runtime_composition = create_studio_runtime_composition(
        opened_project.snapshot, opened_project.canonical_path);
    viewport_window = runtime_composition->viewport_window();
    transport_bridge = runtime_composition->transport_bridge();
  } catch (const std::exception& error) {
    viewport_diagnostic = QString::fromUtf8(error.what());
    qCritical() << "Engine viewport initialization failed:" << viewport_diagnostic;
  }

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("studioBridge"), &bridge);
  engine.rootContext()->setContextProperty(
      QStringLiteral("engineViewportWindow"), viewport_window);
  engine.rootContext()->setContextProperty(
      QStringLiteral("transportBridge"), transport_bridge);
  engine.rootContext()->setContextProperty(
      QStringLiteral("engineViewportAvailable"), viewport_window != nullptr);
  engine.rootContext()->setContextProperty(
      QStringLiteral("engineViewportDiagnostic"), viewport_diagnostic);
  engine.loadFromModule(QStringLiteral("ReFusion.Studio"), QStringLiteral("Main"));
  if (engine.rootObjects().isEmpty()) {
    return 1;
  }
  return application.exec();
}
