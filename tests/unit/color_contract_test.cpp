#include "refusion/core/ColorContract.hpp"

#include <stdexcept>
#include <string>

namespace {

void require(const bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

}  // namespace

int main() {
  using namespace refusion::core;
  const auto& contract = desktop_v1_sdr_color_contract();
  require(is_desktop_v1_sdr_color_contract(contract),
          "Desktop v1 color contract must recognize itself");
  require(contract.profile_id == "refusion.color.desktop-v1-sdr.v1",
          "color profile identity changed");
  require(contract.project_alpha == ProjectAlphaModel::straight &&
              contract.compositing_alpha ==
                  CompositingAlphaModel::premultiplied,
          "project/compositing alpha policy changed");
  require(contract.gradient_interpolation ==
              GradientInterpolationPolicy::srgb_straight,
          "gradient interpolation policy changed");
  require(contract.filter_edge == FilterEdgePolicy::transparent_decal,
          "filter edge policy changed");
  require(contract.target_format == VisualTargetFormat::bgra8_unorm &&
              contract.output_transfer == OutputTransferFunction::srgb,
          "target/output policy changed");
  require(!desktop_v1_sdr_color_contract_canonical_bytes().empty(),
          "color contract canonical bytes are missing");
  require(desktop_v1_sdr_color_contract_digest().starts_with("sha256:") &&
              desktop_v1_sdr_color_contract_digest().size() == 71,
          "color contract digest is not canonical SHA-256");
  require(desktop_v1_sdr_color_contract_digest() ==
              "sha256:50a4acc6cc8d7092b5aa10d4f70bc24aa93aaf4e71413617c5ec297e5547af78",
          "color contract canonical bytes changed without a version change");

  auto changed = contract;
  changed.gradient_interpolation =
      static_cast<GradientInterpolationPolicy>(0);
  require(!is_desktop_v1_sdr_color_contract(changed),
          "changed color semantics must fail closed");
}
