#include "adapters/QtRfxProjectFileAdapter.hpp"

#include "refusion/core/ProjectRfx.hpp"

#include <QFile>
#include <QFileInfo>

#include <exception>
#include <string_view>
#include <utility>

namespace {

[[nodiscard]] QString formatted_diagnostic(
    const QString& path,
    const refusion::core::RfxDiagnostic& diagnostic) {
  return QStringLiteral("RFX-PROJECT-OPEN: %1:%2:%3: %4: %5")
      .arg(path)
      .arg(static_cast<qulonglong>(diagnostic.location.line))
      .arg(static_cast<qulonglong>(diagnostic.location.column))
      .arg(QString::fromStdString(diagnostic.code),
           QString::fromStdString(diagnostic.message));
}

[[nodiscard]] ProjectOpenResult rejected(QString diagnostic) {
  return ProjectOpenResult{.diagnostic = std::move(diagnostic)};
}

}  // namespace

ProjectOpenResult open_refusion_project(const QString& path) noexcept {
  try {
    const QFileInfo file_info(path);
    const QString canonical_path = file_info.canonicalFilePath();
    if (canonical_path.isEmpty() || !file_info.isFile()) {
      return rejected(
          QStringLiteral("RFX-PROJECT-OPEN: %1: project file does not exist")
              .arg(path));
    }

    QFile file(canonical_path);
    if (!file.open(QIODevice::ReadOnly)) {
      return rejected(QStringLiteral("RFX-PROJECT-OPEN: %1: %2")
                          .arg(canonical_path, file.errorString()));
    }
    const auto bytes = file.readAll();
    const std::string_view source(bytes.constData(),
                                  static_cast<std::size_t>(bytes.size()));
    auto compiled = refusion::core::compile_project_rfx(source);
    if (!compiled.succeeded()) {
      if (compiled.diagnostics.empty()) {
        return rejected(QStringLiteral("RFX-PROJECT-OPEN: %1: compile failed")
                            .arg(canonical_path));
      }
      return rejected(formatted_diagnostic(canonical_path,
                                           compiled.diagnostics.front()));
    }
    return ProjectOpenResult{
        .project = OpenedProject{
            .snapshot = std::move(*compiled.project),
            .canonical_path = canonical_path,
            .source_bytes = bytes,
        },
    };
  } catch (const std::exception& error) {
    return rejected(QStringLiteral("RFX-PROJECT-OPEN: %1")
                        .arg(QString::fromUtf8(error.what())));
  } catch (...) {
    return rejected(
        QStringLiteral("RFX-PROJECT-OPEN: unknown Project.rfx open failure"));
  }
}
