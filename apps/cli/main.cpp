#include "AgentJson.hpp"
#include "adapters/AtomicProjectFile.hpp"

#include "refusion/application/ProjectCommandService.hpp"
#include "refusion/core/AgentIntrospection.hpp"
#include "refusion/core/ProjectClock.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/core/VisualPropertyRegistry.hpp"

#if defined(REFUSION_CLI_SKIA_MEASUREMENT)
#include "refusion/adapters/skia/SkiaTextLayout.hpp"
#endif

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace refusion::core;

struct LoadedProject final {
  std::filesystem::path path;
  std::string source;
  ProjectSnapshot project;
};

void print_usage(const char* executable) {
  std::cerr
      << "usage:\n"
      << "  " << executable << " validate <Project.rfx> [--json]\n"
      << "  " << executable << " describe <Project.rfx>\n"
      << "  " << executable << " outline <Project.rfx>\n"
      << "  " << executable << " inspect <Project.rfx> <layer:id|group:id>\n"
      << "  " << executable << " capabilities\n"
      << "  " << executable << " lint <Project.rfx>\n"
      << "  " << executable << " diff <before.rfx> <after.rfx>\n"
      << "  " << executable
      << " measure <Project.rfx> <project-time-ns> [--json]\n"
      << "  " << executable
      << " commit group <Project.rfx> <group-id> <name> <node-ref>...\n"
      << "  " << executable
      << " commit add-glow <Project.rfx> <layer-id> <effect-id> <sigma-px> "
         "<#RRGGBBAA>\n"
      << "  " << executable
      << " commit align <Project.rfx> <subject-ref> <target-ref> <time-ns> "
         "<none|left|center|right> <none|top|center|bottom> "
         "<geometry|logical|ink>\n";
}

[[nodiscard]] std::optional<std::string> read_file(
    const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) return std::nullopt;
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

void print_diagnostics(const std::filesystem::path& path,
                       const RfxCompileResult& result) {
  for (const auto& diagnostic : result.diagnostics) {
    std::cerr << path.string() << ':' << diagnostic.location.line << ':'
              << diagnostic.location.column << ": error " << diagnostic.code
              << ": " << diagnostic.message << '\n';
  }
}

[[nodiscard]] std::optional<LoadedProject> load_project(
    const std::filesystem::path& path, const bool json_errors,
    const std::string_view schema) {
  const auto source = read_file(path);
  if (!source) {
    if (json_errors) {
      refusion::cli::write_error_json(std::cout, schema, "RFX-RFX-IO-001",
                                      "cannot read Project.rfx");
    } else {
      std::cerr << path.string()
                << ": error RFX-RFX-IO-001: cannot read file\n";
    }
    return std::nullopt;
  }
  auto compiled = compile_project_rfx(*source);
  if (!compiled.succeeded()) {
    print_diagnostics(path, compiled);
    if (json_errors) {
      const auto& diagnostic = compiled.diagnostics.front();
      refusion::cli::write_error_json(std::cout, schema, diagnostic.code,
                                      diagnostic.message);
    }
    return std::nullopt;
  }
  return LoadedProject{.path = path,
                       .source = *source,
                       .project = std::move(*compiled.project)};
}

[[nodiscard]] std::string visual_ref(const VisualNodeRef& node) {
  if (const auto* layer = std::get_if<LayerId>(&node)) {
    return "layer:" + layer->value;
  }
  return "group:" + std::get<LayerGroupId>(node).value;
}

[[nodiscard]] std::optional<VisualNodeRef> parse_visual_ref(
    const std::string_view source) {
  constexpr std::string_view layer_prefix = "layer:";
  constexpr std::string_view group_prefix = "group:";
  if (source.starts_with(layer_prefix) && source.size() > layer_prefix.size()) {
    return LayerId{std::string(source.substr(layer_prefix.size()))};
  }
  if (source.starts_with(group_prefix) && source.size() > group_prefix.size()) {
    return LayerGroupId{std::string(source.substr(group_prefix.size()))};
  }
  return std::nullopt;
}

template <typename Value>
[[nodiscard]] std::optional<Value> parse_number(const std::string_view source) {
  Value result{};
  const auto parsed =
      std::from_chars(source.data(), source.data() + source.size(), result);
  if (parsed.ec != std::errc{} || parsed.ptr != source.data() + source.size()) {
    return std::nullopt;
  }
  return result;
}

[[nodiscard]] std::optional<ColorRgba8> parse_color(
    const std::string_view source) {
  if (source.size() != 9 || source.front() != '#') return std::nullopt;
  const auto component = [source](const std::size_t offset)
      -> std::optional<std::uint8_t> {
    unsigned int value = 0;
    const auto begin = source.data() + offset;
    const auto parsed = std::from_chars(begin, begin + 2, value, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != begin + 2 || value > 255U) {
      return std::nullopt;
    }
    return static_cast<std::uint8_t>(value);
  };
  const auto red = component(1);
  const auto green = component(3);
  const auto blue = component(5);
  const auto alpha = component(7);
  if (!red || !green || !blue || !alpha) return std::nullopt;
  return ColorRgba8{.red = *red, .green = *green, .blue = *blue, .alpha = *alpha};
}

[[nodiscard]] std::optional<HorizontalAlignIntent> parse_horizontal(
    const std::string_view value) {
  if (value == "none") return HorizontalAlignIntent::none;
  if (value == "left") return HorizontalAlignIntent::left;
  if (value == "center") return HorizontalAlignIntent::center;
  if (value == "right") return HorizontalAlignIntent::right;
  return std::nullopt;
}

[[nodiscard]] std::optional<VerticalAlignIntent> parse_vertical(
    const std::string_view value) {
  if (value == "none") return VerticalAlignIntent::none;
  if (value == "top") return VerticalAlignIntent::top;
  if (value == "center") return VerticalAlignIntent::center;
  if (value == "bottom") return VerticalAlignIntent::bottom;
  return std::nullopt;
}

[[nodiscard]] std::optional<AlignmentBoundsBasis> parse_basis(
    const std::string_view value) {
  if (value == "geometry") return AlignmentBoundsBasis::geometry;
  if (value == "logical") return AlignmentBoundsBasis::logical;
  if (value == "ink") return AlignmentBoundsBasis::ink;
  return std::nullopt;
}

[[nodiscard]] CommandEnvelope envelope(const ProjectSnapshot& project,
                                       const std::string_view operation) {
  const auto next_revision = project.revision_id.value + 1;
  const auto identity = "agent." + std::string(operation) + ".r" +
                        std::to_string(next_revision);
  return CommandEnvelope{.command_id = CommandId{identity},
                         .expected_revision = project.revision_id,
                         .idempotency_key = IdempotencyKey{identity}};
}

void describe(const ProjectSnapshot& project) {
  const auto outline = agent_project_outline(project);
  const ProjectClockSpec clock{.duration_ns = outline.duration,
                               .frame_rate = outline.frame_rate,
                               .loop = false};
  std::cout << "project id=" << project.project_id.value
            << " revision=" << project.revision_id.value << " name=\""
            << project.display_name << "\" snapshot_digest="
            << outline.snapshot_digest << '\n'
            << "composition id=" << outline.composition_id.value << " canvas="
            << outline.canvas.width_pixels << 'x' << outline.canvas.height_pixels
            << "px frame_rate=" << outline.frame_rate.numerator << '/'
            << outline.frame_rate.denominator << " duration_frames="
            << clock.frame_at_time(outline.duration) << " duration_ns="
            << outline.duration << '\n'
            << "visual_property_registry=" << outline.registry_digest << '\n';
  for (const auto& node : outline.nodes) {
    std::cout << visual_ref(node.node) << " name=\"" << node.display_name
              << "\" timeline_row=" << node.timeline_row << " parent_path=";
    if (node.parent_path.empty()) std::cout << "root";
    for (const auto& parent : node.parent_path) {
      std::cout << "/group:" << parent.value;
    }
    std::cout << " start_frame=" << clock.frame_at_time(node.active_range.start)
              << " end_frame=" << clock.frame_at_time(node.active_range.end())
              << " start_ns=" << node.active_range.start
              << " end_ns=" << node.active_range.end()
              << " effects=" << node.owned_effects.size()
              << " masks=" << node.owned_masks.size()
              << " animations=" << node.animated_properties.size() << '\n';
  }
}

[[nodiscard]] int publish_commit(const LoadedProject& loaded,
                                 const ApplyResult& applied,
                                 const std::string_view operation) {
  if (!applied.accepted()) {
    refusion::cli::write_error_json(std::cout, "refusion.agent.commit.v1",
                                    applied.diagnostic.code,
                                    applied.diagnostic.message);
    return 1;
  }
  const auto canonical = serialize_project_rfx(applied.active_snapshot);
  const auto replaced = refusion::cli::replace_project_file_if_unchanged(
      loaded.path, loaded.source, canonical);
  if (!replaced.replaced) {
    refusion::cli::write_error_json(std::cout, "refusion.agent.commit.v1",
                                    replaced.code, replaced.message);
    return 1;
  }
  const auto diff = agent_project_diff(loaded.project, applied.active_snapshot);
  std::cout << "{\"schema\":\"refusion.agent.commit.v1\",\"ok\":true,"
               "\"status\":\"candidate_published\",\"operation\":\""
            << operation << "\",\"project_id\":\""
            << applied.active_snapshot.project_id.value << "\",\"revision\":"
            << applied.committed_revision.value << ",\"before_digest\":\""
            << diff.before_digest << "\",\"after_digest\":\""
            << diff.after_digest
            << "\",\"runtime_revalidation_required\":true}\n";
  return 0;
}

[[nodiscard]] std::shared_ptr<TextLayoutPort> command_layout_port() {
#if defined(REFUSION_CLI_SKIA_MEASUREMENT)
  return std::shared_ptr<TextLayoutPort>(
      refusion::adapters::skia::create_skia_text_layout_port());
#else
  return nullptr;
#endif
}

[[nodiscard]] int commit_group(const int argc, char** argv) {
  if (argc < 7) return 2;
  auto loaded = load_project(argv[3], true, "refusion.agent.commit.v1");
  if (!loaded) return 1;
  std::vector<VisualNodeRef> nodes;
  nodes.reserve(static_cast<std::size_t>(argc - 6));
  for (int index = 6; index < argc; ++index) {
    const auto node = parse_visual_ref(argv[index]);
    if (!node) {
      refusion::cli::write_error_json(
          std::cout, "refusion.agent.commit.v1", "RFX-AGENT-ARG-001",
          "group nodes must use layer:<id> or group:<id>");
      return 2;
    }
    nodes.push_back(*node);
  }
  auto host = refusion::application::create_application_host(loaded->project);
  const auto result = host->submit(GroupNodesCommand{
      .envelope = envelope(loaded->project, "group"),
      .group_id = LayerGroupId{argv[4]},
      .display_name = argv[5],
      .nodes = std::move(nodes),
  });
  return publish_commit(*loaded, result, "group");
}

[[nodiscard]] int commit_add_glow(const int argc, char** argv) {
  if (argc != 8) return 2;
  auto loaded = load_project(argv[3], true, "refusion.agent.commit.v1");
  if (!loaded) return 1;
  const auto sigma = parse_number<double>(argv[6]);
  const auto glow_color = parse_color(argv[7]);
  if (!sigma || !glow_color) {
    refusion::cli::write_error_json(
        std::cout, "refusion.agent.commit.v1", "RFX-AGENT-ARG-002",
        "sigma must be numeric and color must be #RRGGBBAA");
    return 2;
  }
  auto host = refusion::application::create_application_host(loaded->project);
  const auto result = host->submit(AddEffectCommand{
      .envelope = envelope(loaded->project, "add-glow"),
      .layer_id = LayerId{argv[4]},
      .effect = LayerEffect{
          .effect_id = EffectId{argv[5]},
          .enabled = true,
          .parameters = GlowEffect{.sigma = *sigma, .color = *glow_color},
      },
  });
  return publish_commit(*loaded, result, "add-glow");
}

[[nodiscard]] int commit_align(const int argc, char** argv) {
  if (argc != 10) return 2;
  auto loaded = load_project(argv[3], true, "refusion.agent.commit.v1");
  if (!loaded) return 1;
  const auto subject = parse_visual_ref(argv[4]);
  const auto target = parse_visual_ref(argv[5]);
  const auto time = parse_number<ProjectTimeNs>(argv[6]);
  const auto horizontal = parse_horizontal(argv[7]);
  const auto vertical = parse_vertical(argv[8]);
  const auto basis = parse_basis(argv[9]);
  if (!subject || !target || !time || !horizontal || !vertical || !basis) {
    refusion::cli::write_error_json(
        std::cout, "refusion.agent.commit.v1", "RFX-AGENT-ARG-003",
        "invalid align node, time, relation or bounds basis");
    return 2;
  }
  auto layout = command_layout_port();
  auto host = refusion::application::create_application_host(
      loaded->project, std::move(layout));
  const auto result = host->submit(AlignNodesCommand{
      .envelope = envelope(loaded->project, "align"),
      .subject = *subject,
      .target = *target,
      .composition_time = *time,
      .horizontal = *horizontal,
      .vertical = *vertical,
      .bounds_basis = *basis,
  });
  return publish_commit(*loaded, result, "align");
}

[[nodiscard]] int commit(const int argc, char** argv) {
  if (argc < 3) return 2;
  const std::string_view operation(argv[2]);
  if (operation == "group") return commit_group(argc, argv);
  if (operation == "add-glow") return commit_add_glow(argc, argv);
  if (operation == "align") return commit_align(argc, argv);
  return 2;
}

}  // namespace

int main(const int argc, char** argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 2;
  }
  const std::string_view command(argv[1]);
  if (command == "capabilities" && argc == 2) {
    refusion::cli::write_capabilities_json(std::cout);
    return 0;
  }
  if (command == "commit") {
    const auto result = commit(argc, argv);
    if (result == 2) print_usage(argv[0]);
    return result;
  }
  if (command == "diff") {
    if (argc != 4) {
      print_usage(argv[0]);
      return 2;
    }
    auto before = load_project(argv[2], true, "refusion.agent.diff.v1");
    auto after = load_project(argv[3], true, "refusion.agent.diff.v1");
    if (!before || !after) return 1;
    refusion::cli::write_diff_json(before->project, after->project, std::cout);
    return 0;
  }

  const bool validate = command == "validate";
  const bool json_validate = validate && argc == 4 &&
                             std::string_view(argv[3]) == "--json";
  const bool simple_project_command =
      command == "describe" || command == "outline" || command == "lint";
  const bool inspect = command == "inspect";
  const bool measure = command == "measure";
  const bool valid_arity =
      (validate && (argc == 3 || json_validate)) ||
      (simple_project_command && argc == 3) || (inspect && argc == 4) ||
      (measure && (argc == 4 ||
                   (argc == 5 && std::string_view(argv[4]) == "--json")));
  if (!valid_arity) {
    print_usage(argv[0]);
    return 2;
  }

  const bool json_errors = command != "describe" && !validate ? true
                           : json_validate;
  auto loaded = load_project(argv[2], json_errors,
                             "refusion.agent." + std::string(command) + ".v1");
  if (!loaded) return 1;

  if (validate) {
    if (json_validate) {
      refusion::cli::write_validate_json(loaded->project, std::cout);
    } else {
      const auto& composition = *loaded->project.composition;
      std::cout << "VALID project_id=" << loaded->project.project_id.value
                << " revision=" << loaded->project.revision_id.value
                << " layers=" << composition.layers.size()
                << " groups=" << composition.groups.size() << " roots="
                << composition_root_nodes(composition).size() << '\n';
    }
    return 0;
  }
  if (command == "describe") {
    describe(loaded->project);
    return 0;
  }
  if (command == "outline") {
    refusion::cli::write_outline_json(loaded->project, std::cout);
    return 0;
  }
  if (command == "inspect") {
    const auto node = parse_visual_ref(argv[3]);
    if (!node) {
      refusion::cli::write_error_json(
          std::cout, "refusion.agent.inspect.v1", "RFX-AGENT-ARG-001",
          "node must use layer:<id> or group:<id>");
      return 2;
    }
    std::string error;
    if (!refusion::cli::write_inspect_json(loaded->project, *node, std::cout,
                                           error)) {
      refusion::cli::write_error_json(std::cout, "refusion.agent.inspect.v1",
                                      "RFX-AGENT-NODE-001", error);
      return 1;
    }
    return 0;
  }
  if (command == "lint") {
    refusion::cli::write_lint_json(loaded->project, std::cout);
    return 0;
  }
  if (command == "measure") {
    const auto time = parse_number<ProjectTimeNs>(argv[3]);
    if (!time) {
      refusion::cli::write_error_json(
          std::cout, "refusion.agent.measure.v1", "RFX-MEASURE-TIME-002",
          "invalid integer project time");
      return 2;
    }
    std::unique_ptr<TextLayoutPort> layout;
#if defined(REFUSION_CLI_SKIA_MEASUREMENT)
    layout = refusion::adapters::skia::create_skia_text_layout_port();
#endif
    std::string error;
    if (!refusion::cli::write_measure_json(loaded->project, *time, layout.get(),
                                           std::cout, error)) {
      refusion::cli::write_error_json(std::cout,
                                      "refusion.agent.measure.v1",
                                      "RFX-MEASURE-EVALUATION-001", error);
      return 1;
    }
    return 0;
  }

  print_usage(argv[0]);
  return 2;
}
