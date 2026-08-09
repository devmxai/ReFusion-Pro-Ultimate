#include "MediaCommands.hpp"

#include "AgentJson.hpp"
#include "adapters/AtomicProjectFile.hpp"
#include "adapters/QtMediaImportWorkspace.hpp"
#include "adapters/QtRfxProjectFileAdapter.hpp"

#include "refusion/adapters/media/FfmpegMediaDemuxer.hpp"
#include "refusion/application/ExactAssetRelinkService.hpp"
#include "refusion/application/ImportVideoService.hpp"
#include "refusion/application/ProjectCommandService.hpp"
#include "refusion/core/ProjectRfx.hpp"

#include <QFileInfo>

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace refusion::cli {
namespace {

using namespace refusion::application;
using namespace refusion::core;

[[nodiscard]] std::filesystem::path native_path(const QString& value) {
  const auto utf8 = value.toUtf8();
  const std::u8string encoded(
      reinterpret_cast<const char8_t*>(utf8.constData()),
      static_cast<std::size_t>(utf8.size()));
  return std::filesystem::path(encoded);
}

[[nodiscard]] bool parse_time(const QString& text, ProjectTimeNs& result) {
  const auto bytes = text.toLatin1();
  if (bytes.isEmpty()) return false;
  const auto parsed =
      std::from_chars(bytes.constData(), bytes.constData() + bytes.size(), result);
  return parsed.ec == std::errc{} &&
         parsed.ptr == bytes.constData() + bytes.size();
}

[[nodiscard]] CommandEnvelope envelope(const ProjectSnapshot& project,
                                       const std::string_view operation) {
  const auto suffix = std::to_string(project.revision_id.value + 1);
  const auto identity = "agent." + std::string(operation) + ".r" + suffix;
  return CommandEnvelope{
      .command_id = CommandId{identity},
      .expected_revision = project.revision_id,
      .idempotency_key = IdempotencyKey{identity},
  };
}

class PersistedRevisionProxy final : public ProjectRevisionService {
 public:
  PersistedRevisionProxy(ProjectCommandService& commands,
                         std::filesystem::path project_path,
                         std::string accepted_source)
      : commands_(commands),
        project_path_(std::move(project_path)),
        accepted_source_(std::move(accepted_source)) {}

  [[nodiscard]] ProjectSnapshot active_snapshot() const override {
    return commands_.active_snapshot();
  }

  [[nodiscard]] ApplyResult submit(
      const ReplaceProjectCommand& command) override {
    const auto before = commands_.active_snapshot();
    auto applied = commands_.submit(command);
    if (!applied.accepted()) return applied;
    const auto canonical = serialize_project_rfx(applied.active_snapshot);
    const auto persisted = replace_project_file_if_unchanged(
        project_path_, accepted_source_, canonical);
    if (!persisted.replaced) {
      return ApplyResult{
          .status = ApplyStatus::rejected,
          .command_id = command.envelope.command_id,
          .active_snapshot = before,
          .diagnostic = Diagnostic{
              .code = persisted.code,
              .message = persisted.message,
              .blocking = true,
          },
      };
    }
    accepted_source_ = canonical;
    return applied;
  }

 private:
  ProjectCommandService& commands_;
  std::filesystem::path project_path_;
  std::string accepted_source_;
};

[[nodiscard]] int import_video(const QStringList& arguments) {
  if (arguments.size() != 6) return 2;
  const auto project_path = arguments.at(3);
  const auto selected_file = arguments.at(4);
  ProjectTimeNs timeline_start = 0;
  if (!parse_time(arguments.at(5), timeline_start)) {
    write_error_json(std::cout, "refusion.agent.media.v1",
                     "RFX-AGENT-ARG-MEDIA-TIME",
                     "timeline start must be integer project nanoseconds");
    return 2;
  }
  auto opened = open_refusion_project(project_path);
  if (!opened.succeeded()) {
    write_error_json(std::cout, "refusion.agent.media.v1",
                     "RFX-MEDIA-PROJECT-OPEN",
                     opened.diagnostic.toStdString());
    return 1;
  }
  auto source = open_immutable_compressed_file_source(selected_file);
  if (!source.succeeded()) {
    write_error_json(std::cout, "refusion.agent.media.v1",
                     "RFX-MEDIA-IMPORT-SOURCE-OPEN",
                     source.diagnostic.toStdString());
    return 1;
  }

  auto commands = create_application_host(opened.project->snapshot);
  const std::string accepted_source(
      opened.project->source_bytes.constData(),
      static_cast<std::size_t>(opened.project->source_bytes.size()));
  PersistedRevisionProxy revisions(
      *commands, native_path(opened.project->canonical_path), accepted_source);
  refusion::adapters::media::FfmpegMediaDemuxer demuxer;
  MediaIndexingService indexing(demuxer, nullptr, 1);
  QtMediaImportWorkspace workspace(
      QFileInfo(opened.project->canonical_path).absolutePath());
  ImportVideoService service(revisions, indexing, workspace);
  const auto result = service.execute(ImportVideoIntent{
      .envelope = envelope(opened.project->snapshot, "import-video"),
      .source = source.source,
      .original_display_name = QFileInfo(selected_file).fileName().toStdString(),
      .timeline_start = timeline_start,
  });
  if (!result.succeeded()) {
    write_error_json(std::cout, "refusion.agent.media.v1", result.code,
                     result.diagnostic);
    return 1;
  }
  std::cout << "{\"schema\":\"refusion.agent.media.v1\",\"ok\":true,"
               "\"operation\":\"import-video\",\"status\":\""
            << (result.status == ImportVideoStatus::replayed ? "replayed"
                                                             : "accepted")
            << "\",\"revision\":" << result.active_revision.value
            << ",\"asset_id\":\"" << result.asset_id.value
            << "\",\"media_source_id\":\""
            << result.media_source_id.value << "\",\"linked_import_id\":\""
            << result.linked_import_id.value << "\"}\n";
  return 0;
}

[[nodiscard]] int relink_exact(const QStringList& arguments) {
  if (arguments.size() != 6) return 2;
  const auto project_path = arguments.at(3);
  const auto asset_id = AssetId{arguments.at(4).toStdString()};
  const auto selected_file = arguments.at(5);
  auto opened = open_refusion_project(project_path);
  if (!opened.succeeded()) {
    write_error_json(std::cout, "refusion.agent.media.v1",
                     "RFX-MEDIA-PROJECT-OPEN",
                     opened.diagnostic.toStdString());
    return 1;
  }
  auto source = open_immutable_compressed_file_source(selected_file);
  if (!source.succeeded()) {
    write_error_json(std::cout, "refusion.agent.media.v1",
                     "RFX-MEDIA-RELINK-SOURCE-OPEN",
                     source.diagnostic.toStdString());
    return 1;
  }
  auto commands = create_application_host(opened.project->snapshot);
  QtMediaImportWorkspace workspace(
      QFileInfo(opened.project->canonical_path).absolutePath());
  ExactAssetRelinkService service(*commands, workspace);
  const auto result = service.execute(RelinkExactAssetIntent{
      .envelope = envelope(opened.project->snapshot, "relink-exact"),
      .asset_id = asset_id,
      .source = source.source,
  });
  if (!result.succeeded()) {
    write_error_json(std::cout, "refusion.agent.media.v1", result.code,
                     result.diagnostic);
    return 1;
  }
  std::cout << "{\"schema\":\"refusion.agent.media.v1\",\"ok\":true,"
               "\"operation\":\"relink-exact\",\"status\":\"restored\","
               "\"revision\":"
            << result.active_revision.value << ",\"asset_id\":\""
            << result.asset_id.value << "\"}\n";
  return 0;
}

}  // namespace

int run_media_command(const QStringList& arguments) {
  if (arguments.size() < 3 || arguments.at(1) != QStringLiteral("commit")) {
    return 2;
  }
  const auto operation = arguments.at(2);
  if (operation == QStringLiteral("import-video")) {
    return import_video(arguments);
  }
  if (operation == QStringLiteral("relink-exact")) {
    return relink_exact(arguments);
  }
  return 2;
}

}  // namespace refusion::cli
