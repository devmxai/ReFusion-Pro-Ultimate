#include "adapters/QtJsonProjectFileAdapter.hpp"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace {

struct ProjectOpenFailure final {
  QString diagnostic;
};

[[noreturn]] void invalid(const QString& path, const QString& message) {
  throw ProjectOpenFailure{
      .diagnostic =
          QStringLiteral("RFX-PROJECT-OPEN: %1: %2").arg(path, message),
  };
}

[[nodiscard]] QJsonObject require_object(const QJsonObject& parent,
                                         const QString& key,
                                         const QString& path) {
  const auto value = parent.value(key);
  if (!value.isObject()) {
    invalid(path + QLatin1Char('.') + key, QStringLiteral("object is required"));
  }
  return value.toObject();
}

[[nodiscard]] QJsonArray require_array(const QJsonObject& parent,
                                       const QString& key,
                                       const QString& path) {
  const auto value = parent.value(key);
  if (!value.isArray()) {
    invalid(path + QLatin1Char('.') + key, QStringLiteral("array is required"));
  }
  return value.toArray();
}

[[nodiscard]] QString require_string(const QJsonObject& parent,
                                     const QString& key,
                                     const QString& path) {
  const auto value = parent.value(key);
  if (!value.isString() || value.toString().trimmed().isEmpty()) {
    invalid(path + QLatin1Char('.') + key,
            QStringLiteral("non-empty string is required"));
  }
  return value.toString();
}

[[nodiscard]] double require_number(const QJsonObject& parent,
                                    const QString& key,
                                    const QString& path) {
  const auto value = parent.value(key);
  if (!value.isDouble() || !std::isfinite(value.toDouble())) {
    invalid(path + QLatin1Char('.') + key, QStringLiteral("finite number is required"));
  }
  return value.toDouble();
}

[[nodiscard]] std::uint64_t require_u64(const QJsonObject& parent,
                                        const QString& key,
                                        const QString& path) {
  const double number = require_number(parent, key, path);
  if (number < 0.0 || std::floor(number) != number ||
      number > static_cast<double>(std::numeric_limits<std::uint64_t>::max())) {
    invalid(path + QLatin1Char('.') + key,
            QStringLiteral("unsigned integer is required"));
  }
  return static_cast<std::uint64_t>(number);
}

[[nodiscard]] std::uint32_t require_u32(const QJsonObject& parent,
                                        const QString& key,
                                        const QString& path) {
  const auto number = require_u64(parent, key, path);
  if (number > std::numeric_limits<std::uint32_t>::max()) {
    invalid(path + QLatin1Char('.') + key,
            QStringLiteral("32-bit unsigned integer is required"));
  }
  return static_cast<std::uint32_t>(number);
}

[[nodiscard]] bool require_bool(const QJsonObject& parent,
                                const QString& key,
                                const QString& path) {
  const auto value = parent.value(key);
  if (!value.isBool()) {
    invalid(path + QLatin1Char('.') + key, QStringLiteral("boolean is required"));
  }
  return value.toBool();
}

[[nodiscard]] std::uint8_t hex_byte(const QString& value,
                                    const int offset,
                                    const QString& path) {
  bool ok = false;
  const auto byte = value.mid(offset, 2).toUInt(&ok, 16);
  if (!ok || byte > 255) {
    invalid(path, QStringLiteral("color must be #RRGGBBAA"));
  }
  return static_cast<std::uint8_t>(byte);
}

[[nodiscard]] refusion::core::ColorRgba8 parse_color(const QString& value,
                                                     const QString& path) {
  if (value.size() != 9 || !value.startsWith(QLatin1Char('#'))) {
    invalid(path, QStringLiteral("color must be #RRGGBBAA"));
  }
  return refusion::core::ColorRgba8{
      .red = hex_byte(value, 1, path),
      .green = hex_byte(value, 3, path),
      .blue = hex_byte(value, 5, path),
      .alpha = hex_byte(value, 7, path),
  };
}

[[nodiscard]] refusion::core::AnimatedProperty parse_property(
    const QString& value, const QString& path) {
  using refusion::core::AnimatedProperty;
  if (value == QStringLiteral("transform.position.x")) {
    return AnimatedProperty::position_x;
  }
  if (value == QStringLiteral("transform.position.y")) {
    return AnimatedProperty::position_y;
  }
  if (value == QStringLiteral("transform.scale.x")) {
    return AnimatedProperty::scale_x;
  }
  if (value == QStringLiteral("transform.scale.y")) {
    return AnimatedProperty::scale_y;
  }
  if (value == QStringLiteral("transform.rotation")) {
    return AnimatedProperty::rotation_degrees;
  }
  if (value == QStringLiteral("transform.opacity")) {
    return AnimatedProperty::opacity;
  }
  invalid(path, QStringLiteral("unsupported animated property"));
}

[[nodiscard]] refusion::core::Transform2D parse_transform(
    const QJsonObject& object, const QString& path) {
  return refusion::core::Transform2D{
      .position_x = require_number(object, QStringLiteral("position_x"), path),
      .position_y = require_number(object, QStringLiteral("position_y"), path),
      .scale_x = require_number(object, QStringLiteral("scale_x"), path),
      .scale_y = require_number(object, QStringLiteral("scale_y"), path),
      .rotation_degrees = require_number(object, QStringLiteral("rotation_degrees"), path),
      .opacity = require_number(object, QStringLiteral("opacity"), path),
  };
}

[[nodiscard]] refusion::core::LayerContent parse_content(
    const QJsonObject& object, const QString& path) {
  const auto type = require_string(object, QStringLiteral("type"), path);
  const auto fill = parse_color(
      require_string(object, QStringLiteral("fill"), path),
      path + QStringLiteral(".fill"));
  if (type == QStringLiteral("shape")) {
    return refusion::core::ShapeLayerContent{
        .width = require_number(object, QStringLiteral("width"), path),
        .height = require_number(object, QStringLiteral("height"), path),
        .corner_radius = require_number(object, QStringLiteral("corner_radius"), path),
        .fill = fill,
    };
  }
  if (type == QStringLiteral("text")) {
    return refusion::core::TextLayerContent{
        .text = require_string(object, QStringLiteral("text"), path).toStdString(),
        .font_family =
            require_string(object, QStringLiteral("font_family"), path).toStdString(),
        .font_size = require_number(object, QStringLiteral("font_size"), path),
        .layout_width = require_number(object, QStringLiteral("layout_width"), path),
        .left_to_right = require_bool(object, QStringLiteral("left_to_right"), path),
        .fill = fill,
    };
  }
  invalid(path + QStringLiteral(".type"), QStringLiteral("unsupported layer type"));
}

[[nodiscard]] refusion::core::LayerSnapshot parse_layer(const QJsonObject& object,
                                                        const QString& path) {
  const auto range = require_object(object, QStringLiteral("active_range"), path);
  std::vector<refusion::core::ScalarAnimation> animations;
  const auto animation_array = require_array(object, QStringLiteral("animations"), path);
  animations.reserve(static_cast<std::size_t>(animation_array.size()));
  for (qsizetype animation_index = 0; animation_index < animation_array.size();
       ++animation_index) {
    const QString animation_path =
        path + QStringLiteral(".animations[%1]").arg(animation_index);
    if (!animation_array.at(animation_index).isObject()) {
      invalid(animation_path, QStringLiteral("object is required"));
    }
    const auto animation_object = animation_array.at(animation_index).toObject();
    const auto keyframe_array =
        require_array(animation_object, QStringLiteral("keyframes"), animation_path);
    std::vector<refusion::core::ScalarKeyframe> keyframes;
    keyframes.reserve(static_cast<std::size_t>(keyframe_array.size()));
    for (qsizetype keyframe_index = 0; keyframe_index < keyframe_array.size();
         ++keyframe_index) {
      const QString keyframe_path =
          animation_path + QStringLiteral(".keyframes[%1]").arg(keyframe_index);
      if (!keyframe_array.at(keyframe_index).isObject()) {
        invalid(keyframe_path, QStringLiteral("object is required"));
      }
      const auto keyframe = keyframe_array.at(keyframe_index).toObject();
      keyframes.push_back(refusion::core::ScalarKeyframe{
          .time = require_u64(keyframe, QStringLiteral("time_ns"), keyframe_path),
          .value = require_number(keyframe, QStringLiteral("value"), keyframe_path),
      });
    }
    animations.push_back(refusion::core::ScalarAnimation{
        .property = parse_property(
            require_string(animation_object, QStringLiteral("property"), animation_path),
            animation_path + QStringLiteral(".property")),
        .keyframes = std::move(keyframes),
    });
  }

  return refusion::core::LayerSnapshot{
      .layer_id = refusion::core::LayerId{
          require_string(object, QStringLiteral("id"), path).toStdString()},
      .display_name = require_string(object, QStringLiteral("name"), path).toStdString(),
      .active_range = refusion::core::TimeRangeNs{
          .start = require_u64(range, QStringLiteral("start_ns"), path + ".active_range"),
          .duration =
              require_u64(range, QStringLiteral("duration_ns"), path + ".active_range"),
      },
      .transform = parse_transform(
          require_object(object, QStringLiteral("transform"), path),
          path + QStringLiteral(".transform")),
      .animations = std::move(animations),
      .content = parse_content(
          require_object(object, QStringLiteral("content"), path),
          path + QStringLiteral(".content")),
  };
}

}  // namespace

namespace {

OpenedProject parse_refusion_project(const QString& path) {
  QFileInfo file_info(path);
  const QString canonical_path = file_info.canonicalFilePath();
  if (canonical_path.isEmpty() || !file_info.isFile()) {
    invalid(path, QStringLiteral("project file does not exist"));
  }

  QFile file(canonical_path);
  if (!file.open(QIODevice::ReadOnly)) {
    invalid(canonical_path, file.errorString());
  }
  QJsonParseError parse_error;
  const auto document = QJsonDocument::fromJson(file.readAll(), &parse_error);
  if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
    invalid(canonical_path,
            QStringLiteral("invalid JSON: %1").arg(parse_error.errorString()));
  }

  const auto root = document.object();
  if (require_u32(root, QStringLiteral("schema_version"), QStringLiteral("$")) != 1) {
    invalid(QStringLiteral("$.schema_version"),
            QStringLiteral("unsupported project schema version"));
  }
  const auto project = require_object(root, QStringLiteral("project"), QStringLiteral("$"));
  const auto composition_object =
      require_object(root, QStringLiteral("composition"), QStringLiteral("$"));
  const auto canvas = require_object(
      composition_object, QStringLiteral("canvas"), QStringLiteral("$.composition"));
  const auto frame_rate = require_object(
      composition_object, QStringLiteral("frame_rate"), QStringLiteral("$.composition"));

  std::vector<refusion::core::LayerSnapshot> layers;
  const auto layer_array = require_array(
      composition_object, QStringLiteral("layers"), QStringLiteral("$.composition"));
  layers.reserve(static_cast<std::size_t>(layer_array.size()));
  for (qsizetype index = 0; index < layer_array.size(); ++index) {
    const QString layer_path = QStringLiteral("$.composition.layers[%1]").arg(index);
    if (!layer_array.at(index).isObject()) {
      invalid(layer_path, QStringLiteral("object is required"));
    }
    layers.push_back(parse_layer(layer_array.at(index).toObject(), layer_path));
  }

  refusion::core::CompositionSnapshot composition{
      .composition_id = refusion::core::CompositionId{
          require_string(composition_object, QStringLiteral("id"),
                         QStringLiteral("$.composition")).toStdString()},
      .display_name =
          require_string(composition_object, QStringLiteral("name"),
                         QStringLiteral("$.composition")).toStdString(),
      .canvas = refusion::core::CanvasExtent{
          .width_pixels =
              require_u32(canvas, QStringLiteral("width_pixels"),
                          QStringLiteral("$.composition.canvas")),
          .height_pixels =
              require_u32(canvas, QStringLiteral("height_pixels"),
                          QStringLiteral("$.composition.canvas")),
      },
      .frame_rate = refusion::core::RationalRate{
          .numerator = require_u32(frame_rate, QStringLiteral("numerator"),
                                   QStringLiteral("$.composition.frame_rate")),
          .denominator = require_u32(frame_rate, QStringLiteral("denominator"),
                                     QStringLiteral("$.composition.frame_rate")),
      },
      .duration = require_u64(composition_object, QStringLiteral("duration_ns"),
                              QStringLiteral("$.composition")),
      .layers = std::move(layers),
  };
  const auto validation = refusion::core::validate_composition(composition);
  if (!validation.valid) {
    invalid(canonical_path,
            QString::fromStdString(validation.code + ": " + validation.message));
  }

  return OpenedProject{
      .snapshot = refusion::core::ProjectSnapshot{
          .project_id = refusion::core::ProjectId{
              require_string(project, QStringLiteral("id"),
                             QStringLiteral("$.project")).toStdString()},
          .revision_id = refusion::core::RevisionId{
              require_u64(project, QStringLiteral("revision"),
                          QStringLiteral("$.project"))},
          .display_name =
              require_string(project, QStringLiteral("name"),
                             QStringLiteral("$.project")).toStdString(),
          .composition = std::move(composition),
      },
      .canonical_path = canonical_path,
  };
}

}  // namespace

ProjectOpenResult open_refusion_project(const QString& path) noexcept {
  try {
    return ProjectOpenResult{.project = parse_refusion_project(path)};
  } catch (const ProjectOpenFailure& failure) {
    return ProjectOpenResult{.diagnostic = failure.diagnostic};
  } catch (const std::exception& error) {
    return ProjectOpenResult{.diagnostic = QString::fromUtf8(error.what())};
  } catch (...) {
    return ProjectOpenResult{
        .diagnostic = QStringLiteral("RFX-PROJECT-OPEN: unknown project-open failure")};
  }
}
