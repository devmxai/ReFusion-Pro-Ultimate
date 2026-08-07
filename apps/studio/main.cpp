#include "StudioBridge.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char* argv[]) {
  QGuiApplication application(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("ReFusion Studio"));
  QGuiApplication::setOrganizationName(QStringLiteral("ReFusion"));

  StudioBridge bridge;
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("studioBridge"), &bridge);
  engine.loadFromModule(QStringLiteral("ReFusion.Studio"), QStringLiteral("Main"));
  if (engine.rootObjects().isEmpty()) {
    return 1;
  }
  return application.exec();
}

