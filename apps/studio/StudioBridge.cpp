#include "StudioBridge.hpp"

#include <string>

StudioBridge::StudioBridge(refusion::application::ProjectCommandService& commands,
                           QObject* parent)
    : QObject(parent), commands_(&commands) {}

QString StudioBridge::projectId() const {
  return QString::fromStdString(commands_->active_snapshot().project_id.value);
}

QString StudioBridge::projectName() const {
  return QString::fromStdString(commands_->active_snapshot().display_name);
}

qulonglong StudioBridge::revision() const {
  return commands_->active_snapshot().revision_id.value;
}

QString StudioBridge::diagnostic() const { return diagnostic_; }

void StudioBridge::submitRename(const QString& requested_name) {
  const auto base = commands_->active_snapshot();
  ++command_sequence_;
  const auto result = commands_->submit(refusion::core::RenameProjectCommand{
      .envelope = refusion::core::CommandEnvelope{
          .command_id = refusion::core::CommandId{
              "cmd_qt_rename_" + std::to_string(command_sequence_)},
          .expected_revision = base.revision_id,
          .idempotency_key = refusion::core::IdempotencyKey{
              "qt-command-" + std::to_string(command_sequence_)},
      },
      .requested_name = requested_name.toStdString(),
  });

  if (result.accepted()) {
    diagnostic_.clear();
    emit snapshotChanged();
  } else {
    diagnostic_ = QString::fromStdString(result.diagnostic.code + ": " +
                                         result.diagnostic.message);
  }
  emit diagnosticChanged();
}
