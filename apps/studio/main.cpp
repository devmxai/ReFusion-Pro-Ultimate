#include "StudioBridge.hpp"

#include "refusion/application/ProjectCommandService.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

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
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("studioBridge"), &bridge);
  engine.loadFromModule(QStringLiteral("ReFusion.Studio"), QStringLiteral("Main"));
  if (engine.rootObjects().isEmpty()) {
    return 1;
  }
  return application.exec();
}
