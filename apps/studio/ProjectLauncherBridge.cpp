#include "ProjectLauncherBridge.hpp"

#include "adapters/QtProjectWorkspaceCreator.hpp"
#include "adapters/QtRfxProjectFileAdapter.hpp"

#include <QDir>
#include <QFileInfo>
#include <QVariantMap>

#include <cstdint>
#include <string>
#include <utility>

namespace {

[[nodiscard]] std::string utf8_string(const QString& value) {
  const auto bytes = value.toUtf8();
  return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

}  // namespace

ProjectLauncherBridge::ProjectLauncherBridge(QObject* parent)
    : QObject(parent),
      creation_(refusion::application::create_project_creation_service()) {}

QVariantList ProjectLauncherBridge::presets() const {
  QVariantList result;
  for (const auto& preset : creation_->presets()) {
    QVariantList resolutions;
    for (const auto& resolution : preset.resolutions) {
      resolutions.push_back(QVariantMap{
          {QStringLiteral("id"), QString::fromStdString(resolution.id)},
          {QStringLiteral("name"),
           QString::fromStdString(resolution.display_name)},
          {QStringLiteral("width"), resolution.canvas.width_pixels},
          {QStringLiteral("height"), resolution.canvas.height_pixels},
      });
    }
    result.push_back(QVariantMap{
        {QStringLiteral("id"), QString::fromStdString(preset.id)},
        {QStringLiteral("name"), QString::fromStdString(preset.display_name)},
        {QStringLiteral("aspect"), QString::fromStdString(preset.aspect_label)},
        {QStringLiteral("resolutions"), resolutions},
    });
  }
  return result;
}

QVariantList ProjectLauncherBridge::frameRates() const {
  QVariantList result;
  for (const auto rate : creation_->frame_rates()) {
    result.push_back(rate);
  }
  return result;
}

QString ProjectLauncherBridge::diagnostic() const { return diagnostic_; }

bool ProjectLauncherBridge::busy() const noexcept { return busy_; }

void ProjectLauncherBridge::createProject(
    const QString& display_name,
    const QString& preset_id,
    const QString& resolution_id,
    const uint frame_rate,
    const uint duration_seconds,
    const QUrl& selected_folder) {
  if (busy_) {
    return;
  }
  setBusy(true);
  setDiagnostic({});
  if (!selected_folder.isLocalFile()) {
    setDiagnostic(QStringLiteral(
        "RFX-WORKSPACE-LOCATION-002: select a local desktop folder"));
    setBusy(false);
    return;
  }

  const auto blueprint = creation_->create(refusion::core::CreateProjectRequest{
      .display_name = utf8_string(display_name),
      .composition_preset_id = utf8_string(preset_id),
      .resolution_id = utf8_string(resolution_id),
      .frame_rate = static_cast<std::uint32_t>(frame_rate),
      .duration_seconds = static_cast<std::uint32_t>(duration_seconds),
  });
  if (!blueprint.succeeded()) {
    setDiagnostic(QString::fromStdString(blueprint.code + ": " +
                                         blueprint.message));
    setBusy(false);
    return;
  }

  const auto workspace = create_project_workspace(
      selected_folder.toLocalFile(), *blueprint.project);
  if (!workspace.succeeded()) {
    setDiagnostic(workspace.diagnostic);
    setBusy(false);
    return;
  }
  setBusy(false);
  emit projectReady(workspace.workspace->project_path);
}

void ProjectLauncherBridge::openProjectWorkspace(
    const QUrl& selected_folder) {
  if (busy_) {
    return;
  }
  setDiagnostic({});
  if (!selected_folder.isLocalFile()) {
    setDiagnostic(QStringLiteral(
        "RFX-PROJECT-OPEN: select a local ReFusion project folder"));
    return;
  }

  const QFileInfo workspace_info(selected_folder.toLocalFile());
  const QString canonical_workspace = workspace_info.canonicalFilePath();
  if (canonical_workspace.isEmpty() || !workspace_info.isDir()) {
    setDiagnostic(QStringLiteral(
                      "RFX-PROJECT-OPEN: %1: selected path is not an "
                      "existing project folder")
                      .arg(selected_folder.toLocalFile()));
    return;
  }

  const QString project_path =
      QDir(canonical_workspace).filePath(QStringLiteral("Project.rfx"));
  if (!QFileInfo(project_path).isFile()) {
    setDiagnostic(
        QStringLiteral(
            "RFX-PROJECT-OPEN: %1: folder does not contain Project.rfx")
            .arg(canonical_workspace));
    return;
  }

  const auto opened = open_refusion_project(project_path);
  if (!opened.succeeded()) {
    setDiagnostic(opened.diagnostic);
    return;
  }
  emit projectReady(opened.project->canonical_path);
}

void ProjectLauncherBridge::publishOpenFailure(QString diagnostic) {
  setDiagnostic(std::move(diagnostic));
}

void ProjectLauncherBridge::setDiagnostic(QString diagnostic) {
  if (diagnostic_ == diagnostic) {
    return;
  }
  diagnostic_ = std::move(diagnostic);
  emit diagnosticChanged();
}

void ProjectLauncherBridge::setBusy(const bool busy) {
  if (busy_ == busy) {
    return;
  }
  busy_ = busy;
  emit busyChanged();
}
