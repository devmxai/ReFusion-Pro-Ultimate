#include "StudioBridge.hpp"

#include <string>

StudioBridge::StudioBridge(QObject* parent)
    : QObject(parent),
      authority_(refusion::core::ProjectSnapshot{
          .project_id = refusion::core::ProjectId{"prj_refusion_foundation"},
          .revision_id = refusion::core::RevisionId{1},
          .display_name = "ReFusion Foundation",
      }) {}

QString StudioBridge::projectId() const {
  return QString::fromStdString(authority_.active_snapshot().project_id.value);
}

QString StudioBridge::projectName() const {
  return QString::fromStdString(authority_.active_snapshot().display_name);
}

qulonglong StudioBridge::revision() const {
  return authority_.active_snapshot().revision_id.value;
}

QString StudioBridge::diagnostic() const { return diagnostic_; }

void StudioBridge::submitRename(const QString& requested_name) {
  const auto base = authority_.active_snapshot();
  ++command_sequence_;
  const auto result = authority_.apply(refusion::core::RenameProjectCommand{
      .expected_revision = base.revision_id,
      .requested_name = requested_name.toStdString(),
      .idempotency_key = "qt-command-" + std::to_string(command_sequence_),
  });

  if (result.accepted) {
    diagnostic_.clear();
    emit snapshotChanged();
  } else {
    diagnostic_ = QString::fromStdString(result.diagnostic.code + ": " +
                                         result.diagnostic.message);
  }
  emit diagnosticChanged();
}

