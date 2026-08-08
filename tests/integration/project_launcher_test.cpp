#include "ProjectLauncherBridge.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QUrl>

#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("project launcher test requirement failed");
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  ProjectLauncherBridge launcher;
  require(launcher.presets().size() == 4);
  require(launcher.frameRates().size() == 6);

  QTemporaryDir temporary;
  require(temporary.isValid());
  const QString root = temporary.filePath(QStringLiteral("Agent Project"));
  require(QDir().mkpath(root));

  QString ready_path;
  QObject::connect(&launcher, &ProjectLauncherBridge::projectReady,
                   [&ready_path](const QString& path) { ready_path = path; });
  launcher.createProject(QStringLiteral("Agent Project"),
                         QStringLiteral("reels-9x16"),
                         QStringLiteral("2k"),
                         90,
                         30,
                         QUrl::fromLocalFile(root));
  require(launcher.diagnostic().isEmpty());
  require(!ready_path.isEmpty());
  require(QFileInfo::exists(ready_path));

  ready_path.clear();
  launcher.openProjectWorkspace(QUrl::fromLocalFile(root));
  require(!ready_path.isEmpty());
  require(launcher.diagnostic().isEmpty());

  const QString invalid_root =
      temporary.filePath(QStringLiteral("Invalid Project"));
  require(QDir().mkpath(invalid_root));

  ready_path.clear();
  launcher.openProjectWorkspace(QUrl::fromLocalFile(invalid_root));
  require(ready_path.isEmpty());
  require(launcher.diagnostic().contains(
      QStringLiteral("folder does not contain Project.rfx")));

  launcher.openProjectWorkspace(QUrl::fromLocalFile(
      root + QStringLiteral("/Project.rfx")));
  require(ready_path.isEmpty());
  require(launcher.diagnostic().contains(
      QStringLiteral("selected path is not an existing project folder")));

  launcher.createProject(QStringLiteral("Invalid"),
                         QStringLiteral("unknown"),
                         QStringLiteral("2k"),
                         90,
                         30,
                         QUrl::fromLocalFile(invalid_root));
  require(launcher.diagnostic().startsWith(
      QStringLiteral("RFX-CREATE-PRESET-001")));
  require(!QFileInfo::exists(invalid_root + QStringLiteral("/Project.rfx")));
}
