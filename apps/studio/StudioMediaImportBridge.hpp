#pragma once

#include "refusion/application/ImportVideoService.hpp"

#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>
#include <memory>

class StudioBridge;

class StudioMediaImportBridge final
    : public QObject,
      public refusion::application::ImportVideoProgressPort {
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString stage READ stage NOTIFY stageChanged)
  Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY diagnosticChanged)

 public:
  StudioMediaImportBridge(
      refusion::application::ProjectCommandService& commands,
      StudioBridge& studio_bridge,
      QString project_directory,
      std::function<refusion::core::ProjectTimeNs()> timeline_time_provider,
      QObject* parent = nullptr);
  ~StudioMediaImportBridge() override;

  [[nodiscard]] bool busy() const noexcept;
  [[nodiscard]] QString stage() const;
  [[nodiscard]] QString diagnostic() const;

  Q_INVOKABLE void importSelectedFile(const QUrl& selected_file);
  Q_INVOKABLE void cancelImport();

  void report(refusion::application::ImportVideoStage stage) noexcept override;

 signals:
  void busyChanged();
  void stageChanged();
  void diagnosticChanged();
  void importCompleted(bool accepted);

 private:
  class Implementation;
  std::unique_ptr<Implementation> implementation_;
  bool busy_{false};
  QString stage_;
  QString diagnostic_;
};
