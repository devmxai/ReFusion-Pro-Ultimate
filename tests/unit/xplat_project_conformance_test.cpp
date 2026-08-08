#include "refusion/core/AgentIntrospection.hpp"
#include "refusion/core/CanonicalText.hpp"
#include "refusion/core/ContentDigest.hpp"
#include "refusion/core/ProjectRfx.hpp"
#include "refusion/core/VisualContributionRegistry.hpp"
#include "refusion/core/VisualPropertyRegistry.hpp"

#include "../support/TestEnvironment.hpp"

#include <cstdlib>
#include <fstream>
#include <locale>
#include <map>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void require(const bool condition) {
  static std::size_t assertion = 0;
  ++assertion;
  if (!condition) {
    throw std::runtime_error("xplat project conformance requirement " +
                             std::to_string(assertion) + " failed");
  }
}

[[nodiscard]] std::string read_file(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  require(input.good());
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

class HostileNumericPunctuation final : public std::numpunct<char> {
 protected:
  [[nodiscard]] char do_decimal_point() const override { return ','; }
  [[nodiscard]] char do_thousands_sep() const override { return '_'; }
  [[nodiscard]] std::string do_grouping() const override { return "\3"; }
};

}  // namespace

int main() {
  using namespace refusion::core;

  const auto receipt = read_receipt(REFUSION_XPLAT_PROJECT_RECEIPT_PATH);
  require(receipt.at("schema") == "refusion.xplat-project-conformance.v2");

  const auto source = read_file(REFUSION_XPLAT_PROJECT_FIXTURE_PATH);
  const auto compiled = compile_project_rfx(source);
  require(compiled.succeeded());
  require(canonical_uint64(compiled.project->revision_id.value) ==
          receipt.at("revision"));
  require(visual_property_registry_digest() == receipt.at("registry_digest"));
  require(project_snapshot_digest(*compiled.project) ==
          receipt.at("snapshot_digest"));

  const auto canonical = serialize_project_rfx(*compiled.project);
  const auto canonical_bytes = std::span<const std::uint8_t>(
      reinterpret_cast<const std::uint8_t*>(canonical.data()),
      canonical.size());
  if (const auto receipt_path = refusion::tests::environment_variable(
          "REFUSION_XPLAT_PROJECT_RECEIPT_OUTPUT");
      receipt_path && !receipt_path->empty()) {
    std::ofstream output(*receipt_path, std::ios::trunc);
    require(static_cast<bool>(output));
    output << "schema=refusion.xplat-project-conformance.v2\n"
           << "snapshot_digest="
           << project_snapshot_digest(*compiled.project) << "\n"
           << "registry_digest=" << visual_property_registry_digest() << "\n"
           << "contribution_registry_digest="
           << visual_contribution_registry_digest() << "\n"
           << "canonical_sha256="
           << sha256_content_digest(canonical_bytes) << "\n"
           << "canonical_size=" << canonical_uint64(canonical.size()) << "\n"
           << "revision="
           << canonical_uint64(compiled.project->revision_id.value) << "\n";
    require(static_cast<bool>(output));
  }
  if (const auto canonical_path = refusion::tests::environment_variable(
          "REFUSION_XPLAT_CANONICAL_PROJECT_OUTPUT");
      canonical_path && !canonical_path->empty()) {
    std::ofstream output(*canonical_path,
                         std::ios::binary | std::ios::trunc);
    require(static_cast<bool>(output));
    output.write(canonical.data(),
                 static_cast<std::streamsize>(canonical.size()));
    require(static_cast<bool>(output));
  }
  require(canonical == read_file(REFUSION_XPLAT_CANONICAL_PROJECT_PATH));
  require(sha256_content_digest(canonical_bytes) ==
          receipt.at("canonical_sha256"));
  require(canonical_uint64(canonical.size()) ==
          receipt.at("canonical_size"));
  require(visual_contribution_registry_digest() ==
          receipt.at("contribution_registry_digest"));
  const auto round_trip = compile_project_rfx(canonical);
  require(round_trip.succeeded());
  require(*round_trip.project == *compiled.project);

  const auto original_locale = std::locale();
  std::locale::global(
      std::locale(original_locale, new HostileNumericPunctuation));
  const auto hostile_canonical = serialize_project_rfx(*compiled.project);
  const auto hostile_digest = project_snapshot_digest(*compiled.project);
  const auto hostile_registry = visual_property_registry_digest();
  std::locale::global(original_locale);

  require(hostile_canonical == canonical);
  require(hostile_digest == receipt.at("snapshot_digest"));
  require(hostile_registry == receipt.at("registry_digest"));

  const auto& text = std::get<TextLayerContent>(
      compiled.project->composition->layers.at(3).content);
  require(validate_preserved_utf8(text.text).valid());
  require(text.text.find("اختبار") != std::string::npos);
}
