#include "refusion/core/ProjectAuthority.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/core/TextLayout.hpp"
#include "refusion/core/VisualMeasurement.hpp"

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace refusion::core;

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("measured alignment test requirement failed");
  }
}

[[nodiscard]] bool near(const double lhs, const double rhs,
                        const double tolerance = 0.0001) {
  return std::abs(lhs - rhs) <= tolerance;
}

class FixedTextLayout final : public TextLayoutPort {
 public:
  [[nodiscard]] std::string layout_engine_digest() const override {
    return "fixed-alignment-layout-v1";
  }

  [[nodiscard]] TextLayoutOutcome layout(
      const TextLayoutRequest& request) override {
    const LocalRect logical{
        .left = -40.0,
        .top = -10.0,
        .right = 60.0,
        .bottom = 20.0,
    };
    const LocalRect ink{
        .left = -30.0,
        .top = -8.0,
        .right = 70.0,
        .bottom = 18.0,
    };
    return TextLayoutOutcome{
        .result = TextLayoutResult{
            .layout_box = text_box_bounds(request.text.box),
            .content_box = text_box_content_bounds(request.text.box),
            .logical_bounds = logical,
            .ink_bounds = ink,
            .clipped_bounds = ink,
            .lines = {{.utf8_length = request.text.text.size(),
                       .baseline_y = 10.0,
                       .logical_width = 100.0,
                       .ink_bounds = ink}},
            .baselines = {10.0},
            .ascent = 20.0,
            .descent = 10.0,
            .resolved_font_digest = "fixed-font",
            .layout_engine_digest = layout_engine_digest(),
            .cache_key = text_layout_cache_key(request,
                                                layout_engine_digest()),
        },
    };
  }
};

class DriftingTextLayout final : public TextLayoutPort {
 public:
  [[nodiscard]] std::string layout_engine_digest() const override {
    return "drifting-layout-v1";
  }

  [[nodiscard]] TextLayoutOutcome layout(
      const TextLayoutRequest& request) override {
    const double drift = static_cast<double>(calls_++) * 10.0;
    const LocalRect bounds{
        .left = -40.0 + drift,
        .top = -10.0,
        .right = 60.0 + drift,
        .bottom = 20.0,
    };
    return TextLayoutOutcome{
        .result = TextLayoutResult{
            .layout_box = text_box_bounds(request.text.box),
            .content_box = text_box_content_bounds(request.text.box),
            .logical_bounds = bounds,
            .ink_bounds = bounds,
            .clipped_bounds = bounds,
            .resolved_font_digest = "drifting-font",
            .layout_engine_digest = layout_engine_digest(),
            .cache_key = "drifting-key-" + std::to_string(calls_),
        },
    };
  }

 private:
  int calls_{0};
};

[[nodiscard]] LayerSnapshot shape(std::string id, const double x,
                                  const double y, const double width,
                                  const double height) {
  return LayerSnapshot{
      .layer_id = LayerId{std::move(id)},
      .display_name = "Shape",
      .active_range = {.start = 0, .duration = 2'000'000'000},
      .transform = {.position_x = x, .position_y = y},
      .content = ShapeLayerContent{
          .width = width,
          .height = height,
          .fill = ColorRgba8{.red = 255},
      },
  };
}

[[nodiscard]] LayerSnapshot text(const double x, const double y) {
  return LayerSnapshot{
      .layer_id = LayerId{"lyr_text"},
      .display_name = "Text",
      .active_range = {.start = 0, .duration = 2'000'000'000},
      .transform = {.position_x = x, .position_y = y},
      .content = TextLayerContent{
          .text = "Subject",
          .font = FontIdentity{.family_name = "Arial"},
          .font_size = 40.0,
          .box = {.width = 200.0, .height = 80.0},
          .fill = ColorRgba8{.red = 255, .green = 255, .blue = 255},
      },
  };
}

[[nodiscard]] ProjectSnapshot project(
    CompositionSnapshot composition, const std::uint64_t revision = 1) {
  return ProjectSnapshot{
      .project_id = ProjectId{"prj_alignment"},
      .revision_id = RevisionId{revision},
      .display_name = "Alignment",
      .composition = std::move(composition),
  };
}

[[nodiscard]] CompositionSnapshot composition(
    std::vector<LayerSnapshot> layers,
    std::vector<LayerGroupSnapshot> groups = {},
    std::vector<VisualNodeRef> roots = {}) {
  if (roots.empty()) {
    for (const auto& layer : layers) {
      roots.emplace_back(layer.layer_id);
    }
  }
  return CompositionSnapshot{
      .composition_id = CompositionId{"cmp_alignment"},
      .display_name = "Alignment",
      .canvas = {.width_pixels = 1920, .height_pixels = 1080},
      .frame_rate = {.numerator = 30, .denominator = 1},
      .duration = 2'000'000'000,
      .layers = std::move(layers),
      .groups = std::move(groups),
      .root_nodes = std::move(roots),
  };
}

[[nodiscard]] CommandEnvelope envelope(std::string id,
                                       const std::uint64_t revision) {
  return CommandEnvelope{
      .command_id = CommandId{id},
      .expected_revision = RevisionId{revision},
      .idempotency_key = IdempotencyKey{"idem_" + id},
  };
}

[[nodiscard]] double center_x(const LocalRect& bounds) {
  return (bounds.left + bounds.right) * 0.5;
}

[[nodiscard]] double center_y(const LocalRect& bounds) {
  return (bounds.top + bounds.bottom) * 0.5;
}

void shape_alignment_is_atomic_and_replayable() {
  auto snapshot = project(composition({
      shape("lyr_subject", 100.0, 100.0, 50.0, 50.0),
      shape("lyr_target", 400.0, 300.0, 200.0, 100.0),
  }));
  ProjectAuthority authority(snapshot);
  const AlignNodesCommand command{
      .envelope = envelope("align_shapes", 1),
      .subject = LayerId{"lyr_subject"},
      .target = LayerId{"lyr_target"},
      .composition_time = 0,
      .horizontal = HorizontalAlignIntent::center,
      .vertical = VerticalAlignIntent::center,
      .bounds_basis = AlignmentBoundsBasis::geometry,
  };
  const auto result = authority.apply(command);
  require(result.accepted());
  require(result.committed_revision == RevisionId{2});
  const auto* aligned = find_layer(*result.active_snapshot.composition,
                                   LayerId{"lyr_subject"});
  require(aligned != nullptr);
  require(near(aligned->transform.position_x, 400.0));
  require(near(aligned->transform.position_y, 300.0));
  require(result.active_snapshot.composition->layers.size() == 2);
  require(result.active_snapshot.composition->groups.empty());

  const auto persisted = serialize_project_rfx(result.active_snapshot);
  const auto recompiled = compile_project_rfx(persisted);
  require(recompiled.succeeded());
  require(*recompiled.project == result.active_snapshot);

  const auto replay = authority.apply(command);
  require(replay.replayed());
  require(replay.committed_revision == RevisionId{2});
}

void nested_rotated_parent_uses_inverse_parent_transform() {
  auto subject = shape("lyr_subject", 25.0, 40.0, 80.0, 60.0);
  auto target = shape("lyr_target", 700.0, 500.0, 100.0, 100.0);
  LayerGroupSnapshot parent{
      .group_id = LayerGroupId{"grp_parent"},
      .display_name = "Rotated Parent",
      .active_range = {.start = 0, .duration = 2'000'000'000},
      .transform = {
          .position_x = 250.0,
          .position_y = 175.0,
          .scale_x = 1.75,
          .scale_y = 0.8,
          .rotation_degrees = 63.0,
      },
      .children = {LayerId{"lyr_subject"}},
  };
  ProjectAuthority authority(project(composition(
      {std::move(subject), std::move(target)}, {parent},
      {LayerGroupId{"grp_parent"}, LayerId{"lyr_target"}})));
  const auto result = authority.apply(AlignNodesCommand{
      .envelope = envelope("align_nested_rotated", 1),
      .subject = LayerId{"lyr_subject"},
      .target = LayerId{"lyr_target"},
      .composition_time = 0,
      .horizontal = HorizontalAlignIntent::center,
      .vertical = VerticalAlignIntent::center,
  });
  require(result.accepted());
  const auto measured = measure_visual_nodes(
      *result.active_snapshot.composition, 0);
  require(measured.succeeded());
  const auto* aligned = find_visual_measurement(
      *measured.snapshot, LayerId{"lyr_subject"});
  const auto* target_measure = find_visual_measurement(
      *measured.snapshot, LayerId{"lyr_target"});
  require(aligned != nullptr && target_measure != nullptr);
  require(near(center_x(aligned->geometry_world),
               center_x(target_measure->geometry_world), 0.25));
  require(near(center_y(aligned->geometry_world),
               center_y(target_measure->geometry_world), 0.25));

  const auto* aligned_layer = find_layer(
      *result.active_snapshot.composition, LayerId{"lyr_subject"});
  require(aligned_layer != nullptr);
  require(!near(aligned_layer->transform.position_x, 700.0));
  require(!near(aligned_layer->transform.position_y, 500.0));
}

void text_bases_are_measured_not_guessed() {
  const auto make_project = [] {
    return project(composition({
        text(50.0, 60.0),
        shape("lyr_target", 500.0, 400.0, 200.0, 100.0),
    }));
  };
  auto layout = std::make_shared<FixedTextLayout>();
  ProjectAuthority logical_authority(make_project(), layout);
  const auto logical = logical_authority.apply(AlignNodesCommand{
      .envelope = envelope("align_text_logical", 1),
      .subject = LayerId{"lyr_text"},
      .target = LayerId{"lyr_target"},
      .composition_time = 0,
      .horizontal = HorizontalAlignIntent::center,
      .vertical = VerticalAlignIntent::center,
      .bounds_basis = AlignmentBoundsBasis::logical,
  });
  require(logical.accepted());
  const auto* logical_text = find_layer(*logical.active_snapshot.composition,
                                        LayerId{"lyr_text"});
  require(near(logical_text->transform.position_x, 490.0));
  require(near(logical_text->transform.position_y, 395.0));

  ProjectAuthority ink_authority(make_project(), layout);
  const auto ink = ink_authority.apply(AlignNodesCommand{
      .envelope = envelope("align_text_ink", 1),
      .subject = LayerId{"lyr_text"},
      .target = LayerId{"lyr_target"},
      .composition_time = 0,
      .horizontal = HorizontalAlignIntent::center,
      .vertical = VerticalAlignIntent::center,
      .bounds_basis = AlignmentBoundsBasis::ink,
  });
  require(ink.accepted());
  const auto* ink_text = find_layer(*ink.active_snapshot.composition,
                                    LayerId{"lyr_text"});
  require(near(ink_text->transform.position_x, 480.0));
  require(near(ink_text->transform.position_y, 395.0));
}

void rotated_parent_and_group_bounds_align_in_world_space() {
  auto child = shape("lyr_child", 40.0, 20.0, 80.0, 40.0);
  auto companion = shape("lyr_companion", 140.0, 20.0, 40.0, 40.0);
  auto target = shape("lyr_target", 700.0, 500.0, 160.0, 120.0);
  LayerGroupSnapshot group{
      .group_id = LayerGroupId{"grp_subject"},
      .display_name = "Subject Group",
      .active_range = {.start = 0, .duration = 2'000'000'000},
      .transform = {
          .position_x = 300.0,
          .position_y = 200.0,
          .scale_x = 2.0,
          .scale_y = 1.5,
          .rotation_degrees = 35.0,
      },
      .children = {LayerId{"lyr_child"}, LayerId{"lyr_companion"}},
  };
  auto source = composition(
      {std::move(child), std::move(companion), std::move(target)},
      {group}, {LayerGroupId{"grp_subject"}, LayerId{"lyr_target"}});
  ProjectAuthority authority(project(source));
  const auto result = authority.apply(AlignNodesCommand{
      .envelope = envelope("align_rotated_group", 1),
      .subject = LayerGroupId{"grp_subject"},
      .target = LayerId{"lyr_target"},
      .composition_time = 0,
      .horizontal = HorizontalAlignIntent::right,
      .vertical = VerticalAlignIntent::bottom,
      .bounds_basis = AlignmentBoundsBasis::geometry,
  });
  require(result.accepted());
  const auto measured = measure_visual_nodes(
      *result.active_snapshot.composition, 0);
  require(measured.succeeded());
  const auto* group_measure = find_visual_measurement(
      *measured.snapshot, LayerGroupId{"grp_subject"});
  const auto* target_measure = find_visual_measurement(
      *measured.snapshot, LayerId{"lyr_target"});
  require(group_measure != nullptr && target_measure != nullptr);
  require(near(group_measure->geometry_world.right,
               target_measure->geometry_world.right, 0.25));
  require(near(group_measure->geometry_world.bottom,
               target_measure->geometry_world.bottom, 0.25));
}

void animated_position_curve_is_translated_as_one_shot_edit() {
  auto animated = shape("lyr_subject", 0.0, 100.0, 50.0, 50.0);
  animated.animations = {
      ScalarAnimation{
          .property = AnimatedProperty::position_x,
          .keyframes = {{.time = 0, .value = 100.0},
                        {.time = 1'000'000'000, .value = 200.0},
                        {.time = 2'000'000'000, .value = 300.0}},
      },
  };
  ProjectAuthority authority(project(composition({
      std::move(animated),
      shape("lyr_target", 500.0, 100.0, 100.0, 100.0),
  })));
  const auto result = authority.apply(AlignNodesCommand{
      .envelope = envelope("align_animated", 1),
      .subject = LayerId{"lyr_subject"},
      .target = LayerId{"lyr_target"},
      .composition_time = 500'000'000,
      .horizontal = HorizontalAlignIntent::center,
  });
  require(result.accepted());
  const auto* aligned = find_layer(*result.active_snapshot.composition,
                                   LayerId{"lyr_subject"});
  require(aligned != nullptr);
  const auto& keys = aligned->animations.front().keyframes;
  require(near(keys.at(0).value, 450.0));
  require(near(keys.at(1).value, 550.0));
  require(near(keys.at(2).value, 650.0));
  require(near(aligned->transform.position_y, 100.0));
}

void animated_alignment_is_exact_at_multiple_requested_times() {
  for (const auto requested_time :
       std::vector<ProjectTimeNs>{0, 500'000'000, 1'500'000'000}) {
    auto animated = shape("lyr_subject", 0.0, 100.0, 50.0, 50.0);
    animated.animations = {
        ScalarAnimation{
            .property = AnimatedProperty::position_x,
            .keyframes = {{.time = 0, .value = 100.0},
                          {.time = 1'000'000'000, .value = 200.0},
                          {.time = 2'000'000'000, .value = 300.0}},
        },
    };
    ProjectAuthority authority(project(composition({
        std::move(animated),
        shape("lyr_target", 500.0, 100.0, 100.0, 100.0),
    })));
    const auto result = authority.apply(AlignNodesCommand{
        .envelope = envelope("align_at_" + std::to_string(requested_time), 1),
        .subject = LayerId{"lyr_subject"},
        .target = LayerId{"lyr_target"},
        .composition_time = requested_time,
        .horizontal = HorizontalAlignIntent::center,
    });
    require(result.accepted());
    const auto measured = measure_visual_nodes(
        *result.active_snapshot.composition, requested_time);
    require(measured.succeeded());
    const auto* subject = find_visual_measurement(
        *measured.snapshot, LayerId{"lyr_subject"});
    const auto* target = find_visual_measurement(
        *measured.snapshot, LayerId{"lyr_target"});
    require(subject != nullptr && target != nullptr);
    require(near(center_x(subject->geometry_world),
                 center_x(target->geometry_world), 0.25));
  }
}

void rejection_paths_retain_last_known_good() {
  auto base = project(composition({
      text(50.0, 60.0),
      shape("lyr_target", 500.0, 400.0, 200.0, 100.0),
  }));
  ProjectAuthority no_port(base);
  const auto before = no_port.active_snapshot();
  const auto unavailable = no_port.apply(AlignNodesCommand{
      .envelope = envelope("align_without_port", 1),
      .subject = LayerId{"lyr_text"},
      .target = LayerId{"lyr_target"},
      .composition_time = 0,
      .horizontal = HorizontalAlignIntent::center,
      .bounds_basis = AlignmentBoundsBasis::logical,
  });
  require(!unavailable.accepted());
  require(unavailable.diagnostic.code == "RFX-MEASURE-PORT-001");
  require(no_port.active_snapshot() == before);

  auto drifting = std::make_shared<DriftingTextLayout>();
  ProjectAuthority drifting_authority(base, drifting);
  const auto drift_before = drifting_authority.active_snapshot();
  const auto drifted = drifting_authority.apply(AlignNodesCommand{
      .envelope = envelope("align_drifting", 1),
      .subject = LayerId{"lyr_text"},
      .target = LayerId{"lyr_target"},
      .composition_time = 0,
      .horizontal = HorizontalAlignIntent::center,
      .bounds_basis = AlignmentBoundsBasis::logical,
  });
  require(!drifted.accepted());
  require(drifted.diagnostic.code == "RFX-MEASURE-POSTCONDITION-001");
  require(drifting_authority.active_snapshot() == drift_before);

  auto group_source = composition(
      {shape("lyr_child", 100.0, 100.0, 50.0, 50.0),
       shape("lyr_target", 500.0, 400.0, 100.0, 100.0)},
      {LayerGroupSnapshot{
          .group_id = LayerGroupId{"grp_parent"},
          .display_name = "Parent",
          .active_range = {.start = 0, .duration = 2'000'000'000},
          .children = {LayerId{"lyr_child"}},
      }},
      {LayerGroupId{"grp_parent"}, LayerId{"lyr_target"}});
  ProjectAuthority ancestor(project(std::move(group_source)));
  const auto ancestor_before = ancestor.active_snapshot();
  const auto related = ancestor.apply(AlignNodesCommand{
      .envelope = envelope("align_ancestor", 1),
      .subject = LayerGroupId{"grp_parent"},
      .target = LayerId{"lyr_child"},
      .composition_time = 0,
      .horizontal = HorizontalAlignIntent::left,
  });
  require(!related.accepted());
  require(related.diagnostic.code == "RFX-INTENT-ALIGN-002");
  require(ancestor.active_snapshot() == ancestor_before);

  ProjectAuthority time_authority(project(composition({
      shape("lyr_subject", 100.0, 100.0, 50.0, 50.0),
      shape("lyr_target", 400.0, 300.0, 100.0, 100.0),
  })));
  const auto invalid_time = time_authority.apply(AlignNodesCommand{
      .envelope = envelope("align_time", 1),
      .subject = LayerId{"lyr_subject"},
      .target = LayerId{"lyr_target"},
      .composition_time = 2'000'000'000,
      .horizontal = HorizontalAlignIntent::center,
  });
  require(!invalid_time.accepted());
  require(invalid_time.diagnostic.code == "RFX-MEASURE-TIME-001");
}

}  // namespace

int main() {
  shape_alignment_is_atomic_and_replayable();
  text_bases_are_measured_not_guessed();
  rotated_parent_and_group_bounds_align_in_world_space();
  nested_rotated_parent_uses_inverse_parent_transform();
  animated_position_curve_is_translated_as_one_shot_edit();
  animated_alignment_is_exact_at_multiple_requested_times();
  rejection_paths_retain_last_known_good();
}
