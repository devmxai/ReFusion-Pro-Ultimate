#pragma once

#include "refusion/core/ProjectDocument.hpp"
#include "refusion/core/TextLayout.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace refusion::core {

enum class AlignmentBoundsBasis : std::uint8_t {
  geometry,
  logical,
  ink,
};

struct VisualNodeMeasurement final {
  VisualNodeRef node;
  LocalRect geometry_world;
  std::optional<LocalRect> logical_world;
  std::optional<LocalRect> ink_world;
  AffineTransform2D parent_world_transform;
  AffineTransform2D world_transform;

  friend bool operator==(const VisualNodeMeasurement&,
                         const VisualNodeMeasurement&) = default;
};

struct VisualMeasurementSnapshot final {
  ProjectTimeNs composition_time{0};
  std::string layout_engine_digest;
  std::vector<VisualNodeMeasurement> nodes;
};

struct VisualMeasurementDiagnostic final {
  std::string code;
  std::string message;
};

struct VisualMeasurementOutcome final {
  std::optional<VisualMeasurementSnapshot> snapshot;
  std::optional<VisualMeasurementDiagnostic> diagnostic;

  [[nodiscard]] bool succeeded() const noexcept {
    return snapshot.has_value() && !diagnostic.has_value();
  }
};

// Measures one accepted Composition at one exact time. A null layout port is
// valid for geometry-only Shape/TextBox measurement; logical/ink measurement
// of Text nodes remains explicitly unavailable in that outcome.
[[nodiscard]] VisualMeasurementOutcome measure_visual_nodes(
    const CompositionSnapshot& composition,
    ProjectTimeNs composition_time,
    TextLayoutPort* text_layout_port = nullptr);

[[nodiscard]] const VisualNodeMeasurement* find_visual_measurement(
    const VisualMeasurementSnapshot& snapshot,
    const VisualNodeRef& node) noexcept;

[[nodiscard]] std::optional<LocalRect> measurement_bounds(
    const VisualNodeMeasurement& measurement,
    AlignmentBoundsBasis basis) noexcept;

}  // namespace refusion::core
