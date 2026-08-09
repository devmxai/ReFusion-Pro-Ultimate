#include "adapters/QtMediaImportWorkspace.hpp"

#include "refusion/adapters/media/FfmpegMediaDemuxer.hpp"
#include "refusion/application/ExactAssetRelinkService.hpp"
#include "refusion/application/ImportVideoService.hpp"
#include "refusion/core/ProjectCreation.hpp"
#include "refusion/core/ProjectRfx.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace {

using namespace refusion::adapters::media;
using namespace refusion::application;
using namespace refusion::core;

void require(const bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  QTemporaryDir temporary;
  require(temporary.isValid(), "temporary project workspace is unavailable");
  const auto project_directory = temporary.filePath(QStringLiteral("Project"));
  require(QDir().mkpath(project_directory + QStringLiteral("/.refusion/Journal/Import")) &&
              QDir().mkpath(project_directory + QStringLiteral("/Assets/Media")),
          "could not create import transaction workspace");

  auto created = create_initial_project({
      .display_name = "Physical Import Transaction",
      .composition_preset_id = "reels-9x16",
      .resolution_id = "1080p",
      .frame_rate = 60,
      .duration_seconds = 1,
  });
  require(created.succeeded(), "could not create the initial project");
  auto commands = create_application_host(*created.project);

  const auto fixture = QString::fromUtf8(REFUSION_VIDEO_IMPORT_SOURCE_PATH);
  const auto source = open_immutable_compressed_file_source(fixture);
  require(source.succeeded(), source.diagnostic.toStdString());

  FfmpegMediaDemuxer demuxer;
  MediaIndexingService indexing(demuxer, nullptr, 1);
  QtMediaImportWorkspace workspace(project_directory);
  ImportVideoService import(*commands, indexing, workspace);
  const auto result = import.execute(ImportVideoIntent{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_physical_import_001"},
          .expected_revision = created.project->revision_id,
          .idempotency_key = IdempotencyKey{"physical-import-001"},
      },
      .source = source.source,
      .original_display_name = "source.mp4",
      .timeline_start = 0,
  });
  require(result.status == ImportVideoStatus::accepted,
          result.code + ": " + result.diagnostic);

  const auto active = commands->active_snapshot();
  const auto validation = validate_project(active);
  require(validation.valid, validation.code + ": " + validation.message);
  require(active.revision_id.value == created.project->revision_id.value + 1 &&
              active.assets.size() == 1 && active.media_sources.size() == 1 &&
              active.linked_imports.size() == 1 &&
              active.composition->video_clips.size() == 1 &&
              active.composition->audio_clips.size() == 1,
          "physical import did not publish one linked Video/Audio revision");
  const auto asset_path =
      project_directory + QLatin1Char('/') +
      QString::fromStdString(active.assets.front().project_relative_original);
  require(QFileInfo(asset_path).isFile() &&
              static_cast<std::uint64_t>(QFileInfo(asset_path).size()) ==
                  source.source->byte_size(),
          "verified original was not materialized inside the project package");

  require(QFile::remove(asset_path),
          "could not simulate a missing accepted project Asset");
  ExactAssetRelinkService relink(*commands, workspace);
  const auto relinked = relink.execute(RelinkExactAssetIntent{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_physical_relink_001"},
          .expected_revision = active.revision_id,
          .idempotency_key = IdempotencyKey{"physical-relink-001"},
      },
      .asset_id = active.assets.front().asset_id,
      .source = source.source,
  });
  require(relinked.succeeded() &&
              relinked.active_revision == active.revision_id &&
              commands->active_snapshot() == active &&
              QFileInfo(asset_path).isFile(),
          relinked.code + ": " + relinked.diagnostic);
  const auto canonical = serialize_project_rfx(active);
  const auto reopened = compile_project_rfx(canonical);
  require(reopened.succeeded() && *reopened.project == active,
          "accepted import revision did not survive canonical RFX6 reopen");
  require(canonical.find(fixture.toStdString()) == std::string::npos &&
              canonical.find("/Users/") == std::string::npos,
          "canonical import leaked the host fixture path");

  std::cout << "physical FFmpeg -> asset -> linked revision transaction passed\n";
  return 0;
}
