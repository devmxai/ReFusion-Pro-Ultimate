#include "StudioBridge.hpp"

#include "refusion/application/VisualContributionCommands.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"

#include <QVariantMap>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace {

[[nodiscard]] QString property_kind_name(
    const refusion::core::VisualPropertyValueKind kind) {
  using refusion::core::VisualPropertyValueKind;
  switch (kind) {
    case VisualPropertyValueKind::number:
      return QStringLiteral("number");
    case VisualPropertyValueKind::color_rgba8:
      return QStringLiteral("color");
    case VisualPropertyValueKind::text:
      return QStringLiteral("text");
    case VisualPropertyValueKind::boolean:
      return QStringLiteral("boolean");
    case VisualPropertyValueKind::shape_fill:
      return QStringLiteral("paint");
  }
  return QStringLiteral("unsupported");
}

[[nodiscard]] QString color_text(const refusion::core::ColorRgba8& color);

[[nodiscard]] QVariantMap shape_fill_value(
    const refusion::core::ShapeFill& fill) {
  if (const auto* solid = std::get_if<refusion::core::ColorRgba8>(&fill)) {
    return QVariantMap{
        {QStringLiteral("kind"), QStringLiteral("solid")},
        {QStringLiteral("color"), color_text(*solid)},
    };
  }
  if (const auto* linear =
          std::get_if<refusion::core::LinearGradientFill>(&fill)) {
    return QVariantMap{
        {QStringLiteral("kind"), QStringLiteral("linear_gradient")},
        {QStringLiteral("colorA"), color_text(linear->stops.front().color)},
        {QStringLiteral("colorB"), color_text(linear->stops.back().color)},
        {QStringLiteral("startX"), linear->start_x},
        {QStringLiteral("startY"), linear->start_y},
        {QStringLiteral("endX"), linear->end_x},
        {QStringLiteral("endY"), linear->end_y},
        {QStringLiteral("stopCount"),
         static_cast<qulonglong>(linear->stops.size())},
    };
  }
  const auto& radial = std::get<refusion::core::RadialGradientFill>(fill);
  return QVariantMap{
      {QStringLiteral("kind"), QStringLiteral("radial_gradient")},
      {QStringLiteral("colorA"), color_text(radial.stops.front().color)},
      {QStringLiteral("colorB"), color_text(radial.stops.back().color)},
      {QStringLiteral("centerX"), radial.center_x},
      {QStringLiteral("centerY"), radial.center_y},
      {QStringLiteral("radius"), radial.radius},
      {QStringLiteral("stopCount"),
       static_cast<qulonglong>(radial.stops.size())},
  };
}

[[nodiscard]] QString color_text(const refusion::core::ColorRgba8& color) {
  return QStringLiteral("#%1%2%3%4")
      .arg(color.red, 2, 16, QLatin1Char('0'))
      .arg(color.green, 2, 16, QLatin1Char('0'))
      .arg(color.blue, 2, 16, QLatin1Char('0'))
      .arg(color.alpha, 2, 16, QLatin1Char('0'))
      .toUpper();
}

[[nodiscard]] QVariant property_value(
    const refusion::core::VisualPropertyValue& value) {
  if (const auto* number = std::get_if<double>(&value)) {
    return *number;
  }
  if (const auto* color =
          std::get_if<refusion::core::ColorRgba8>(&value)) {
    return color_text(*color);
  }
  if (const auto* text = std::get_if<std::string>(&value)) {
    return QString::fromStdString(*text);
  }
  if (const auto* boolean = std::get_if<bool>(&value)) {
    return *boolean;
  }
  return shape_fill_value(std::get<refusion::core::ShapeFill>(value));
}

[[nodiscard]] std::optional<refusion::core::ColorRgba8> parse_color(
    const QString& text) {
  const auto normalized = text.trimmed();
  if (normalized.size() != 9 || !normalized.startsWith(QLatin1Char('#'))) {
    return std::nullopt;
  }
  bool red_ok = false;
  bool green_ok = false;
  bool blue_ok = false;
  bool alpha_ok = false;
  const auto red = normalized.sliced(1, 2).toUInt(&red_ok, 16);
  const auto green = normalized.sliced(3, 2).toUInt(&green_ok, 16);
  const auto blue = normalized.sliced(5, 2).toUInt(&blue_ok, 16);
  const auto alpha = normalized.sliced(7, 2).toUInt(&alpha_ok, 16);
  if (!red_ok || !green_ok || !blue_ok || !alpha_ok) {
    return std::nullopt;
  }
  return refusion::core::ColorRgba8{
      .red = static_cast<std::uint8_t>(red),
      .green = static_cast<std::uint8_t>(green),
      .blue = static_cast<std::uint8_t>(blue),
      .alpha = static_cast<std::uint8_t>(alpha),
  };
}

[[nodiscard]] QString parameter_kind_name(
    const refusion::core::VisualParameterValueKind kind) {
  using refusion::core::VisualParameterValueKind;
  switch (kind) {
    case VisualParameterValueKind::number:
      return QStringLiteral("number");
    case VisualParameterValueKind::color_rgba8:
      return QStringLiteral("color");
    case VisualParameterValueKind::boolean:
      return QStringLiteral("boolean");
  }
  return QStringLiteral("unsupported");
}

[[nodiscard]] QVariant parameter_value(
    const refusion::core::VisualParameterValue& value) {
  if (const auto* number = std::get_if<double>(&value)) return *number;
  if (const auto* color =
          std::get_if<refusion::core::ColorRgba8>(&value)) {
    return color_text(*color);
  }
  return std::get<bool>(value);
}

[[nodiscard]] QString parameter_ui_key(const std::string& parameter_id) {
  QString result;
  bool uppercase_next = false;
  for (const char character : parameter_id) {
    if (character == '_') {
      uppercase_next = true;
      continue;
    }
    const QChar value = QLatin1Char(character);
    result.append(uppercase_next ? value.toUpper() : value);
    uppercase_next = false;
  }
  return result;
}

[[nodiscard]] QVariantMap parameter_descriptor_value(
    const refusion::core::VisualParameterDescriptor& descriptor) {
  QVariantMap result{
      {QStringLiteral("id"), QString::fromStdString(descriptor.id)},
      {QStringLiteral("label"),
       QString::fromStdString(descriptor.display_name)},
      {QStringLiteral("kind"), parameter_kind_name(descriptor.value_kind)},
      {QStringLiteral("unit"), QString::fromStdString(descriptor.unit)},
      {QStringLiteral("animatable"), descriptor.animatable},
  };
  if (descriptor.minimum) {
    result.insert(QStringLiteral("minimum"), *descriptor.minimum);
    result.insert(QStringLiteral("minimumInclusive"),
                  descriptor.minimum_inclusive);
  }
  if (descriptor.maximum) {
    result.insert(QStringLiteral("maximum"), *descriptor.maximum);
    result.insert(QStringLiteral("maximumInclusive"),
                  descriptor.maximum_inclusive);
  }
  return result;
}

[[nodiscard]] QVariantMap contribution_descriptor_value(
    const refusion::core::VisualContributionDescriptor& descriptor) {
  QVariantList parameters;
  parameters.reserve(static_cast<qsizetype>(descriptor.parameters.size()));
  for (const auto& parameter : descriptor.parameters) {
    parameters.push_back(parameter_descriptor_value(parameter));
  }
  return QVariantMap{
      {QStringLiteral("kind"), QString::fromStdString(descriptor.id)},
      {QStringLiteral("label"),
       QString::fromStdString(descriptor.display_name)},
      {QStringLiteral("capabilityId"),
       QString::fromStdString(descriptor.capability_id)},
      {QStringLiteral("schemaVersion"), descriptor.schema_version},
      {QStringLiteral("parameters"), parameters},
  };
}

[[nodiscard]] QVariantList parameter_records_value(
    const std::vector<refusion::core::VisualParameterRecord>& records,
    QVariantMap* flat_projection = nullptr) {
  QVariantList result;
  result.reserve(static_cast<qsizetype>(records.size()));
  for (const auto& record : records) {
    auto item = parameter_descriptor_value(record.descriptor);
    const auto value = parameter_value(record.value);
    item.insert(QStringLiteral("value"), value);
    result.push_back(item);
    if (flat_projection != nullptr) {
      flat_projection->insert(parameter_ui_key(record.descriptor.id), value);
    }
  }
  return result;
}

[[nodiscard]] std::optional<refusion::core::VisualParameterValue>
parse_parameter_value(
    const refusion::core::VisualParameterDescriptor& descriptor,
    const QVariantMap& values) {
  const auto canonical_key = QString::fromStdString(descriptor.id);
  const auto ui_key = parameter_ui_key(descriptor.id);
  const auto input = values.contains(canonical_key)
                         ? values.value(canonical_key)
                         : values.value(ui_key);
  switch (descriptor.value_kind) {
    case refusion::core::VisualParameterValueKind::number: {
      bool valid = false;
      const auto number = input.toString().toDouble(&valid);
      if (valid) return number;
      break;
    }
    case refusion::core::VisualParameterValueKind::color_rgba8:
      if (const auto color = parse_color(input.toString())) return *color;
      break;
    case refusion::core::VisualParameterValueKind::boolean: {
      if (input.metaType().id() == QMetaType::Bool) return input.toBool();
      const auto normalized = input.toString().trimmed().toLower();
      if (normalized == QStringLiteral("true") ||
          normalized == QStringLiteral("1")) return true;
      if (normalized == QStringLiteral("false") ||
          normalized == QStringLiteral("0")) return false;
      break;
    }
  }
  return std::nullopt;
}

[[nodiscard]] QVariantMap rect_value(const refusion::core::LocalRect& rect) {
  return QVariantMap{
      {QStringLiteral("left"), rect.left},
      {QStringLiteral("top"), rect.top},
      {QStringLiteral("right"), rect.right},
      {QStringLiteral("bottom"), rect.bottom},
      {QStringLiteral("width"), rect.right - rect.left},
      {QStringLiteral("height"), rect.bottom - rect.top},
  };
}

[[nodiscard]] std::optional<refusion::core::HorizontalAlignIntent>
horizontal_intent(const QString& value) {
  using refusion::core::HorizontalAlignIntent;
  const auto normalized = value.trimmed().toLower();
  if (normalized == QStringLiteral("none")) {
    return HorizontalAlignIntent::none;
  }
  if (normalized == QStringLiteral("left")) {
    return HorizontalAlignIntent::left;
  }
  if (normalized == QStringLiteral("center")) {
    return HorizontalAlignIntent::center;
  }
  if (normalized == QStringLiteral("right")) {
    return HorizontalAlignIntent::right;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<refusion::core::VerticalAlignIntent>
vertical_intent(const QString& value) {
  using refusion::core::VerticalAlignIntent;
  const auto normalized = value.trimmed().toLower();
  if (normalized == QStringLiteral("none")) {
    return VerticalAlignIntent::none;
  }
  if (normalized == QStringLiteral("top")) {
    return VerticalAlignIntent::top;
  }
  if (normalized == QStringLiteral("center")) {
    return VerticalAlignIntent::center;
  }
  if (normalized == QStringLiteral("bottom")) {
    return VerticalAlignIntent::bottom;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<refusion::core::AlignmentBoundsBasis>
alignment_basis(const QString& value) {
  using refusion::core::AlignmentBoundsBasis;
  const auto normalized = value.trimmed().toLower();
  if (normalized == QStringLiteral("geometry")) {
    return AlignmentBoundsBasis::geometry;
  }
  if (normalized == QStringLiteral("logical")) {
    return AlignmentBoundsBasis::logical;
  }
  if (normalized == QStringLiteral("ink")) {
    return AlignmentBoundsBasis::ink;
  }
  return std::nullopt;
}

}  // namespace

StudioBridge::StudioBridge(
    refusion::application::ProjectCommandService& commands,
    std::shared_ptr<refusion::core::TextLayoutPort> text_layout_port,
    QObject* parent)
    : QObject(parent),
      commands_(&commands),
      composition_time_provider_([] { return refusion::core::ProjectTimeNs{0}; }),
      text_layout_port_(std::move(text_layout_port)) {}

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

const refusion::core::Transform2D* StudioBridge::selectedTransform(
    const refusion::core::ProjectSnapshot& snapshot) const {
  if (!selected_visual_node_ || !snapshot.composition) {
    return nullptr;
  }
  if (const auto* layer_id =
          std::get_if<refusion::core::LayerId>(&*selected_visual_node_)) {
    const auto* layer =
        refusion::core::find_layer(*snapshot.composition, *layer_id);
    return layer == nullptr ? nullptr : &layer->transform;
  }
  const auto* group = refusion::core::find_layer_group(
      *snapshot.composition,
      std::get<refusion::core::LayerGroupId>(*selected_visual_node_));
  return group == nullptr ? nullptr : &group->transform;
}

const refusion::core::LayerSnapshot* StudioBridge::selectedLayer(
    const refusion::core::ProjectSnapshot& snapshot) const {
  if (!selected_visual_node_ || !snapshot.composition) {
    return nullptr;
  }
  const auto* layer_id =
      std::get_if<refusion::core::LayerId>(&*selected_visual_node_);
  return layer_id == nullptr
             ? nullptr
             : refusion::core::find_layer(*snapshot.composition, *layer_id);
}

bool StudioBridge::hasVisualSelection() const {
  return selectedTransform(commands_->active_snapshot()) != nullptr;
}

QString StudioBridge::selectedNodeId() const {
  const auto snapshot = commands_->active_snapshot();
  if (selectedTransform(snapshot) == nullptr) {
    return {};
  }
  if (const auto* layer =
          std::get_if<refusion::core::LayerId>(&*selected_visual_node_)) {
    return QString::fromStdString(layer->value);
  }
  return QString::fromStdString(
      std::get<refusion::core::LayerGroupId>(*selected_visual_node_).value);
}

QString StudioBridge::selectedNodeKind() const {
  const auto snapshot = commands_->active_snapshot();
  if (selectedTransform(snapshot) == nullptr || !snapshot.composition) {
    return {};
  }
  if (const auto* group =
          std::get_if<refusion::core::LayerGroupId>(&*selected_visual_node_)) {
    static_cast<void>(group);
    return QStringLiteral("Group");
  }
  const auto* layer = refusion::core::find_layer(
      *snapshot.composition,
      std::get<refusion::core::LayerId>(*selected_visual_node_));
  if (layer == nullptr) {
    return {};
  }
  return std::holds_alternative<refusion::core::ShapeLayerContent>(
             layer->content)
             ? QStringLiteral("Shape")
             : QStringLiteral("Text");
}

QString StudioBridge::selectedDisplayName() const {
  const auto snapshot = commands_->active_snapshot();
  if (selectedTransform(snapshot) == nullptr || !snapshot.composition) {
    return {};
  }
  if (const auto* layer_id =
          std::get_if<refusion::core::LayerId>(&*selected_visual_node_)) {
    const auto* layer = refusion::core::find_layer(*snapshot.composition, *layer_id);
    return layer == nullptr ? QString{}
                            : QString::fromStdString(layer->display_name);
  }
  const auto* group = refusion::core::find_layer_group(
      *snapshot.composition,
      std::get<refusion::core::LayerGroupId>(*selected_visual_node_));
  return group == nullptr ? QString{}
                          : QString::fromStdString(group->display_name);
}

double StudioBridge::selectedPositionX() const {
  const auto snapshot = commands_->active_snapshot();
  const auto* transform = selectedTransform(snapshot);
  return transform == nullptr ? 0.0 : transform->position_x;
}

double StudioBridge::selectedPositionY() const {
  const auto snapshot = commands_->active_snapshot();
  const auto* transform = selectedTransform(snapshot);
  return transform == nullptr ? 0.0 : transform->position_y;
}

double StudioBridge::selectedAnchorX() const {
  const auto snapshot = commands_->active_snapshot();
  const auto* transform = selectedTransform(snapshot);
  return transform == nullptr ? 0.0 : transform->anchor_x;
}

double StudioBridge::selectedAnchorY() const {
  const auto snapshot = commands_->active_snapshot();
  const auto* transform = selectedTransform(snapshot);
  return transform == nullptr ? 0.0 : transform->anchor_y;
}

double StudioBridge::selectedScaleX() const {
  const auto snapshot = commands_->active_snapshot();
  const auto* transform = selectedTransform(snapshot);
  return transform == nullptr ? 1.0 : transform->scale_x;
}

double StudioBridge::selectedScaleY() const {
  const auto snapshot = commands_->active_snapshot();
  const auto* transform = selectedTransform(snapshot);
  return transform == nullptr ? 1.0 : transform->scale_y;
}

double StudioBridge::selectedRotation() const {
  const auto snapshot = commands_->active_snapshot();
  const auto* transform = selectedTransform(snapshot);
  return transform == nullptr ? 0.0 : transform->rotation_degrees;
}

double StudioBridge::selectedOpacity() const {
  const auto snapshot = commands_->active_snapshot();
  const auto* transform = selectedTransform(snapshot);
  return transform == nullptr ? 1.0 : transform->opacity;
}

QVariantList StudioBridge::selectedProperties() const {
  QVariantList result;
  const auto snapshot = commands_->active_snapshot();
  if (!selected_visual_node_ || !snapshot.composition) {
    return result;
  }
  const auto properties = refusion::core::inspect_visual_properties(
      *snapshot.composition, *selected_visual_node_);
  result.reserve(static_cast<qsizetype>(properties.size()));
  for (const auto& property : properties) {
    QVariantMap item{
        {QStringLiteral("id"),
         QString::fromStdString(property.descriptor.id.value)},
        {QStringLiteral("label"),
         QString::fromStdString(property.descriptor.display_name)},
        {QStringLiteral("kind"),
         property_kind_name(property.descriptor.value_kind)},
        {QStringLiteral("unit"),
         QString::fromStdString(property.descriptor.unit)},
        {QStringLiteral("value"), property_value(property.value)},
        {QStringLiteral("animatable"), property.descriptor.animatable},
        {QStringLiteral("writable"), property.descriptor.writable},
    };
    if (property.descriptor.minimum) {
      item.insert(QStringLiteral("minimum"), *property.descriptor.minimum);
    }
    if (property.descriptor.maximum) {
      item.insert(QStringLiteral("maximum"), *property.descriptor.maximum);
    }
    result.push_back(item);
  }
  return result;
}

QVariantList StudioBridge::selectedEffects() const {
  QVariantList result;
  const auto snapshot = commands_->active_snapshot();
  const auto* layer = selectedLayer(snapshot);
  if (layer == nullptr) {
    return result;
  }
  result.reserve(static_cast<qsizetype>(layer->effects.size()));
  for (const auto& effect : layer->effects) {
    const auto kind = refusion::core::visual_effect_kind(effect);
    const auto* descriptor =
        refusion::core::find_visual_contribution_descriptor(kind);
    if (descriptor == nullptr) continue;
    QVariantMap item{
        {QStringLiteral("id"),
         QString::fromStdString(effect.effect_id.value)},
        {QStringLiteral("enabled"), effect.enabled},
        {QStringLiteral("kind"), QString::fromStdString(descriptor->id)},
        {QStringLiteral("label"),
         QString::fromStdString(descriptor->display_name)},
        {QStringLiteral("capabilityId"),
         QString::fromStdString(descriptor->capability_id)},
    };
    item.insert(
        QStringLiteral("parameters"),
        parameter_records_value(
            refusion::core::inspect_visual_effect_parameters(effect), &item));
    result.push_back(item);
  }
  return result;
}

QVariantList StudioBridge::availableEffects() const {
  QVariantList result;
  for (const auto& descriptor :
       refusion::core::visual_contribution_descriptors()) {
    if (descriptor.category ==
        refusion::core::VisualContributionCategory::effect) {
      result.push_back(contribution_descriptor_value(descriptor));
    }
  }
  return result;
}

QVariantMap StudioBridge::selectedShapeFill() const {
  const auto snapshot = commands_->active_snapshot();
  const auto* layer = selectedLayer(snapshot);
  if (layer == nullptr) {
    return {};
  }
  const auto* shape =
      std::get_if<refusion::core::ShapeLayerContent>(&layer->content);
  return shape == nullptr ? QVariantMap{} : shape_fill_value(shape->fill);
}

QVariantList StudioBridge::selectedMasks() const {
  QVariantList result;
  const auto snapshot = commands_->active_snapshot();
  const auto* layer = selectedLayer(snapshot);
  if (layer == nullptr) {
    return result;
  }
  result.reserve(static_cast<qsizetype>(layer->masks.size()));
  for (const auto& mask : layer->masks) {
    const auto kind = refusion::core::visual_mask_kind(mask);
    const auto* descriptor =
        refusion::core::find_visual_contribution_descriptor(kind);
    if (descriptor == nullptr) continue;
    QVariantMap item{
        {QStringLiteral("id"), QString::fromStdString(mask.mask_id.value)},
        {QStringLiteral("kind"), QString::fromStdString(descriptor->id)},
        {QStringLiteral("label"),
         QString::fromStdString(descriptor->display_name)},
        {QStringLiteral("capabilityId"),
         QString::fromStdString(descriptor->capability_id)},
        {QStringLiteral("enabled"), mask.enabled},
        {QStringLiteral("inverted"), mask.inverted},
    };
    item.insert(
        QStringLiteral("parameters"),
        parameter_records_value(
            refusion::core::inspect_visual_mask_parameters(mask), &item));
    result.push_back(item);
  }
  return result;
}

QVariantList StudioBridge::availableMasks() const {
  QVariantList result;
  for (const auto& descriptor :
       refusion::core::visual_contribution_descriptors()) {
    if (descriptor.category ==
        refusion::core::VisualContributionCategory::mask) {
      result.push_back(contribution_descriptor_value(descriptor));
    }
  }
  return result;
}

QVariantMap StudioBridge::selectedMeasuredBounds() const {
  return selected_measured_bounds_;
}

QVariantList StudioBridge::alignmentTargets() const {
  return alignment_targets_;
}

void StudioBridge::rebuildMeasurementProjection() {
  QVariantMap next_bounds;
  QVariantList next_targets;
  const auto snapshot = commands_->active_snapshot();
  if (!selected_visual_node_ || !snapshot.composition ||
      selectedTransform(snapshot) == nullptr) {
    selected_measured_bounds_ = std::move(next_bounds);
    alignment_targets_ = std::move(next_targets);
    return;
  }

  refusion::core::ProjectTimeNs composition_time{0};
  try {
    composition_time = composition_time_provider_();
  } catch (const std::exception& error) {
    selected_measured_bounds_ = QVariantMap{
        {QStringLiteral("available"), false},
        {QStringLiteral("code"), QStringLiteral("RFX-MEASURE-TIME-SOURCE-001")},
        {QStringLiteral("message"), QString::fromUtf8(error.what())},
    };
    alignment_targets_.clear();
    return;
  }

  next_bounds.insert(QStringLiteral("timeNs"),
                     QVariant::fromValue<qulonglong>(composition_time));
  try {
    const auto transforms = refusion::core::evaluate_visual_node_transforms(
        *snapshot.composition, composition_time);
    next_targets.reserve(static_cast<qsizetype>(transforms.size()));
    for (const auto& evaluated : transforms) {
      if (evaluated.node == *selected_visual_node_ ||
          refusion::core::visual_node_is_ancestor(
              *snapshot.composition, *selected_visual_node_, evaluated.node) ||
          refusion::core::visual_node_is_ancestor(
              *snapshot.composition, evaluated.node, *selected_visual_node_)) {
        continue;
      }
      QVariantMap target;
      if (const auto* layer_id =
              std::get_if<refusion::core::LayerId>(&evaluated.node)) {
        const auto* layer =
            refusion::core::find_layer(*snapshot.composition, *layer_id);
        if (layer == nullptr) {
          continue;
        }
        target = QVariantMap{
            {QStringLiteral("id"), QString::fromStdString(layer_id->value)},
            {QStringLiteral("displayName"),
             QString::fromStdString(layer->display_name)},
            {QStringLiteral("isGroup"), false},
            {QStringLiteral("kind"), QStringLiteral("Layer")},
        };
      } else {
        const auto group_id =
            std::get<refusion::core::LayerGroupId>(evaluated.node);
        const auto* group =
            refusion::core::find_layer_group(*snapshot.composition, group_id);
        if (group == nullptr) {
          continue;
        }
        target = QVariantMap{
            {QStringLiteral("id"), QString::fromStdString(group_id.value)},
            {QStringLiteral("displayName"),
             QString::fromStdString(group->display_name)},
            {QStringLiteral("isGroup"), true},
            {QStringLiteral("kind"), QStringLiteral("Group")},
        };
      }
      next_targets.push_back(std::move(target));
    }
  } catch (const std::exception& error) {
    next_bounds.insert(QStringLiteral("available"), false);
    next_bounds.insert(QStringLiteral("code"),
                       QStringLiteral("RFX-MEASURE-EVALUATION-001"));
    next_bounds.insert(QStringLiteral("message"),
                       QString::fromUtf8(error.what()));
    selected_measured_bounds_ = std::move(next_bounds);
    alignment_targets_ = std::move(next_targets);
    return;
  }

  const auto measured = refusion::core::measure_visual_nodes(
      *snapshot.composition, composition_time, text_layout_port_.get());
  if (!measured.succeeded()) {
    next_bounds.insert(QStringLiteral("available"), false);
    next_bounds.insert(QStringLiteral("code"),
                       QString::fromStdString(measured.diagnostic->code));
    next_bounds.insert(QStringLiteral("message"),
                       QString::fromStdString(measured.diagnostic->message));
  } else {
    const auto* selected = refusion::core::find_visual_measurement(
        *measured.snapshot, *selected_visual_node_);
    if (selected == nullptr) {
      next_bounds.insert(QStringLiteral("available"), false);
      next_bounds.insert(QStringLiteral("code"),
                         QStringLiteral("RFX-MEASURE-NODE-INACTIVE-001"));
      next_bounds.insert(QStringLiteral("message"),
                         QStringLiteral("selected node is inactive"));
    } else {
      next_bounds.insert(QStringLiteral("available"), true);
      next_bounds.insert(QStringLiteral("geometry"),
                         rect_value(selected->geometry_world));
      next_bounds.insert(QStringLiteral("logicalAvailable"),
                         selected->logical_world.has_value());
      next_bounds.insert(QStringLiteral("inkAvailable"),
                         selected->ink_world.has_value());
      if (selected->logical_world) {
        next_bounds.insert(QStringLiteral("logical"),
                           rect_value(*selected->logical_world));
      }
      if (selected->ink_world) {
        next_bounds.insert(QStringLiteral("ink"),
                           rect_value(*selected->ink_world));
      }
      next_bounds.insert(
          QStringLiteral("layoutEngineDigest"),
          QString::fromStdString(measured.snapshot->layout_engine_digest));
    }
  }
  selected_measured_bounds_ = std::move(next_bounds);
  alignment_targets_ = std::move(next_targets);
}

void StudioBridge::refreshMeasurementProjection() {
  const auto previous_bounds = selected_measured_bounds_;
  const auto previous_targets = alignment_targets_;
  rebuildMeasurementProjection();
  if (previous_bounds != selected_measured_bounds_) {
    emit measurementChanged();
  }
  if (previous_targets != alignment_targets_) {
    emit alignmentTargetsChanged();
  }
}

void StudioBridge::setAcceptedObserver(AcceptedObserver observer) {
  accepted_observer_ = std::move(observer);
}

void StudioBridge::setCompositionTimeProvider(
    CompositionTimeProvider provider) {
  if (provider) {
    composition_time_provider_ = std::move(provider);
  } else {
    composition_time_provider_ = [] {
      return refusion::core::ProjectTimeNs{0};
    };
  }
  refreshMeasurementProjection();
}

void StudioBridge::publishExternalResult(
    const refusion::core::ApplyResult& result) {
  if (result.accepted()) {
    diagnostic_.clear();
    refreshMeasurementProjection();
    emit snapshotChanged();
  } else {
    diagnostic_ = QString::fromStdString(result.diagnostic.code + ": " +
                                         result.diagnostic.message);
  }
  emit diagnosticChanged();
}

void StudioBridge::publishExternalDiagnostic(QString diagnostic) {
  diagnostic_ = std::move(diagnostic);
  emit diagnosticChanged();
}

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

  publishUiResult(result);
}

void StudioBridge::addVisualLayer(const QString& preset) {
  const auto base = commands_->active_snapshot();
  std::optional<refusion::core::VisualLayerPreset> typed_preset;
  if (preset == QStringLiteral("BG")) {
    typed_preset = refusion::core::VisualLayerPreset::background;
  } else if (preset == QStringLiteral("SHP")) {
    typed_preset = refusion::core::VisualLayerPreset::shape;
  } else if (preset == QStringLiteral("TXT")) {
    typed_preset = refusion::core::VisualLayerPreset::text;
  }
  if (!typed_preset) {
    diagnostic_ = QStringLiteral(
        "RFX-LAYER-PRESET-501: this creation preset is not implemented in the current visual slice");
    emit diagnosticChanged();
    return;
  }
  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  const auto result = commands_->submit(refusion::core::AddVisualLayerCommand{
      .envelope = refusion::core::CommandEnvelope{
          .command_id = refusion::core::CommandId{
              "cmd_qt_add_visual_layer_" + suffix},
          .expected_revision = base.revision_id,
          .idempotency_key = refusion::core::IdempotencyKey{
              "qt-add-visual-layer-" + suffix},
      },
      .preset = *typed_preset,
  });
  if (result.accepted() && result.active_snapshot.composition) {
    for (const auto& layer : result.active_snapshot.composition->layers) {
      if (!base.composition ||
          refusion::core::find_layer(*base.composition, layer.layer_id) ==
              nullptr) {
        selected_visual_node_ = refusion::core::VisualNodeRef{layer.layer_id};
        break;
      }
    }
  }
  publishUiResult(result);
}

void StudioBridge::selectVisualNode(const QString& node_id,
                                    const bool is_group) {
  const auto snapshot = commands_->active_snapshot();
  if (!snapshot.composition) {
    diagnostic_ = QStringLiteral(
        "RFX-VISUAL-SELECTION-001: project has no Composition");
    emit diagnosticChanged();
    return;
  }

  refusion::core::VisualNodeRef candidate =
      is_group
          ? refusion::core::VisualNodeRef{refusion::core::LayerGroupId{
                node_id.toStdString()}}
          : refusion::core::VisualNodeRef{
                refusion::core::LayerId{node_id.toStdString()}};
  selected_visual_node_ = std::move(candidate);
  if (selectedTransform(snapshot) == nullptr) {
    selected_visual_node_.reset();
    diagnostic_ = QStringLiteral(
        "RFX-VISUAL-SELECTION-404: selected visual node does not exist");
    emit diagnosticChanged();
    refreshMeasurementProjection();
    emit snapshotChanged();
    return;
  }
  if (!diagnostic_.isEmpty()) {
    diagnostic_.clear();
    emit diagnosticChanged();
  }
  refreshMeasurementProjection();
  emit snapshotChanged();
}

void StudioBridge::clearVisualSelection() {
  if (!selected_visual_node_) {
    return;
  }
  selected_visual_node_.reset();
  refreshMeasurementProjection();
  emit snapshotChanged();
}

void StudioBridge::submitSelectedTransform(
    const double position_x,
    const double position_y,
    const double anchor_x,
    const double anchor_y,
    const double scale_x,
    const double scale_y,
    const double rotation_degrees,
    const double opacity) {
  const auto base = commands_->active_snapshot();
  if (!selected_visual_node_ || selectedTransform(base) == nullptr) {
    diagnostic_ = QStringLiteral(
        "RFX-VISUAL-SELECTION-002: select a Layer or Group before editing");
    emit diagnosticChanged();
    return;
  }

  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  const auto result =
      commands_->submit(refusion::core::SetVisualTransformCommand{
          .envelope = refusion::core::CommandEnvelope{
              .command_id = refusion::core::CommandId{
                  "cmd_qt_visual_transform_" + suffix},
              .expected_revision = base.revision_id,
              .idempotency_key = refusion::core::IdempotencyKey{
                  "qt-visual-transform-" + suffix},
          },
          .node = *selected_visual_node_,
          .transform = refusion::core::Transform2D{
              .position_x = position_x,
              .position_y = position_y,
              .anchor_x = anchor_x,
              .anchor_y = anchor_y,
              .scale_x = scale_x,
              .scale_y = scale_y,
              .rotation_degrees = rotation_degrees,
              .opacity = opacity,
          },
      });
  publishUiResult(result);
}

void StudioBridge::submitSelectedProperty(const QString& property_id,
                                          const QVariant& value) {
  const auto base = commands_->active_snapshot();
  if (!selected_visual_node_ || !base.composition) {
    diagnostic_ = QStringLiteral(
        "RFX-VISUAL-SELECTION-002: select a Layer or Group before editing");
    emit diagnosticChanged();
    return;
  }
  const auto properties = refusion::core::inspect_visual_properties(
      *base.composition, *selected_visual_node_);
  const auto found = std::find_if(
      properties.begin(), properties.end(), [&property_id](const auto& item) {
        return item.descriptor.id.value == property_id.toStdString();
      });
  if (found == properties.end()) {
    diagnostic_ = QStringLiteral(
        "RFX-PROPERTY-404: selected property is not available for this node");
    emit diagnosticChanged();
    return;
  }
  if (!found->descriptor.writable) {
    diagnostic_ = QStringLiteral(
        "RFX-PROPERTY-READONLY-400: selected property is read-only");
    emit diagnosticChanged();
    return;
  }

  std::optional<refusion::core::VisualPropertyValue> typed_value;
  using refusion::core::VisualPropertyValueKind;
  switch (found->descriptor.value_kind) {
    case VisualPropertyValueKind::number: {
      bool valid = false;
      const double number = value.toString().toDouble(&valid);
      if (valid) {
        typed_value = number;
      }
      break;
    }
    case VisualPropertyValueKind::color_rgba8:
      if (const auto color = parse_color(value.toString())) {
        typed_value = *color;
      }
      break;
    case VisualPropertyValueKind::text:
      typed_value = value.toString().toStdString();
      break;
    case VisualPropertyValueKind::boolean: {
      const auto normalized = value.toString().trimmed().toLower();
      if (normalized == QStringLiteral("true") ||
          normalized == QStringLiteral("1")) {
        typed_value = true;
      } else if (normalized == QStringLiteral("false") ||
                 normalized == QStringLiteral("0")) {
        typed_value = false;
      }
      break;
    }
    case VisualPropertyValueKind::shape_fill:
      break;
  }
  if (!typed_value) {
    diagnostic_ = QStringLiteral(
        "RFX-PROPERTY-INPUT-400: value does not match the property type; colors require #RRGGBBAA");
    emit diagnosticChanged();
    return;
  }

  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  const auto result =
      commands_->submit(refusion::core::SetVisualPropertyCommand{
          .envelope = refusion::core::CommandEnvelope{
              .command_id = refusion::core::CommandId{
                  "cmd_qt_visual_property_" + suffix},
              .expected_revision = base.revision_id,
              .idempotency_key = refusion::core::IdempotencyKey{
                  "qt-visual-property-" + suffix},
          },
          .node = *selected_visual_node_,
          .property_id =
              refusion::core::VisualPropertyId{property_id.toStdString()},
          .value = std::move(*typed_value),
      });
  publishUiResult(result);
}

void StudioBridge::submitSelectedShapeFill(
    const QString& fill_kind,
    const QVariantMap& parameters) {
  const auto base = commands_->active_snapshot();
  const auto* layer = selectedLayer(base);
  if (layer == nullptr ||
      !std::holds_alternative<refusion::core::ShapeLayerContent>(
          layer->content)) {
    diagnostic_ = QStringLiteral(
        "RFX-FILL-SELECTION-002: select a Shape Layer before editing its fill");
    emit diagnosticChanged();
    return;
  }
  const auto number = [&parameters](const QString& key)
      -> std::optional<double> {
    bool valid = false;
    const auto value = parameters.value(key).toString().toDouble(&valid);
    return valid ? std::optional<double>{value} : std::nullopt;
  };
  std::optional<refusion::core::ShapeFill> fill;
  if (fill_kind == QStringLiteral("solid")) {
    if (const auto color =
            parse_color(parameters.value(QStringLiteral("color")).toString())) {
      fill = refusion::core::ShapeFill{*color};
    }
  } else if (fill_kind == QStringLiteral("linear_gradient")) {
    const auto color_a =
        parse_color(parameters.value(QStringLiteral("colorA")).toString());
    const auto color_b =
        parse_color(parameters.value(QStringLiteral("colorB")).toString());
    const auto start_x = number(QStringLiteral("startX"));
    const auto start_y = number(QStringLiteral("startY"));
    const auto end_x = number(QStringLiteral("endX"));
    const auto end_y = number(QStringLiteral("endY"));
    if (color_a && color_b && start_x && start_y && end_x && end_y) {
      fill = refusion::core::ShapeFill{refusion::core::LinearGradientFill{
          .start_x = *start_x,
          .start_y = *start_y,
          .end_x = *end_x,
          .end_y = *end_y,
          .stops = {
              {.offset = 0.0, .color = *color_a},
              {.offset = 1.0, .color = *color_b},
          },
      }};
    }
  } else if (fill_kind == QStringLiteral("radial_gradient")) {
    const auto color_a =
        parse_color(parameters.value(QStringLiteral("colorA")).toString());
    const auto color_b =
        parse_color(parameters.value(QStringLiteral("colorB")).toString());
    const auto center_x = number(QStringLiteral("centerX"));
    const auto center_y = number(QStringLiteral("centerY"));
    const auto radius = number(QStringLiteral("radius"));
    if (color_a && color_b && center_x && center_y && radius) {
      fill = refusion::core::ShapeFill{refusion::core::RadialGradientFill{
          .center_x = *center_x,
          .center_y = *center_y,
          .radius = *radius,
          .stops = {
              {.offset = 0.0, .color = *color_a},
              {.offset = 1.0, .color = *color_b},
          },
      }};
    }
  }
  if (!fill) {
    diagnostic_ = QStringLiteral(
        "RFX-FILL-INPUT-400: fill requires valid geometry and #RRGGBBAA colors");
    emit diagnosticChanged();
    return;
  }

  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  const auto result = commands_->submit(refusion::core::SetVisualPropertyCommand{
      .envelope = refusion::core::CommandEnvelope{
          .command_id = refusion::core::CommandId{
              "cmd_qt_shape_fill_" + suffix},
          .expected_revision = base.revision_id,
          .idempotency_key = refusion::core::IdempotencyKey{
              "qt-shape-fill-" + suffix},
      },
      .node = *selected_visual_node_,
      .property_id = refusion::core::VisualPropertyId{"shape.fill"},
      .value = refusion::core::VisualPropertyValue{
          std::in_place_type<refusion::core::ShapeFill>, std::move(*fill)},
  });
  publishUiResult(result);
}

void StudioBridge::addSelectedEffect(const QString& effect_kind) {
  const auto base = commands_->active_snapshot();
  const auto* layer = selectedLayer(base);
  if (layer == nullptr) {
    diagnostic_ = QStringLiteral(
        "RFX-EFFECT-SELECTION-002: select a Shape or Text Layer before editing FX");
    emit diagnosticChanged();
    return;
  }
  const auto instance_id =
      "fx_ui_" + std::to_string(base.revision_id.value + 1) + "_" +
      std::to_string(layer->effects.size() + 1);
  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  publishUiResult(refusion::application::submit_visual_contribution(
      *commands_, refusion::application::AddVisualContributionRequest{
                      .envelope = refusion::core::CommandEnvelope{
                          .command_id = refusion::core::CommandId{
                              "cmd_qt_add_contribution_" + suffix},
                          .expected_revision = base.revision_id,
                          .idempotency_key = refusion::core::IdempotencyKey{
                              "qt-add-contribution-" + suffix},
                      },
                      .category =
                          refusion::core::VisualContributionCategory::effect,
                      .layer_id = layer->layer_id,
                      .descriptor_id = effect_kind.toStdString(),
                      .instance_id = instance_id,
                  }));
}

void StudioBridge::updateSelectedEffect(const QString& effect_id,
                                        const bool enabled,
                                        const QVariantMap& parameters) {
  const auto base = commands_->active_snapshot();
  const auto* layer = selectedLayer(base);
  if (layer == nullptr) {
    diagnostic_ = QStringLiteral(
        "RFX-EFFECT-SELECTION-002: select a Shape or Text Layer before editing FX");
    emit diagnosticChanged();
    return;
  }
  const auto found = std::find_if(
      layer->effects.begin(), layer->effects.end(),
      [&effect_id](const auto& effect) {
        return effect.effect_id.value == effect_id.toStdString();
      });
  if (found == layer->effects.end()) {
    diagnostic_ = QStringLiteral(
        "RFX-EFFECT-404: selected effect does not exist");
    emit diagnosticChanged();
    return;
  }
  const auto records =
      refusion::core::inspect_visual_effect_parameters(*found);
  if (records.empty()) {
    diagnostic_ = QStringLiteral(
        "RFX-EFFECT-KIND-400: effect is absent from the contribution registry");
    emit diagnosticChanged();
    return;
  }
  std::vector<refusion::application::VisualContributionParameterAssignment>
      assignments;
  assignments.reserve(records.size());
  for (const auto& record : records) {
    const auto value = parse_parameter_value(record.descriptor, parameters);
    if (!value) {
      diagnostic_ = QStringLiteral(
          "RFX-EFFECT-INPUT-400: parameter does not match its registered type");
      emit diagnosticChanged();
      return;
    }
    assignments.push_back({
        .parameter_id = record.descriptor.id,
        .value = *value,
    });
  }
  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  publishUiResult(refusion::application::submit_visual_contribution(
      *commands_, refusion::application::UpdateVisualContributionRequest{
                      .envelope = refusion::core::CommandEnvelope{
                          .command_id = refusion::core::CommandId{
                              "cmd_qt_update_contribution_" + suffix},
                          .expected_revision = base.revision_id,
                          .idempotency_key = refusion::core::IdempotencyKey{
                              "qt-update-contribution-" + suffix},
                      },
                      .category =
                          refusion::core::VisualContributionCategory::effect,
                      .layer_id = layer->layer_id,
                      .instance_id = effect_id.toStdString(),
                      .enabled = enabled,
                      .parameters = std::move(assignments),
                  }));
}

void StudioBridge::removeSelectedEffect(const QString& effect_id) {
  const auto base = commands_->active_snapshot();
  const auto* layer = selectedLayer(base);
  if (layer == nullptr) {
    diagnostic_ = QStringLiteral(
        "RFX-EFFECT-SELECTION-002: select a Shape or Text Layer before editing FX");
    emit diagnosticChanged();
    return;
  }
  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  publishUiResult(refusion::application::submit_visual_contribution(
      *commands_, refusion::application::RemoveVisualContributionRequest{
                      .envelope = refusion::core::CommandEnvelope{
                          .command_id = refusion::core::CommandId{
                              "cmd_qt_remove_contribution_" + suffix},
                          .expected_revision = base.revision_id,
                          .idempotency_key = refusion::core::IdempotencyKey{
                              "qt-remove-contribution-" + suffix},
                      },
                      .category =
                          refusion::core::VisualContributionCategory::effect,
                      .layer_id = layer->layer_id,
                      .instance_id = effect_id.toStdString(),
                  }));
}

void StudioBridge::addSelectedMask(const QString& mask_kind) {
  const auto base = commands_->active_snapshot();
  const auto* layer = selectedLayer(base);
  if (layer == nullptr) {
    diagnostic_ = QStringLiteral(
        "RFX-MASK-SELECTION-002: select a Shape or Text Layer before editing masks");
    emit diagnosticChanged();
    return;
  }
  const auto instance_id =
      "mask_ui_" + std::to_string(base.revision_id.value + 1) + "_" +
      std::to_string(layer->masks.size() + 1);
  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  publishUiResult(refusion::application::submit_visual_contribution(
      *commands_, refusion::application::AddVisualContributionRequest{
                      .envelope = refusion::core::CommandEnvelope{
                          .command_id = refusion::core::CommandId{
                              "cmd_qt_add_contribution_" + suffix},
                          .expected_revision = base.revision_id,
                          .idempotency_key = refusion::core::IdempotencyKey{
                              "qt-add-contribution-" + suffix},
                      },
                      .category =
                          refusion::core::VisualContributionCategory::mask,
                      .layer_id = layer->layer_id,
                      .descriptor_id = mask_kind.toStdString(),
                      .instance_id = instance_id,
                  }));
}

void StudioBridge::addSelectedRoundedRectMask() {
  addSelectedMask(QStringLiteral("rounded_rect"));
}

void StudioBridge::updateSelectedMask(const QString& mask_id,
                                      const bool enabled,
                                      const bool inverted,
                                      const QVariantMap& geometry) {
  const auto base = commands_->active_snapshot();
  const auto* layer = selectedLayer(base);
  if (layer == nullptr) {
    diagnostic_ = QStringLiteral(
        "RFX-MASK-SELECTION-002: select a Shape or Text Layer before editing masks");
    emit diagnosticChanged();
    return;
  }
  const auto found = std::find_if(
      layer->masks.begin(), layer->masks.end(),
      [&mask_id](const auto& mask) {
        return mask.mask_id.value == mask_id.toStdString();
      });
  if (found == layer->masks.end()) {
    diagnostic_ = QStringLiteral("RFX-MASK-404: selected mask does not exist");
    emit diagnosticChanged();
    return;
  }
  const auto records = refusion::core::inspect_visual_mask_parameters(*found);
  if (records.empty()) {
    diagnostic_ = QStringLiteral(
        "RFX-MASK-KIND-400: mask is absent from the contribution registry");
    emit diagnosticChanged();
    return;
  }
  std::vector<refusion::application::VisualContributionParameterAssignment>
      assignments;
  assignments.reserve(records.size());
  for (const auto& record : records) {
    const auto value = parse_parameter_value(record.descriptor, geometry);
    if (!value) {
      diagnostic_ = QStringLiteral(
          "RFX-MASK-INPUT-400: parameter does not match its registered type");
      emit diagnosticChanged();
      return;
    }
    assignments.push_back({
        .parameter_id = record.descriptor.id,
        .value = *value,
    });
  }
  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  publishUiResult(refusion::application::submit_visual_contribution(
      *commands_, refusion::application::UpdateVisualContributionRequest{
                      .envelope = refusion::core::CommandEnvelope{
                          .command_id = refusion::core::CommandId{
                              "cmd_qt_update_contribution_" + suffix},
                          .expected_revision = base.revision_id,
                          .idempotency_key = refusion::core::IdempotencyKey{
                              "qt-update-contribution-" + suffix},
                      },
                      .category =
                          refusion::core::VisualContributionCategory::mask,
                      .layer_id = layer->layer_id,
                      .instance_id = mask_id.toStdString(),
                      .enabled = enabled,
                      .inverted = inverted,
                      .parameters = std::move(assignments),
                  }));
}

void StudioBridge::removeSelectedMask(const QString& mask_id) {
  const auto base = commands_->active_snapshot();
  const auto* layer = selectedLayer(base);
  if (layer == nullptr) {
    diagnostic_ = QStringLiteral(
        "RFX-MASK-SELECTION-002: select a Shape or Text Layer before editing masks");
    emit diagnosticChanged();
    return;
  }
  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  publishUiResult(refusion::application::submit_visual_contribution(
      *commands_, refusion::application::RemoveVisualContributionRequest{
                      .envelope = refusion::core::CommandEnvelope{
                          .command_id = refusion::core::CommandId{
                              "cmd_qt_remove_contribution_" + suffix},
                          .expected_revision = base.revision_id,
                          .idempotency_key = refusion::core::IdempotencyKey{
                              "qt-remove-contribution-" + suffix},
                      },
                      .category =
                          refusion::core::VisualContributionCategory::mask,
                      .layer_id = layer->layer_id,
                      .instance_id = mask_id.toStdString(),
                  }));
}

void StudioBridge::submitSelectedAlignment(const QString& target_id,
                                           const bool target_is_group,
                                           const QString& horizontal,
                                           const QString& vertical,
                                           const QString& bounds_basis) {
  const auto base = commands_->active_snapshot();
  if (!selected_visual_node_ || selectedTransform(base) == nullptr) {
    diagnostic_ = QStringLiteral(
        "RFX-VISUAL-SELECTION-002: select a Layer or Group before aligning");
    emit diagnosticChanged();
    return;
  }
  const auto typed_horizontal = horizontal_intent(horizontal);
  const auto typed_vertical = vertical_intent(vertical);
  const auto typed_basis = alignment_basis(bounds_basis);
  if (target_id.trimmed().isEmpty() || !typed_horizontal ||
      !typed_vertical || !typed_basis) {
    diagnostic_ = QStringLiteral(
        "RFX-ALIGNMENT-INPUT-400: target, relations and bounds basis are required");
    emit diagnosticChanged();
    return;
  }

  const refusion::core::VisualNodeRef target =
      target_is_group
          ? refusion::core::VisualNodeRef{
                refusion::core::LayerGroupId{target_id.toStdString()}}
          : refusion::core::VisualNodeRef{
                refusion::core::LayerId{target_id.toStdString()}};
  refusion::core::ProjectTimeNs composition_time{0};
  try {
    composition_time = composition_time_provider_();
  } catch (const std::exception& error) {
    diagnostic_ = QStringLiteral("RFX-MEASURE-TIME-SOURCE-001: ") +
                  QString::fromUtf8(error.what());
    emit diagnosticChanged();
    return;
  }
  ++command_sequence_;
  const auto suffix = std::to_string(command_sequence_);
  const auto result = commands_->submit(refusion::core::AlignNodesCommand{
      .envelope = refusion::core::CommandEnvelope{
          .command_id =
              refusion::core::CommandId{"cmd_qt_align_nodes_" + suffix},
          .expected_revision = base.revision_id,
          .idempotency_key =
              refusion::core::IdempotencyKey{"qt-align-nodes-" + suffix},
      },
      .subject = *selected_visual_node_,
      .target = target,
      .composition_time = composition_time,
      .horizontal = *typed_horizontal,
      .vertical = *typed_vertical,
      .bounds_basis = *typed_basis,
  });
  publishUiResult(result);
}

void StudioBridge::publishUiResult(
    const refusion::core::ApplyResult& result) {
  if (result.accepted()) {
    diagnostic_.clear();
    if (accepted_observer_) {
      accepted_observer_(result.active_snapshot);
    }
    refreshMeasurementProjection();
    emit snapshotChanged();
  } else {
    diagnostic_ = QString::fromStdString(result.diagnostic.code + ": " +
                                         result.diagnostic.message);
  }
  emit diagnosticChanged();
}
