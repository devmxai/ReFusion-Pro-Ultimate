#pragma once

#include "refusion/application/ProjectCommandService.hpp"

#include <QObject>
#include <QString>

class StudioBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString projectId READ projectId CONSTANT)
  Q_PROPERTY(QString projectName READ projectName NOTIFY snapshotChanged)
  Q_PROPERTY(qulonglong revision READ revision NOTIFY snapshotChanged)
  Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY diagnosticChanged)

 public:
  explicit StudioBridge(refusion::application::ProjectCommandService& commands,
                        QObject* parent = nullptr);

  [[nodiscard]] QString projectId() const;
  [[nodiscard]] QString projectName() const;
  [[nodiscard]] qulonglong revision() const;
  [[nodiscard]] QString diagnostic() const;

  Q_INVOKABLE void submitRename(const QString& requested_name);

 signals:
  void snapshotChanged();
  void diagnosticChanged();

 private:
  refusion::application::ProjectCommandService* commands_;
  QString diagnostic_;
  qulonglong command_sequence_{0};
};
