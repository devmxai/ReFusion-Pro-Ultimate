#include "refusion/core/AgentIntrospection.hpp"
#include "refusion/core/CanonicalCoordinates.hpp"
#include "refusion/core/CanonicalText.hpp"
#include "refusion/core/ProjectAuthority.hpp"
#include "refusion/core/ProjectClock.hpp"
#include "refusion/core/ProjectRfx.hpp"

#include <fstream>
#include <iterator>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void require(const bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] std::string read_file(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  require(input.good(), "cannot read xplat command fixture: " + path);
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

[[nodiscard]] std::map<std::string, std::string> read_receipt(
    const std::string& path) {
  std::istringstream input(read_file(path));
  std::map<std::string, std::string> receipt;
  std::string line;
  while (std::getline(input, line)) {
    const auto separator = line.find('=');
    if (separator != std::string::npos) {
      receipt.emplace(line.substr(0, separator), line.substr(separator + 1));
    }
  }
  return receipt;
}

void expect(const std::map<std::string, std::string>& receipt,
            const std::string& key,
            const std::string& actual) {
  const auto expected = receipt.at(key);
  require(expected == actual,
          key + " mismatch; expected=" + expected + " actual=" + actual);
}

[[nodiscard]] refusion::core::CommandEnvelope envelope(
    const std::string& suffix,
    const refusion::core::RevisionId revision) {
  return {
      .command_id = refusion::core::CommandId{"cmd_xplat_" + suffix},
      .expected_revision = revision,
      .idempotency_key =
          refusion::core::IdempotencyKey{"idem_xplat_" + suffix},
  };
}

class HostileNumericPunctuation final : public std::numpunct<char> {
 protected:
  [[nodiscard]] char do_decimal_point() const override { return ','; }
  [[nodiscard]] char do_thousands_sep() const override { return '_'; }
  [[nodiscard]] std::string do_grouping() const override { return "\3"; }
};

}  // namespace

int main() {
  using namespace refusion::core;

  const auto receipt = read_receipt(REFUSION_XPLAT_COMMAND_RECEIPT_PATH);
  expect(receipt, "schema", "refusion.xplat-command-conformance.v1");

  const auto compiled = compile_project_rfx(
      read_file(REFUSION_XPLAT_COMMAND_FIXTURE_PATH));
  require(compiled.succeeded(), "xplat command Project.rfx did not compile");
  const auto original_locale = std::locale();
  std::locale::global(
      std::locale(original_locale, new HostileNumericPunctuation));
  ProjectAuthority authority(*compiled.project);

  auto active = authority.active_snapshot();
  const auto transform = authority.apply(SetVisualTransformCommand{
      .envelope = envelope("transform", active.revision_id),
      .node = LayerId{"lyr_solid_normal"},
      .transform = Transform2D{
          .position_x = 320.123456,
          .position_y = 180.654321,
          .anchor_x = 0.333333,
          .anchor_y = -0.666667,
          .scale_x = 1.0,
          .scale_y = 1.0,
          .rotation_degrees = 0.0,
          .opacity = 1.0,
      },
  });
  require(transform.accepted(), "canonical transform command was rejected");

  active = authority.active_snapshot();
  const auto property = authority.apply(SetVisualPropertyCommand{
      .envelope = envelope("property", active.revision_id),
      .node = LayerId{"lyr_solid_normal"},
      .property_id = VisualPropertyId{"shape.width"},
      .value = 639.987654,
  });
  require(property.accepted(), "canonical property command was rejected");

  active = authority.active_snapshot();
  const ProjectClockSpec clock{
      .duration_ns = active.composition->duration,
      .frame_rate = active.composition->frame_rate,
      .loop = false,
  };
  const auto aligned = authority.apply(AlignNodesCommand{
      .envelope = envelope("align", active.revision_id),
      .subject = LayerId{"lyr_radial_screen"},
      .target = LayerId{"lyr_solid_normal"},
      .composition_time = clock.time_at_frame(60),
      .horizontal = HorizontalAlignIntent::left,
      .vertical = VerticalAlignIntent::top,
      .bounds_basis = AlignmentBoundsBasis::geometry,
  });
  require(aligned.accepted(), "canonical AlignNodes command was rejected");

  const auto final = authority.active_snapshot();
  const auto* solid = find_layer(*final.composition,
                                 LayerId{"lyr_solid_normal"});
  const auto* radial = find_layer(*final.composition,
                                  LayerId{"lyr_radial_screen"});
  require(solid != nullptr && radial != nullptr,
          "command output lost fixture Layers");
  const auto& solid_shape = std::get<ShapeLayerContent>(solid->content);
  require(is_quantized_authored_pixel(solid->transform.position_x) &&
              is_quantized_authored_pixel(solid->transform.position_y) &&
              is_quantized_authored_pixel(solid->transform.anchor_x) &&
              is_quantized_authored_pixel(solid->transform.anchor_y) &&
              is_quantized_authored_pixel(solid_shape.width) &&
              is_quantized_authored_pixel(radial->transform.position_x) &&
              is_quantized_authored_pixel(radial->transform.position_y),
          "accepted command output escaped the authored subpixel grid");

  const auto canonical = serialize_project_rfx(final);
  expect(receipt, "final_revision",
         canonical_uint64(final.revision_id.value));
  expect(receipt, "snapshot_digest", project_snapshot_digest(final));
  expect(receipt, "canonical_size",
         canonical_uint64(canonical.size()));
  expect(receipt, "solid_position_x",
         canonical_float64(solid->transform.position_x));
  expect(receipt, "solid_position_y",
         canonical_float64(solid->transform.position_y));
  expect(receipt, "solid_anchor_x",
         canonical_float64(solid->transform.anchor_x));
  expect(receipt, "solid_anchor_y",
         canonical_float64(solid->transform.anchor_y));
  expect(receipt, "solid_width", canonical_float64(solid_shape.width));
  expect(receipt, "radial_position_x",
         canonical_float64(radial->transform.position_x));
  expect(receipt, "radial_position_y",
         canonical_float64(radial->transform.position_y));

  const auto reopened = compile_project_rfx(canonical);
  require(reopened.succeeded() && *reopened.project == final,
          "canonical command output did not reopen exactly");
  std::locale::global(original_locale);
}
