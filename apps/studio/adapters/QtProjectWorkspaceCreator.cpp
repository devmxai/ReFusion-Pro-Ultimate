#include "adapters/QtProjectWorkspaceCreator.hpp"

#include "adapters/QtRfxProjectFileAdapter.hpp"

#include "refusion/application/AgentAuthoringGuide.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"
#include "refusion/core/VisualPropertyRegistry.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QUuid>

#include <array>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct CreatedEntry final {
  QString path;
  bool directory{false};
};

[[nodiscard]] WorkspaceCreateResult rejected(QString diagnostic) {
  return WorkspaceCreateResult{.diagnostic = std::move(diagnostic)};
}

[[nodiscard]] QByteArray resource_bytes(const QString& path) {
  QFile resource(path);
  if (!resource.open(QIODevice::ReadOnly)) {
    throw std::runtime_error(
        QStringLiteral("RFX-WORKSPACE-TEMPLATE-001: missing %1")
            .arg(path)
            .toStdString());
  }
  return resource.readAll();
}

void write_file(const QString& path, const QByteArray& bytes) {
  QSaveFile output(path);
  output.setDirectWriteFallback(false);
  if (!output.open(QIODevice::WriteOnly) ||
      output.write(bytes) != bytes.size() || !output.commit()) {
    throw std::runtime_error(
        QStringLiteral("RFX-WORKSPACE-WRITE-001: cannot commit %1")
            .arg(path)
            .toStdString());
  }
}

void ensure_directory(const QString& path) {
  if (!QDir().mkpath(path)) {
    throw std::runtime_error(
        QStringLiteral("RFX-WORKSPACE-DIRECTORY-001: cannot create %1")
            .arg(path)
            .toStdString());
  }
}

[[nodiscard]] bool allowed_existing_metadata(const QString& name) {
  return name == QStringLiteral(".DS_Store") ||
         name == QStringLiteral("Thumbs.db");
}

void remove_entry(const CreatedEntry& entry) noexcept {
  if (entry.directory) {
    QDir(entry.path).removeRecursively();
  } else {
    QFile::remove(entry.path);
  }
}

[[nodiscard]] QByteArray
project_lock(const refusion::core::ProjectSnapshot& project) {
  return QStringLiteral("format = \"refusion-project-lock\"\n"
                        "format_version = 1\n"
                        "project_id = \"%1\"\n"
                        "rfx_language_version = 6\n"
                        "contract = \"refusion-project-rfx-exp6\"\n"
                        "contract_status = \"experimental\"\n"
                        "visual_property_registry = \"%2\"\n"
                        "visual_contribution_registry = \"%3\"\n"
                        "font_asset font_noto_sans_regular "
                        "sha256:f5f552c8c5edb61fe6efb824baf4d4de47b1a8689ab4925ff43f7bd6a4ebece5\n"
                        "font_asset font_noto_sans_arabic_regular "
                        "sha256:7ed3fe069312aceac454f17cf613a30f95271d6ed7ce58005ed4d016bd3823d7\n")
      .arg(QString::fromStdString(project.project_id.value),
           QString::fromStdString(
               refusion::core::visual_property_registry_digest()),
           QString::fromStdString(
               refusion::core::visual_contribution_registry_digest()))
      .toUtf8();
}

void populate_staging(const QString& staging,
                      const refusion::core::ProjectSnapshot& project,
                      const QByteArray& project_source) {
  ensure_directory(staging +
                   QStringLiteral("/.agents/skills/"
                                  "refusion-project-authoring/agents"));
  ensure_directory(staging +
                   QStringLiteral("/.agents/skills/"
                                  "refusion-project-authoring/references"));
  ensure_directory(staging + QStringLiteral("/Assets/Media"));
  ensure_directory(
      staging + QStringLiteral("/Assets/Fonts/font_noto_sans_regular"));
  ensure_directory(
      staging +
      QStringLiteral("/Assets/Fonts/font_noto_sans_arabic_regular"));

  write_file(
      staging + QStringLiteral("/AGENTS.md"),
      resource_bytes(QStringLiteral(":/refusion/project-template/AGENTS.md")));
  write_file(staging + QStringLiteral("/.gitignore"),
             QByteArray(".refusion/\n"));
  write_file(staging + QStringLiteral("/refusion.lock"), project_lock(project));
  write_file(staging + QStringLiteral("/Assets/Media/.keep"), QByteArray{});
  write_file(
      staging +
          QStringLiteral("/Assets/Fonts/font_noto_sans_regular/font.ttf"),
      resource_bytes(QStringLiteral(
          ":/refusion/fonts/noto_sans_latin_baseline/font.ttf")));
  write_file(
      staging +
          QStringLiteral("/Assets/Fonts/font_noto_sans_regular/OFL.txt"),
      resource_bytes(QStringLiteral(
          ":/refusion/fonts/noto_sans_latin_baseline/OFL.txt")));
  write_file(
      staging + QStringLiteral(
                    "/Assets/Fonts/font_noto_sans_arabic_regular/font.ttf"),
      resource_bytes(QStringLiteral(
          ":/refusion/fonts/noto_sans_arabic_baseline/font.ttf")));
  write_file(
      staging + QStringLiteral(
                    "/Assets/Fonts/font_noto_sans_arabic_regular/OFL.txt"),
      resource_bytes(QStringLiteral(
          ":/refusion/fonts/noto_sans_arabic_baseline/OFL.txt")));
  write_file(
      staging + QStringLiteral("/Assets/Fonts/catalog.lock"),
      QByteArray(
          "schema=refusion.font-assets.v1\n"
          "asset=font_noto_sans_regular family=\"Noto Sans\" face=0 "
          "digest=sha256:f5f552c8c5edb61fe6efb824baf4d4de47b1a8689ab4925ff43f7bd6a4ebece5 "
          "license=OFL-1.1 path=font_noto_sans_regular/font.ttf\n"
          "asset=font_noto_sans_arabic_regular family=\"Noto Sans Arabic\" face=0 "
          "digest=sha256:7ed3fe069312aceac454f17cf613a30f95271d6ed7ce58005ed4d016bd3823d7 "
          "license=OFL-1.1 path=font_noto_sans_arabic_regular/font.ttf\n"));

  const QString skill_root =
      staging + QStringLiteral("/.agents/skills/refusion-project-authoring");
  write_file(
      skill_root + QStringLiteral("/SKILL.md"),
      resource_bytes(QStringLiteral(":/refusion/project-template/SKILL.md")));
  write_file(skill_root + QStringLiteral("/agents/openai.yaml"),
             resource_bytes(
                 QStringLiteral(":/refusion/project-template/openai.yaml")));
  write_file(skill_root +
                 QStringLiteral("/references/supported-capabilities.md"),
             resource_bytes(QStringLiteral(
                 ":/refusion/project-template/supported-capabilities.md")));
  write_file(skill_root + QStringLiteral("/references/coordinates-and-time.md"),
             resource_bytes(QStringLiteral(
                 ":/refusion/project-template/coordinates-and-time.md")));
  write_file(skill_root + QStringLiteral("/references/semantic-authoring.md"),
             resource_bytes(QStringLiteral(
                 ":/refusion/project-template/semantic-authoring.md")));
  write_file(skill_root + QStringLiteral("/references/language-v1.md"),
             resource_bytes(
                 QStringLiteral(":/refusion/project-template/language-v1.md")));
  write_file(skill_root + QStringLiteral("/references/language-v2.md"),
             resource_bytes(
                 QStringLiteral(":/refusion/project-template/language-v2.md")));
  write_file(skill_root + QStringLiteral("/references/language-v3.md"),
             resource_bytes(
                 QStringLiteral(":/refusion/project-template/language-v3.md")));
  write_file(skill_root + QStringLiteral("/references/language-v4.md"),
             resource_bytes(
                 QStringLiteral(":/refusion/project-template/language-v4.md")));
  write_file(skill_root + QStringLiteral("/references/language-v5.md"),
             resource_bytes(
                 QStringLiteral(":/refusion/project-template/language-v5.md")));
  write_file(skill_root + QStringLiteral("/references/language-v6.md"),
             resource_bytes(
                 QStringLiteral(":/refusion/project-template/language-v6.md")));
  const auto registry_markdown =
      refusion::core::visual_property_registry_markdown();
  write_file(skill_root + QStringLiteral("/references/property-registry.md"),
             QByteArray(registry_markdown.data(),
                        static_cast<qsizetype>(registry_markdown.size())));
  const auto contribution_markdown =
      refusion::core::visual_contribution_registry_markdown();
  write_file(skill_root +
                 QStringLiteral("/references/visual-contributions.md"),
             QByteArray(contribution_markdown.data(),
                        static_cast<qsizetype>(contribution_markdown.size())));
  write_file(skill_root +
                 QStringLiteral("/references/diagnostics-and-repair.md"),
             resource_bytes(QStringLiteral(
                 ":/refusion/project-template/diagnostics-and-repair.md")));
  const auto agent_commands =
      refusion::application::agent_command_catalog_markdown();
  write_file(skill_root + QStringLiteral("/references/agent-commands.md"),
             QByteArray(agent_commands.data(),
                        static_cast<qsizetype>(agent_commands.size())));

  // Written in staging now, promoted last below as the root commit marker.
  write_file(staging + QStringLiteral("/Project.rfx"), project_source);
}

} // namespace

WorkspaceCreateResult create_project_workspace(
    const QString& selected_directory,
    const refusion::core::ProjectSnapshot& project) noexcept {
  try {
    const QFileInfo selected_info(selected_directory);
    const QString root = selected_info.canonicalFilePath();
    if (root.isEmpty() || !selected_info.isDir()) {
      return rejected(QStringLiteral("RFX-WORKSPACE-LOCATION-001: selected "
                                     "project folder does not exist"));
    }
    QDir root_directory(root);
    const auto entries = root_directory.entryList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    for (const auto& entry : entries) {
      if (!allowed_existing_metadata(entry)) {
        return rejected(QStringLiteral("RFX-WORKSPACE-NOT-EMPTY: select or "
                                       "create an empty folder; found %1")
                            .arg(entry));
      }
    }

    const auto source_string = refusion::core::serialize_project_rfx(project);
    const QByteArray source(source_string.data(),
                            static_cast<qsizetype>(source_string.size()));
    if (!refusion::core::compile_project_rfx(source_string).succeeded()) {
      return rejected(QStringLiteral(
          "RFX-WORKSPACE-SOURCE-001: generated Project.rfx did not compile"));
    }

    const QString staging = root + QStringLiteral("/.refusion-create-") +
                            QUuid::createUuid().toString(QUuid::WithoutBraces);
    ensure_directory(staging);
    std::vector<CreatedEntry> promoted;
    try {
      populate_staging(staging, project, source);
      const std::array<QString, 6> promotion_order{
          QStringLiteral(".agents"),       QStringLiteral(".gitignore"),
          QStringLiteral("AGENTS.md"),     QStringLiteral("Assets"),
          QStringLiteral("refusion.lock"),
          QStringLiteral("Project.rfx"),
      };
      for (const auto& name : promotion_order) {
        const QString source_path = staging + QLatin1Char('/') + name;
        const QString target_path = root + QLatin1Char('/') + name;
        const bool directory = QFileInfo(source_path).isDir();
        if (!QDir().rename(source_path, target_path)) {
          throw std::runtime_error(
              QStringLiteral("RFX-WORKSPACE-PROMOTE-001: cannot promote %1")
                  .arg(name)
                  .toStdString());
        }
        promoted.push_back(
            CreatedEntry{.path = target_path, .directory = directory});
      }
      QDir(staging).removeRecursively();

      const QString project_path = root + QStringLiteral("/Project.rfx");
      const auto reopened = open_refusion_project(project_path);
      if (!reopened.succeeded()) {
        throw std::runtime_error(reopened.diagnostic.toStdString());
      }
      return WorkspaceCreateResult{
          .workspace =
              CreatedProjectWorkspace{
                  .project_path = reopened.project->canonical_path,
                  .project_directory = root,
              },
      };
    } catch (...) {
      for (auto entry = promoted.rbegin(); entry != promoted.rend(); ++entry) {
        remove_entry(*entry);
      }
      QDir(staging).removeRecursively();
      throw;
    }
  } catch (const std::exception& error) {
    return rejected(QString::fromUtf8(error.what()));
  } catch (...) {
    return rejected(QStringLiteral(
        "RFX-WORKSPACE-INTERNAL-001: unknown workspace creation failure"));
  }
}
