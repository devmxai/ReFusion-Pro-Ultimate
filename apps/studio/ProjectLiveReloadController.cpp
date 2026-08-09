#include "ProjectLiveReloadController.hpp"

#include "StudioBridge.hpp"
#include "adapters/QtMediaImportWorkspace.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <QTimer>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

enum class HostLocalState : std::uint8_t {
  fresh,
  current,
  relocated,
  unknown,
};

[[nodiscard]] QString canonical_or_absolute(const QString& path) {
  const QFileInfo info(path);
  const auto canonical = info.canonicalFilePath();
  return QDir::cleanPath(canonical.isEmpty() ? info.absoluteFilePath()
                                             : canonical);
}

[[nodiscard]] HostLocalState inspect_host_local_state(
    const QString& project_path,
    const QString& refusion_directory) {
  if (!QFileInfo::exists(refusion_directory)) {
    return HostLocalState::fresh;
  }
  QFile context(refusion_directory + QStringLiteral("/agent-context.json"));
  if (!context.open(QIODevice::ReadOnly)) {
    return HostLocalState::unknown;
  }
  const auto document = QJsonDocument::fromJson(context.readAll());
  if (!document.isObject()) {
    return HostLocalState::unknown;
  }
  const auto recorded =
      document.object().value(QStringLiteral("project_file")).toString();
  if (recorded.isEmpty()) {
    return HostLocalState::unknown;
  }
  return canonical_or_absolute(recorded) == canonical_or_absolute(project_path)
             ? HostLocalState::current
             : HostLocalState::relocated;
}

void remove_directory_if_present(const QString& path) {
  QDir directory(path);
  if (directory.exists() && !directory.removeRecursively()) {
    throw std::runtime_error(
        QStringLiteral("RFX-PROJECT-LOCAL-STATE-RESET: cannot clear %1")
            .arg(path)
            .toStdString());
  }
}

void reset_host_local_state(const QString& refusion_directory) {
  remove_directory_if_present(refusion_directory + QStringLiteral("/Journal"));
  remove_directory_if_present(refusion_directory +
                              QStringLiteral("/Diagnostics"));
  remove_directory_if_present(refusion_directory + QStringLiteral("/Cache"));
  const auto context =
      refusion_directory + QStringLiteral("/agent-context.json");
  if (QFileInfo::exists(context) && !QFile::remove(context)) {
    throw std::runtime_error(
        "RFX-PROJECT-LOCAL-STATE-RESET: cannot clear agent context");
  }
  QDir directory;
  if (!directory.mkpath(refusion_directory + QStringLiteral("/Journal")) ||
      !directory.mkpath(refusion_directory + QStringLiteral("/Diagnostics")) ||
      !directory.mkpath(refusion_directory + QStringLiteral("/Cache"))) {
    throw std::runtime_error(
        "RFX-PROJECT-LOCAL-STATE-RESET: cannot recreate local directories");
  }
}

[[nodiscard]] QByteArray read_all(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    throw std::runtime_error(
        QStringLiteral("RFX-PROJECT-WATCH-IO: %1").arg(file.errorString()).toStdString());
  }
  return file.readAll();
}

void write_atomically(const QString& path, const QByteArray& bytes) {
  QSaveFile output(path);
  if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() ||
      !output.commit()) {
    throw std::runtime_error(
        QStringLiteral("RFX-PROJECT-WRITE-IO: cannot commit %1").arg(path).toStdString());
  }
}

[[nodiscard]] QString compile_diagnostic_text(
    const refusion::core::RfxDiagnostic& diagnostic) {
  return QStringLiteral("%1:%2:%3: %4")
      .arg(QString::fromStdString(diagnostic.code))
      .arg(static_cast<qulonglong>(diagnostic.location.line))
      .arg(static_cast<qulonglong>(diagnostic.location.column))
      .arg(QString::fromStdString(diagnostic.message));
}

}  // namespace

ProjectLiveReloadController::ProjectLiveReloadController(
    refusion::application::ProjectCommandService& commands,
    StudioBridge& bridge,
    QString project_path,
    QString cli_path,
    QByteArray initial_source,
    QObject* parent)
    : QObject(parent),
      commands_(&commands),
      bridge_(&bridge),
      project_path_(std::move(project_path)),
      project_directory_(QFileInfo(project_path_).absolutePath()),
      refusion_directory_(project_directory_ + QStringLiteral("/.refusion")),
      cli_path_(std::move(cli_path)),
      last_accepted_source_(std::move(initial_source)),
      session_lock_(std::make_unique<QLockFile>(
          refusion_directory_ + QStringLiteral("/session.lock"))) {
  const auto host_local_state =
      inspect_host_local_state(project_path_, refusion_directory_);
  QDir directory;
  if (!directory.mkpath(refusion_directory_ + QStringLiteral("/Journal")) ||
      !directory.mkpath(refusion_directory_ + QStringLiteral("/Diagnostics")) ||
      !directory.mkpath(refusion_directory_ + QStringLiteral("/Cache"))) {
    throw std::runtime_error("RFX-PROJECT-WORKSPACE: cannot create .refusion workspace");
  }
  if (host_local_state == HostLocalState::relocated) {
    const auto copied_lock =
        refusion_directory_ + QStringLiteral("/session.lock");
    if (QFileInfo::exists(copied_lock) && !QFile::remove(copied_lock)) {
      throw std::runtime_error(
          "RFX-PROJECT-LOCKED: copied session lock cannot be replaced");
    }
  }
  session_lock_->setStaleLockTime(0);
  if (!session_lock_->tryLock()) {
    const bool recovered_stale_lock = session_lock_->removeStaleLockFile() &&
                                      session_lock_->tryLock();
    if (!recovered_stale_lock) {
      throw std::runtime_error(
          "RFX-PROJECT-LOCKED: another Studio process owns this project session");
    }
  }
  if (host_local_state != HostLocalState::current) {
    reset_host_local_state(refusion_directory_);
  }

  const auto initial = commands_->active_snapshot();
  QString import_recovery_diagnostic;
  if (!recover_incomplete_media_imports(project_directory_, initial,
                                        &import_recovery_diagnostic)) {
    throw std::runtime_error(import_recovery_diagnostic.toStdString());
  }

  connect(&watcher_, &QFileSystemWatcher::fileChanged,
          this, &ProjectLiveReloadController::projectFileChanged);
  connect(&watcher_, &QFileSystemWatcher::directoryChanged,
          this, &ProjectLiveReloadController::projectFileChanged);
  ensureWatch();
  writeJournal(initial, last_accepted_source_);
  writeAgentContext(initial);
  appendDiagnostic(QStringLiteral("accepted"),
                   QStringLiteral("RFX-PROJECT-OPENED"),
                   QStringLiteral("Project.rfx opened as active revision"),
                   initial.revision_id.value,
                   initial.revision_id.value);

  bridge_->setAcceptedObserver(
      [this](const refusion::core::ProjectSnapshot& snapshot) {
        // Runtime publication already completed atomically inside the
        // Application command transaction. This observer persists only the
        // accepted canonical truth; it has no activation authority.
        persistAcceptedSnapshot(snapshot);
      });
}

ProjectLiveReloadController::~ProjectLiveReloadController() {
  if (bridge_ != nullptr) {
    bridge_->setAcceptedObserver({});
  }
}

void ProjectLiveReloadController::recordWorkflowDiagnostic(
    const bool accepted, const QString& diagnostic) {
  const auto separator = diagnostic.indexOf(QStringLiteral(": "));
  const auto code = separator > 0
                        ? diagnostic.left(separator)
                        : QStringLiteral("RFX-WORKFLOW-DIAGNOSTIC");
  const auto message = separator > 0
                           ? diagnostic.sliced(separator + 2)
                           : diagnostic;
  const auto active_revision =
      static_cast<qulonglong>(commands_->active_snapshot().revision_id.value);
  appendDiagnostic(accepted ? QStringLiteral("accepted")
                            : QStringLiteral("rejected"),
                   code, message, active_revision, active_revision);
}

void ProjectLiveReloadController::projectFileChanged(const QString&) {
  try {
    processCandidate();
  } catch (const std::exception& error) {
    const auto diagnostic = QString::fromUtf8(error.what());
    bridge_->publishExternalDiagnostic(diagnostic);
    appendDiagnostic(QStringLiteral("rejected"),
                     QStringLiteral("RFX-PROJECT-WATCH-EXCEPTION"),
                     diagnostic,
                     0,
                     commands_->active_snapshot().revision_id.value);
    QTimer::singleShot(50, this, &ProjectLiveReloadController::ensureWatch);
  }
}

void ProjectLiveReloadController::ensureWatch() {
  if (!watcher_.directories().contains(project_directory_)) {
    watcher_.addPath(project_directory_);
  }
  if (!QFileInfo::exists(project_path_)) {
    return;
  }
  if (watcher_.files().contains(project_path_)) {
    watcher_.removePath(project_path_);
  }
  watcher_.addPath(project_path_);
}

void ProjectLiveReloadController::processCandidate() {
  QByteArray source;
  try {
    source = read_all(project_path_);
  } catch (const std::exception& error) {
    bridge_->publishExternalDiagnostic(QString::fromUtf8(error.what()));
    appendDiagnostic(QStringLiteral("rejected"),
                     QStringLiteral("RFX-PROJECT-WATCH-IO"),
                     QString::fromUtf8(error.what()),
                     0,
                     commands_->active_snapshot().revision_id.value);
    QTimer::singleShot(50, this, &ProjectLiveReloadController::ensureWatch);
    return;
  }
  ensureWatch();
  if (source == last_accepted_source_) {
    return;
  }

  const std::string_view source_view(source.constData(),
                                     static_cast<std::size_t>(source.size()));
  auto compiled = refusion::core::compile_project_rfx(source_view);
  const auto active = commands_->active_snapshot();
  if (!compiled.succeeded()) {
    const auto diagnostic = compiled.diagnostics.empty()
                                ? refusion::core::RfxDiagnostic{
                                      .code = "RFX-RFX-COMPILE-FAILED",
                                      .message = "Project.rfx compile failed",
                                  }
                                : compiled.diagnostics.front();
    bridge_->publishExternalDiagnostic(compile_diagnostic_text(diagnostic));
    appendDiagnostic(QStringLiteral("rejected"),
                     QString::fromStdString(diagnostic.code),
                     QString::fromStdString(diagnostic.message),
                     0,
                     active.revision_id.value,
                     diagnostic.location);
    return;
  }

  const auto& candidate = *compiled.project;
  if (candidate == active) {
    last_accepted_source_ = source;
    return;
  }
  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  const auto result = commands_->submit(refusion::core::ReplaceProjectCommand{
      .envelope = refusion::core::CommandEnvelope{
          .command_id = refusion::core::CommandId{"cmd_rfx_reload_" + suffix},
          .expected_revision = active.revision_id,
          .idempotency_key =
              refusion::core::IdempotencyKey{"rfx-reload-" + suffix},
      },
      .candidate = candidate,
  });
  if (!result.accepted()) {
    bridge_->publishExternalResult(result);
    appendDiagnostic(QStringLiteral("rejected"),
                     QString::fromStdString(result.diagnostic.code),
                     QString::fromStdString(result.diagnostic.message),
                     candidate.revision_id.value,
                     result.active_snapshot.revision_id.value);
    return;
  }

  last_accepted_source_ = source;
  writeJournal(result.active_snapshot, source);
  writeAgentContext(result.active_snapshot);
  appendDiagnostic(QStringLiteral("accepted"),
                   QStringLiteral("RFX-PROJECT-REVISION-ACCEPTED"),
                   QStringLiteral("Project.rfx candidate activated"),
                   candidate.revision_id.value,
                   result.active_snapshot.revision_id.value);
  bridge_->publishExternalResult(result);
}

void ProjectLiveReloadController::persistAcceptedSnapshot(
    const refusion::core::ProjectSnapshot& snapshot) {
  try {
    const auto serialized = refusion::core::serialize_project_rfx(snapshot);
    const QByteArray source(serialized.data(),
                            static_cast<qsizetype>(serialized.size()));
    write_atomically(project_path_, source);
    last_accepted_source_ = source;
    ensureWatch();
    writeJournal(snapshot, source);
    writeAgentContext(snapshot);
    appendDiagnostic(QStringLiteral("accepted"),
                     QStringLiteral("RFX-UI-REVISION-PERSISTED"),
                     QStringLiteral(
                         "Accepted workflow persisted canonical Project.rfx"),
                     snapshot.revision_id.value,
                     snapshot.revision_id.value);
  } catch (const std::exception& error) {
    bridge_->publishExternalDiagnostic(QString::fromUtf8(error.what()));
    appendDiagnostic(QStringLiteral("fatal"),
                     QStringLiteral("RFX-PROJECT-WRITE-IO"),
                     QString::fromUtf8(error.what()),
                     snapshot.revision_id.value,
                     snapshot.revision_id.value);
  }
}

void ProjectLiveReloadController::writeJournal(
    const refusion::core::ProjectSnapshot& snapshot,
    const QByteArray& source) {
  const auto path = refusion_directory_ + QStringLiteral("/Journal/accepted-r%1.rfx")
                                              .arg(snapshot.revision_id.value);
  if (!QFileInfo::exists(path)) {
    write_atomically(path, source);
  }
}

void ProjectLiveReloadController::writeAgentContext(
    const refusion::core::ProjectSnapshot& snapshot) {
  const QJsonObject context{
      {QStringLiteral("schema_version"), 2},
      {QStringLiteral("project_id"),
       QString::fromStdString(snapshot.project_id.value)},
      {QStringLiteral("active_revision"),
       QString::number(static_cast<qulonglong>(snapshot.revision_id.value))},
      {QStringLiteral("project_file"), project_path_},
      {QStringLiteral("cli_executable"), cli_path_},
      {QStringLiteral("diagnostics_file"),
       refusion_directory_ + QStringLiteral("/Diagnostics/session.jsonl")},
      {QStringLiteral("authoring_skill"),
       project_directory_ + QStringLiteral(
                                "/.agents/skills/refusion-project-authoring/SKILL.md")},
  };
  write_atomically(
      refusion_directory_ + QStringLiteral("/agent-context.json"),
      QJsonDocument(context).toJson(QJsonDocument::Indented));
}

void ProjectLiveReloadController::appendDiagnostic(
    QString status,
    QString code,
    QString message,
    const qulonglong candidate_revision,
    const qulonglong active_revision,
    const refusion::core::RfxSourceLocation location) {
  QJsonObject object{
      {QStringLiteral("timestamp_utc"),
       QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
      {QStringLiteral("status"), std::move(status)},
      {QStringLiteral("code"), std::move(code)},
      {QStringLiteral("message"), std::move(message)},
      {QStringLiteral("project_path"), project_path_},
      {QStringLiteral("candidate_revision"),
       QString::number(candidate_revision)},
      {QStringLiteral("active_revision"), QString::number(active_revision)},
      {QStringLiteral("line"), static_cast<qint64>(location.line)},
      {QStringLiteral("column"), static_cast<qint64>(location.column)},
  };
  QFile output(refusion_directory_ + QStringLiteral("/Diagnostics/session.jsonl"));
  if (output.open(QIODevice::WriteOnly | QIODevice::Append)) {
    output.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
    output.write("\n");
  }
}
