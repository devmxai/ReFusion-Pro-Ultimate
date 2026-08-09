#include "adapters/QtProjectFontAssetResolver.hpp"
#include "adapters/QtMediaImportWorkspace.hpp"
#include "adapters/QtProjectWorkspaceCreator.hpp"
#include "adapters/QtRfxProjectFileAdapter.hpp"

#include "refusion/core/ProjectCreation.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"
#include "refusion/core/VisualPropertyRegistry.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error(
        "project workspace creator test requirement failed");
  }
}

[[nodiscard]] refusion::core::ProjectSnapshot project_fixture() {
  const auto result = refusion::core::create_initial_project({
      .display_name = "Workspace Test",
      .composition_preset_id = "youtube-16x9",
      .resolution_id = "1080p",
      .frame_rate = 60,
      .duration_seconds = 30,
  });
  require(result.succeeded());
  return *result.project;
}

} // namespace

int main(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  QTemporaryDir temporary;
  require(temporary.isValid());
  const QString root = temporary.filePath(QStringLiteral("New Project"));
  require(QDir().mkpath(root));

  const auto created = create_project_workspace(root, project_fixture());
  require(created.succeeded());
  require(QFileInfo::exists(root + QStringLiteral("/Project.rfx")));
  require(QFileInfo::exists(root + QStringLiteral("/refusion.lock")));
  require(QFileInfo::exists(root + QStringLiteral("/AGENTS.md")));
  require(QFileInfo::exists(
      root +
      QStringLiteral("/.agents/skills/refusion-project-authoring/SKILL.md")));
  require(QFileInfo::exists(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/language-v2.md")));
  require(QFileInfo::exists(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/language-v3.md")));
  require(QFileInfo::exists(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/language-v4.md")));
  require(QFileInfo::exists(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/language-v5.md")));
  require(QFileInfo::exists(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/language-v6.md")));
  require(QFileInfo::exists(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/property-registry.md")));
  require(QFileInfo::exists(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/visual-contributions.md")));
  require(QFileInfo::exists(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/semantic-authoring.md")));
  require(QFileInfo::exists(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/agent-commands.md")));
  require(QFileInfo(root + QStringLiteral("/Assets/Media")).isDir());
  require(QFileInfo::exists(
      root + QStringLiteral(
                 "/Assets/Fonts/font_noto_sans_regular/font.ttf")));
  require(QFileInfo::exists(
      root + QStringLiteral(
                 "/Assets/Fonts/font_noto_sans_regular/OFL.txt")));
  require(QFileInfo::exists(
      root + QStringLiteral(
                 "/Assets/Fonts/font_noto_sans_arabic_regular/font.ttf")));
  require(QFileInfo::exists(root +
                            QStringLiteral("/Assets/Fonts/catalog.lock")));
  QtProjectFontAssetResolver font_assets(root);
  const auto latin_font = font_assets.resolve_font_asset({
      .asset_id = "font_noto_sans_regular",
      .expected_content_digest =
          "sha256:f5f552c8c5edb61fe6efb824baf4d4de47b1a8689ab4925ff43f7bd6a4ebece5",
  });
  require(latin_font.succeeded());
  require(latin_font.asset->bytes->size() == 825628U);
  const auto arabic_font = font_assets.resolve_font_asset({
      .asset_id = "font_noto_sans_arabic_regular",
      .expected_content_digest =
          "sha256:7ed3fe069312aceac454f17cf613a30f95271d6ed7ce58005ed4d016bd3823d7",
  });
  require(arabic_font.succeeded());
  require(arabic_font.asset->bytes->size() == 271652U);
  require(!font_assets.resolve_font_asset({
      .asset_id = "font_noto_sans_regular",
      .expected_content_digest =
          "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
  }).succeeded());
  QFile project_source(root + QStringLiteral("/Project.rfx"));
  require(project_source.open(QIODevice::ReadOnly));
  const auto project_bytes = project_source.readAll();
  require(project_bytes.startsWith("rfx 5;"));
  const auto registry_digest = QByteArray::fromStdString(
      refusion::core::visual_property_registry_digest());
  require(project_bytes.contains(registry_digest));
  project_source.close();
  QFile project_lock(root + QStringLiteral("/refusion.lock"));
  require(project_lock.open(QIODevice::ReadOnly));
  const auto lock_bytes = project_lock.readAll();
  require(lock_bytes.contains("rfx_language_version = 6"));
  require(lock_bytes.contains("refusion-project-rfx-exp6"));
  require(lock_bytes.contains(registry_digest));
  require(lock_bytes.contains(QByteArray::fromStdString(
      refusion::core::visual_contribution_registry_digest())));
  project_lock.close();
  QFile registry_projection(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/property-registry.md"));
  require(registry_projection.open(QIODevice::ReadOnly));
  require(registry_projection.readAll().contains(registry_digest));
  registry_projection.close();
  QFile contribution_projection(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/visual-contributions.md"));
  require(contribution_projection.open(QIODevice::ReadOnly));
  const auto contribution_digest = QByteArray::fromStdString(
      refusion::core::visual_contribution_registry_digest());
  const auto contribution_bytes = contribution_projection.readAll();
  require(contribution_bytes.contains(contribution_digest));
  require(contribution_bytes.contains("visual.fx.glow.v1"));
  require(contribution_bytes.contains("color_rgba8"));
  contribution_projection.close();
  QFile agent_commands(
      root + QStringLiteral("/.agents/skills/refusion-project-authoring/"
                            "references/agent-commands.md"));
  require(agent_commands.open(QIODevice::ReadOnly));
  const auto agent_command_bytes = agent_commands.readAll();
  require(agent_command_bytes.contains(registry_digest));
  require(agent_command_bytes.contains("commit align"));
  agent_commands.close();
  const auto opened =
      open_refusion_project(root + QStringLiteral("/Project.rfx"));
  require(opened.succeeded());
  require(opened.project->snapshot.composition->layers.empty());
  require(opened.project->snapshot.composition->canvas.width_pixels == 1920);
  require(opened.project->snapshot.composition->frame_rate.numerator == 60);

  const QString selected_media = temporary.filePath(QStringLiteral("source.mp4"));
  QFile selected_output(selected_media);
  require(selected_output.open(QIODevice::WriteOnly));
  const QByteArray selected_bytes(64 * 1024, '\x51');
  require(selected_output.write(selected_bytes) == selected_bytes.size());
  selected_output.close();
  const auto compressed_source =
      open_immutable_compressed_file_source(selected_media);
  require(compressed_source.succeeded());
  const refusion::application::MediaAssetMaterializationReceipt media_receipt{
      .asset_id = refusion::core::AssetId{"ast_qt_import_test"},
      .content_digest = compressed_source.source->content_digest(),
      .byte_size = compressed_source.source->byte_size(),
      .project_relative_original =
          "Assets/Media/ast_qt_import_test/original.mp4",
  };
  QtMediaImportWorkspace media_workspace(root);
  {
    auto prepared = media_workspace.prepare_copy(
        "qt-import-rollback", media_receipt, *compressed_source.source, nullptr);
    require(prepared != nullptr && prepared->receipt() == media_receipt);
    require(prepared->commit());
    require(QFileInfo::exists(
        root + QStringLiteral("/Assets/Media/ast_qt_import_test/original.mp4")));
  }
  require(!QFileInfo::exists(
      root + QStringLiteral("/Assets/Media/ast_qt_import_test/original.mp4")));
  {
    auto prepared = media_workspace.prepare_copy(
        "qt-import-retain", media_receipt, *compressed_source.source, nullptr);
    require(prepared != nullptr && prepared->commit());
    prepared->retain();
  }
  const auto committed_media =
      root + QStringLiteral("/Assets/Media/ast_qt_import_test/original.mp4");
  require(QFileInfo::exists(committed_media));
  require(QFile(committed_media).size() == selected_bytes.size());

  const auto interrupted_directory =
      root + QStringLiteral("/.refusion/Journal/Import/interrupted");
  require(QDir().mkpath(interrupted_directory));
  QFile interrupted_manifest(interrupted_directory +
                             QStringLiteral("/manifest.json"));
  require(interrupted_manifest.open(QIODevice::WriteOnly));
  const QJsonObject interrupted_object{
      {QStringLiteral("project_relative_original"),
       QStringLiteral("Assets/Media/ast_qt_import_test/original.mp4")},
      {QStringLiteral("content_digest"),
       QString::fromStdString(media_receipt.content_digest)},
      {QStringLiteral("byte_size"),
       QString::number(static_cast<qulonglong>(media_receipt.byte_size))},
  };
  const auto interrupted_bytes =
      QJsonDocument(interrupted_object).toJson(QJsonDocument::Compact);
  require(interrupted_manifest.write(interrupted_bytes) ==
          interrupted_bytes.size());
  interrupted_manifest.close();
  require(recover_incomplete_media_imports(root, opened.project->snapshot));
  require(!QFileInfo::exists(committed_media) &&
          !QFileInfo::exists(interrupted_directory));

  const auto collision = create_project_workspace(root, project_fixture());
  require(!collision.succeeded());
  require(collision.diagnostic.startsWith(
      QStringLiteral("RFX-WORKSPACE-NOT-EMPTY")));
  require(
      open_refusion_project(root + QStringLiteral("/Project.rfx")).succeeded());

  const QString occupied = temporary.filePath(QStringLiteral("Occupied"));
  require(QDir().mkpath(occupied));
  QFile existing(occupied + QStringLiteral("/user-data.txt"));
  require(existing.open(QIODevice::WriteOnly));
  require(existing.write("preserve") == 8);
  existing.close();
  const auto rejected = create_project_workspace(occupied, project_fixture());
  require(!rejected.succeeded());
  require(QFileInfo::exists(occupied + QStringLiteral("/user-data.txt")));
  require(!QFileInfo::exists(occupied + QStringLiteral("/Project.rfx")));
}
