#pragma once

#include "refusion/application/ProjectCreationService.hpp"

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>

#include <memory>

class ProjectLauncherBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList presets READ presets CONSTANT)
  Q_PROPERTY(QVariantList frameRates READ frameRates CONSTANT)
  Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY diagnosticChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

 public:
  explicit ProjectLauncherBridge(QObject* parent = nullptr);

  [[nodiscard]] QVariantList presets() const;
  [[nodiscard]] QVariantList frameRates() const;
  [[nodiscard]] QString diagnostic() const;
  [[nodiscard]] bool busy() const noexcept;

  Q_INVOKABLE void createProject(const QString& display_name,
                                 const QString& preset_id,
                                 const QString& resolution_id,
                                 uint frame_rate,
                                 uint duration_seconds,
                                 const QUrl& selected_folder);
  Q_INVOKABLE void openProjectWorkspace(const QUrl& selected_folder);

  void publishOpenFailure(QString diagnostic);

 signals:
  void projectReady(const QString& project_path);
  void diagnosticChanged();
  void busyChanged();

 private:
  void setDiagnostic(QString diagnostic);
  void setBusy(bool busy);

  std::unique_ptr<refusion::application::ProjectCreationService> creation_;
  QString diagnostic_;
  bool busy_{false};
};
