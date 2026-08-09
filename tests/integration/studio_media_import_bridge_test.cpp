#include "ProjectLiveReloadController.hpp"
#include "StudioBridge.hpp"
#include "StudioMediaImportBridge.hpp"
#include "adapters/QtRfxProjectFileAdapter.hpp"

#include "refusion/application/ProjectCommandService.hpp"
#include "refusion/core/ProjectCreation.hpp"
#include "refusion/core/ProjectRfx.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(const bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

void write_project(const QString& path,
                   const refusion::core::ProjectSnapshot& snapshot) {
  const auto serialized = refusion::core::serialize_project_rfx(snapshot);
  QFile file(path);
  require(file.open(QIODevice::WriteOnly | QIODevice::Truncate),
          "could not create the initial Project.rfx");
  require(file.write(serialized.data(),
                     static_cast<qsizetype>(serialized.size())) ==
              static_cast<qsizetype>(serialized.size()),
          "could not write the initial Project.rfx");
  file.close();
}

}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  QTemporaryDir temporary;
  require(temporary.isValid(), "temporary Studio workspace is unavailable");
  const auto project_directory = temporary.filePath(QStringLiteral("Project"));
  require(QDir().mkpath(project_directory),
          "could not create the Studio project directory");

  auto created = refusion::core::create_initial_project({
      .display_name = "Studio Import Client",
      .composition_preset_id = "reels-9x16",
      .resolution_id = "1080p",
      .frame_rate = 60,
      .duration_seconds = 30,
  });
  require(created.succeeded(), "could not create the initial project");
  const auto project_path =
      project_directory + QStringLiteral("/Project.rfx");
  write_project(project_path, *created.project);

  const auto initial_source = QByteArray::fromStdString(
      refusion::core::serialize_project_rfx(*created.project));
  auto commands =
      refusion::application::create_application_host(*created.project);
  StudioBridge studio(*commands);
  ProjectLiveReloadController live_reload(
      *commands, studio, project_path, QStringLiteral("refusion-cli"),
      initial_source);
  StudioMediaImportBridge media_import(
      *commands, studio, project_directory,
      [] { return refusion::core::ProjectTimeNs{0}; });
  QObject::connect(
      &media_import, &StudioMediaImportBridge::importCompleted, &live_reload,
      [&](const bool accepted) {
        live_reload.recordWorkflowDiagnostic(accepted,
                                             media_import.diagnostic());
      });

  const auto run_import = [&](const QString& path) {
    bool completed = false;
    bool accepted = false;
    QEventLoop loop;
    const auto connection = QObject::connect(
        &media_import, &StudioMediaImportBridge::importCompleted, &loop,
        [&](const bool value) {
          completed = true;
          accepted = value;
          loop.quit();
        });
    QTimer::singleShot(15'000, &loop, &QEventLoop::quit);
    media_import.importSelectedFile(QUrl::fromLocalFile(path));
    loop.exec();
    QObject::disconnect(connection);
    require(completed, "Studio import client timed out");
    return accepted;
  };

  const auto initial_revision = commands->active_snapshot().revision_id;
  require(!run_import(
              QString::fromUtf8(REFUSION_VIDEO_IMPORT_REJECTED_SOURCE_PATH)),
          "unsupported Studio media source was accepted");
  require(commands->active_snapshot().revision_id == initial_revision,
          "rejected Studio media source advanced project truth");
  require(media_import.diagnostic().contains(
              QStringLiteral("RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED")),
          "Studio rejected-media diagnostic code is missing");

  const auto accepted = run_import(
      QString::fromUtf8(REFUSION_VIDEO_IMPORT_SOURCE_PATH));
  require(accepted, media_import.diagnostic().toStdString());
  require(!media_import.busy(), "Studio import client remained busy");
  const auto active = commands->active_snapshot();
  require(active.revision_id.value == created.project->revision_id.value + 1,
          "Studio import client did not publish exactly one revision");
  require(active.assets.size() == 1 && active.media_sources.size() == 1 &&
              active.linked_imports.size() == 1 &&
              active.composition->video_clips.size() == 1 &&
              active.composition->audio_clips.size() == 1,
          "Studio import client did not publish linked Video and Audio clips");

  const auto reopened = open_refusion_project(project_path);
  require(reopened.succeeded(), reopened.diagnostic.toStdString());
  require(reopened.project->snapshot == active,
          "accepted Studio import did not persist and reopen as canonical RFX6");
  const auto canonical = reopened.project->source_bytes;
  require(!canonical.contains(REFUSION_VIDEO_IMPORT_SOURCE_PATH) &&
              !canonical.contains("/Users/"),
          "Studio import persistence leaked the selected host path");

  QFile diagnostics(project_directory +
                    QStringLiteral("/.refusion/Diagnostics/session.jsonl"));
  require(diagnostics.open(QIODevice::ReadOnly),
          "Studio diagnostics stream is unavailable");
  const auto diagnostic_bytes = diagnostics.readAll();
  require(diagnostic_bytes.contains("RFX-MEDIA-IMPORT-ACCEPTED") &&
              diagnostic_bytes.contains(
                  "RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED") &&
              diagnostic_bytes.contains("\"status\":\"rejected\"") &&
              diagnostic_bytes.contains("\"status\":\"accepted\""),
          "Studio media workflow result was not persisted for Agents");

  std::cout << "Studio file client -> shared import -> persisted RFX6 passed\n";
  return 0;
}
