#include "StudioBridge.hpp"

#include "refusion/application/ProjectCommandService.hpp"

#include <QCoreApplication>

#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("Studio bridge test requirement failed");
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  auto commands = refusion::application::create_application_host(
      refusion::core::ProjectSnapshot{
          .project_id = refusion::core::ProjectId{"prj_studio_bridge"},
          .revision_id = refusion::core::RevisionId{3},
          .display_name = "Before",
      });
  StudioBridge bridge(*commands);

  int snapshot_notifications = 0;
  int diagnostic_notifications = 0;
  QObject::connect(&bridge, &StudioBridge::snapshotChanged,
                   [&snapshot_notifications] { ++snapshot_notifications; });
  QObject::connect(&bridge, &StudioBridge::diagnosticChanged,
                   [&diagnostic_notifications] { ++diagnostic_notifications; });

  bridge.submitRename(QStringLiteral("After"));
  require(bridge.projectId() == QStringLiteral("prj_studio_bridge"));
  require(bridge.projectName() == QStringLiteral("After"));
  require(bridge.revision() == 4);
  require(bridge.diagnostic().isEmpty());
  require(snapshot_notifications == 1);
  require(diagnostic_notifications == 1);

  bridge.submitRename(QStringLiteral("   "));
  require(bridge.projectName() == QStringLiteral("After"));
  require(bridge.revision() == 4);
  require(bridge.diagnostic().startsWith(QStringLiteral("RFX-SCHEMA-001")));
  require(snapshot_notifications == 1);
  require(diagnostic_notifications == 2);
}
