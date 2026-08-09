#pragma once

#include "refusion/application/ProjectCommandService.hpp"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

class StudioBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString projectId READ projectId CONSTANT)
  Q_PROPERTY(QString projectName READ projectName NOTIFY snapshotChanged)
  Q_PROPERTY(qulonglong revision READ revision NOTIFY snapshotChanged)
  Q_PROPERTY(uint compositionWidth READ compositionWidth NOTIFY snapshotChanged)
  Q_PROPERTY(uint compositionHeight READ compositionHeight NOTIFY snapshotChanged)
  Q_PROPERTY(bool portraitWorkspace READ portraitWorkspace NOTIFY snapshotChanged)
  Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY diagnosticChanged)
  Q_PROPERTY(bool hasVisualSelection READ hasVisualSelection NOTIFY snapshotChanged)
  Q_PROPERTY(QString selectedNodeId READ selectedNodeId NOTIFY snapshotChanged)
  Q_PROPERTY(QString selectedNodeKind READ selectedNodeKind NOTIFY snapshotChanged)
  Q_PROPERTY(QString selectedDisplayName READ selectedDisplayName NOTIFY snapshotChanged)
  Q_PROPERTY(double selectedPositionX READ selectedPositionX NOTIFY snapshotChanged)
  Q_PROPERTY(double selectedPositionY READ selectedPositionY NOTIFY snapshotChanged)
  Q_PROPERTY(double selectedAnchorX READ selectedAnchorX NOTIFY snapshotChanged)
  Q_PROPERTY(double selectedAnchorY READ selectedAnchorY NOTIFY snapshotChanged)
  Q_PROPERTY(double selectedScaleX READ selectedScaleX NOTIFY snapshotChanged)
  Q_PROPERTY(double selectedScaleY READ selectedScaleY NOTIFY snapshotChanged)
  Q_PROPERTY(double selectedRotation READ selectedRotation NOTIFY snapshotChanged)
  Q_PROPERTY(double selectedOpacity READ selectedOpacity NOTIFY snapshotChanged)
  Q_PROPERTY(QVariantList selectedProperties READ selectedProperties NOTIFY snapshotChanged)
  Q_PROPERTY(QVariantList selectedEffects READ selectedEffects NOTIFY snapshotChanged)
  Q_PROPERTY(QVariantList availableEffects READ availableEffects CONSTANT)
  Q_PROPERTY(QVariantMap selectedShapeFill READ selectedShapeFill NOTIFY snapshotChanged)
  Q_PROPERTY(QVariantList selectedMasks READ selectedMasks NOTIFY snapshotChanged)
  Q_PROPERTY(QVariantList availableMasks READ availableMasks CONSTANT)
  Q_PROPERTY(QVariantMap selectedMeasuredBounds READ selectedMeasuredBounds
                 NOTIFY measurementChanged)
  Q_PROPERTY(QVariantList alignmentTargets READ alignmentTargets NOTIFY
                 alignmentTargetsChanged)

 public:
  explicit StudioBridge(
      refusion::application::ProjectCommandService& commands,
      std::shared_ptr<refusion::core::TextLayoutPort> text_layout_port = nullptr,
      QObject* parent = nullptr);

  [[nodiscard]] QString projectId() const;
  [[nodiscard]] QString projectName() const;
  [[nodiscard]] qulonglong revision() const;
  [[nodiscard]] uint compositionWidth() const noexcept;
  [[nodiscard]] uint compositionHeight() const noexcept;
  [[nodiscard]] bool portraitWorkspace() const noexcept;
  [[nodiscard]] QString diagnostic() const;
  [[nodiscard]] bool hasVisualSelection() const;
  [[nodiscard]] QString selectedNodeId() const;
  [[nodiscard]] QString selectedNodeKind() const;
  [[nodiscard]] QString selectedDisplayName() const;
  [[nodiscard]] double selectedPositionX() const;
  [[nodiscard]] double selectedPositionY() const;
  [[nodiscard]] double selectedAnchorX() const;
  [[nodiscard]] double selectedAnchorY() const;
  [[nodiscard]] double selectedScaleX() const;
  [[nodiscard]] double selectedScaleY() const;
  [[nodiscard]] double selectedRotation() const;
  [[nodiscard]] double selectedOpacity() const;
  [[nodiscard]] QVariantList selectedProperties() const;
  [[nodiscard]] QVariantList selectedEffects() const;
  [[nodiscard]] QVariantList availableEffects() const;
  [[nodiscard]] QVariantMap selectedShapeFill() const;
  [[nodiscard]] QVariantList selectedMasks() const;
  [[nodiscard]] QVariantList availableMasks() const;
  [[nodiscard]] QVariantMap selectedMeasuredBounds() const;
  [[nodiscard]] QVariantList alignmentTargets() const;

  using AcceptedObserver =
      std::function<void(const refusion::core::ProjectSnapshot&)>;
  void setAcceptedObserver(AcceptedObserver observer);
  using CompositionTimeProvider =
      std::function<refusion::core::ProjectTimeNs()>;
  void setCompositionTimeProvider(CompositionTimeProvider provider);
  void publishExternalResult(const refusion::core::ApplyResult& result);
  void publishExternalDiagnostic(QString diagnostic);

  Q_INVOKABLE void submitRename(const QString& requested_name);
  Q_INVOKABLE void addVisualLayer(const QString& preset);
  Q_INVOKABLE void selectVisualNode(const QString& node_id, bool is_group);
  Q_INVOKABLE void clearVisualSelection();
  Q_INVOKABLE void submitSelectedTransform(double position_x,
                                           double position_y,
                                           double anchor_x,
                                           double anchor_y,
                                           double scale_x,
                                           double scale_y,
                                           double rotation_degrees,
                                           double opacity);
  Q_INVOKABLE void submitSelectedProperty(const QString& property_id,
                                          const QVariant& value);
  Q_INVOKABLE void submitSelectedShapeFill(const QString& fill_kind,
                                           const QVariantMap& parameters);
  Q_INVOKABLE void addSelectedEffect(const QString& effect_kind);
  Q_INVOKABLE void updateSelectedEffect(const QString& effect_id,
                                        bool enabled,
                                        const QVariantMap& parameters);
  Q_INVOKABLE void removeSelectedEffect(const QString& effect_id);
  Q_INVOKABLE void addSelectedMask(const QString& mask_kind);
  Q_INVOKABLE void addSelectedRoundedRectMask();
  Q_INVOKABLE void updateSelectedMask(const QString& mask_id,
                                      bool enabled,
                                      bool inverted,
                                      const QVariantMap& geometry);
  Q_INVOKABLE void removeSelectedMask(const QString& mask_id);
  Q_INVOKABLE void submitSelectedAlignment(const QString& target_id,
                                           bool target_is_group,
                                           const QString& horizontal,
                                           const QString& vertical,
                                           const QString& bounds_basis);

 public slots:
  void refreshMeasurementProjection();

 signals:
  void snapshotChanged();
  void diagnosticChanged();
  void measurementChanged();
  void alignmentTargetsChanged();

 private:
  [[nodiscard]] const refusion::core::Transform2D* selectedTransform(
      const refusion::core::ProjectSnapshot& snapshot) const;
  [[nodiscard]] const refusion::core::LayerSnapshot* selectedLayer(
      const refusion::core::ProjectSnapshot& snapshot) const;
  void rebuildMeasurementProjection();
  void publishUiResult(const refusion::core::ApplyResult& result);

  refusion::application::ProjectCommandService* commands_;
  QString diagnostic_;
  qulonglong command_sequence_{0};
  AcceptedObserver accepted_observer_;
  CompositionTimeProvider composition_time_provider_;
  std::shared_ptr<refusion::core::TextLayoutPort> text_layout_port_;
  std::optional<refusion::core::VisualNodeRef> selected_visual_node_;
  QVariantMap selected_measured_bounds_;
  QVariantList alignment_targets_;
};
