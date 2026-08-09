#pragma once

#include "refusion/application/ProjectCommandService.hpp"
#include "refusion/core/ProjectRfx.hpp"

#include <QByteArray>
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>

#include <memory>

class QLockFile;
class StudioBridge;

// Filesystem observation is only an input adapter. Candidate compilation,
// revision acceptance and renderer activation remain Core/engine operations.
class ProjectLiveReloadController final : public QObject {
  Q_OBJECT

 public:
  ProjectLiveReloadController(
      refusion::application::ProjectCommandService& commands,
      StudioBridge& bridge,
      QString project_path,
      QString cli_path,
      QByteArray initial_source,
      QObject* parent = nullptr);
  ~ProjectLiveReloadController() override;

  // Records the terminal result of another typed Application workflow (for
  // example ImportVideo) in the same session diagnostics stream consumed by
  // Agents. This has no Revision or persistence authority.
  void recordWorkflowDiagnostic(bool accepted, const QString& diagnostic);

 private slots:
  void projectFileChanged(const QString& path);

 private:
  void ensureWatch();
  void processCandidate();
  void persistAcceptedSnapshot(const refusion::core::ProjectSnapshot& snapshot);
  void writeJournal(const refusion::core::ProjectSnapshot& snapshot,
                    const QByteArray& source);
  void writeAgentContext(const refusion::core::ProjectSnapshot& snapshot);
  void appendDiagnostic(QString status,
                        QString code,
                        QString message,
                        qulonglong candidate_revision,
                        qulonglong active_revision,
                        refusion::core::RfxSourceLocation location = {});

  refusion::application::ProjectCommandService* commands_;
  StudioBridge* bridge_;
  QString project_path_;
  QString project_directory_;
  QString refusion_directory_;
  QString cli_path_;
  QByteArray last_accepted_source_;
  QFileSystemWatcher watcher_;
  std::unique_ptr<QLockFile> session_lock_;
  qulonglong command_sequence_{0};
};
