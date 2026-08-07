#include "StudioBridge.hpp"
#include "StudioRuntimeComposition.hpp"

#include "refusion/application/ProjectCommandService.hpp"

#include <QGuiApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>

#include <exception>
#include <memory>

int main(int argc, char* argv[]) {
  QGuiApplication application(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("ReFusion Studio"));
  QGuiApplication::setOrganizationName(QStringLiteral("ReFusion"));

  auto commands = refusion::application::create_application_host(
      refusion::core::ProjectSnapshot{
          .project_id = refusion::core::ProjectId{"prj_refusion_foundation"},
          .revision_id = refusion::core::RevisionId{1},
          .display_name = "ReFusion Foundation",
      });
  StudioBridge bridge(*commands);
  std::unique_ptr<StudioRuntimeComposition> runtime_composition;
  QWindow* viewport_window = nullptr;
  QString viewport_diagnostic;
  try {
    runtime_composition = create_studio_runtime_composition();
    viewport_window = runtime_composition->viewport_window();
  } catch (const std::exception& error) {
    viewport_diagnostic = QString::fromUtf8(error.what());
    qCritical() << "Engine viewport initialization failed:" << viewport_diagnostic;
  }

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("studioBridge"), &bridge);
  engine.rootContext()->setContextProperty(
      QStringLiteral("engineViewportWindow"), viewport_window);
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
