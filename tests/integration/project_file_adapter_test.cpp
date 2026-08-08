#include "adapters/QtRfxProjectFileAdapter.hpp"

#include <QFile>
#include <QTemporaryDir>

#include <stdexcept>

namespace {

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error("project file adapter test requirement failed");
  }
}

}  // namespace

int main() {
  const auto open_result =
      open_refusion_project(QString::fromUtf8(REFUSION_TEST_PROJECT_PATH));
  require(open_result.succeeded());
  const auto& opened = *open_result.project;
  require(opened.snapshot.project_id.value == "prj_rfx_authoring_exp_01");
  require(opened.snapshot.revision_id.value == 1);
  require(opened.snapshot.composition.has_value());
  const auto& composition = *opened.snapshot.composition;
  require(composition.composition_id.value == "cmp_reels_main");
  require(composition.canvas.width_pixels == 1080);
  require(composition.canvas.height_pixels == 1920);
  require(composition.duration == 30'000'000'000);
  require(composition.layers.size() == 7);
  require(!opened.canonical_path.isEmpty());
  require(!opened.source_bytes.isEmpty());

  QTemporaryDir temporary_directory;
  require(temporary_directory.isValid());
  const QString invalid_path = temporary_directory.filePath("Project.rfx");
  QFile invalid_file(invalid_path);
  require(invalid_file.open(QIODevice::WriteOnly));
  require(invalid_file.write("rfx 1;\ninvalid") > 0);
  invalid_file.close();

  const auto rejected = open_refusion_project(invalid_path);
  require(!rejected.succeeded());
  require(rejected.diagnostic.startsWith(QStringLiteral("RFX-PROJECT-OPEN:")));
}
