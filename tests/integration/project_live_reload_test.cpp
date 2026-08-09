#include "ProjectLiveReloadController.hpp"
#include "StudioBridge.hpp"
#include "StudioRuntimeComposition.hpp"

#include "refusion/application/ProjectCommandService.hpp"
#include "refusion/core/ProjectRfx.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTimer>

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

constexpr std::string_view kInitialSource = R"RFX(rfx 1;
project id("prj_live_test") revision(1) name("Before");
composition id("cmp_live") name("Live") {
  canvas px(1080, 1920);
  frame_rate rational(30, 1);
  duration frames(900);
  layer shape id("lyr_card") name("Card") {
    range frames(0, 900);
    transform {
      position canvas_px(540, 960);
      scale ratio(1, 1);
      rotation degrees(0);
      opacity ratio(1);
    }
    content shape {
      size px(800, 500);
      corner_radius px(40);
      fill rgba8("#7C5CFFFF");
    }
  }
}
)RFX";

const char* current_phase = "setup";

void require(const bool condition) {
  if (!condition) {
    throw std::runtime_error(std::string("project live reload failed during ") +
                             current_phase);
  }
}

void write_atomically(const QString& path, const QByteArray& source) {
  QSaveFile file(path);
  require(file.open(QIODevice::WriteOnly));
  require(file.write(source) == source.size());
  require(file.commit());
}

void wait_for_signal(QObject* object, const char* signal) {
  QEventLoop loop;
  QTimer timeout;
  timeout.setSingleShot(true);
  QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
  QObject::connect(object, signal, &loop, SLOT(quit()));
  timeout.start(3000);
  loop.exec();
  if (!timeout.isActive()) {
    throw std::runtime_error(std::string("timed out waiting for ") + signal);
  }
}

class FakeRuntime final : public StudioRuntimeComposition {
 public:
  explicit FakeRuntime(refusion::core::ProjectSnapshot project)
      : active(std::move(project)) {}

  [[nodiscard]] QWindow* viewport_window() noexcept override { return nullptr; }
  [[nodiscard]] StudioTransportBridge* transport_bridge() noexcept override {
    return nullptr;
  }
  [[nodiscard]] refusion::application::CandidatePreparationResult prepare(
      const refusion::core::ProjectSnapshot& project) override {
    if (!project.composition || !active.composition ||
        project.composition->composition_id != active.composition->composition_id ||
        project.composition->canvas != active.composition->canvas ||
        project.composition->frame_rate != active.composition->frame_rate ||
        project.composition->duration != active.composition->duration) {
      return {.diagnostic = {.code = "TEST-RUNTIME-REJECTED",
                             .message = "incompatible candidate",
                             .blocking = true}};
    }
    class Prepared final
        : public refusion::application::PreparedProjectRevision {
     public:
      Prepared(FakeRuntime& owner, refusion::core::ProjectSnapshot project)
          : owner_(owner), project_(std::move(project)) {}
      void commit_engine_state() noexcept override {
        owner_.active = std::move(project_);
        ++owner_.accept_count;
      }

     private:
      FakeRuntime& owner_;
      refusion::core::ProjectSnapshot project_;
    };
    return {.prepared = std::make_unique<Prepared>(*this, project)};
  }

  refusion::core::ProjectSnapshot active;
  int accept_count{0};
};

}  // namespace

int run_test(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  QTemporaryDir directory;
  require(directory.isValid());
  const auto path = directory.filePath(QStringLiteral("Project.rfx"));
  const QByteArray initial(kInitialSource.data(),
                           static_cast<qsizetype>(kInitialSource.size()));
  write_atomically(path, initial);

  const auto compiled = refusion::core::compile_project_rfx(kInitialSource);
  require(compiled.succeeded());
  auto commands =
      refusion::application::create_application_host(*compiled.project);
  StudioBridge bridge(*commands);
  auto runtime = std::make_shared<FakeRuntime>(*compiled.project);
  commands->set_candidate_admission_port(runtime);
  ProjectLiveReloadController controller(
      *commands,
      bridge,
      path,
      QStringLiteral("/test/refusion-cli"),
      initial);
  current_phase = "initial context verification";
  QFile agent_context(
      directory.filePath(QStringLiteral(".refusion/agent-context.json")));
  require(agent_context.open(QIODevice::ReadOnly));
  const auto initial_context = agent_context.readAll();
  require(initial_context.contains("/test/refusion-cli"));
  const auto initial_context_object =
      QJsonDocument::fromJson(initial_context).object();
  require(initial_context_object.value(QStringLiteral("schema_version"))
              .toInt() == 2);
  require(initial_context_object.value(QStringLiteral("active_revision"))
              .toString() == QStringLiteral("1"));
  agent_context.close();

  current_phase = "external revision acceptance";
  auto accepted_source = initial;
  accepted_source.replace("revision(1)", "revision(2)");
  accepted_source.replace("name(\"Before\")", "name(\"Agent revision\")");
  write_atomically(path, accepted_source);
  wait_for_signal(&bridge, SIGNAL(snapshotChanged()));
  require(commands->active_snapshot().revision_id.value == 2);
  require(commands->active_snapshot().display_name == "Agent revision");
  require(runtime->active.revision_id.value == 2);
  require(runtime->accept_count == 1);
  require(QFileInfo::exists(
      directory.filePath(QStringLiteral(".refusion/Journal/accepted-r2.rfx"))));

  current_phase = "invalid revision rejection";
  auto invalid_source = accepted_source;
  invalid_source.replace("revision(2)", "revision(3)");
  invalid_source.replace("canvas px", "canvas pixels");
  write_atomically(path, invalid_source);
  wait_for_signal(&bridge, SIGNAL(diagnosticChanged()));
  require(bridge.diagnostic().contains(QStringLiteral("RFX-RFX-PARSE-001")));
  require(commands->active_snapshot().revision_id.value == 2);
  require(runtime->active.revision_id.value == 2);
  require(runtime->accept_count == 1);
  require(!QFileInfo::exists(
      directory.filePath(QStringLiteral(".refusion/Journal/accepted-r3.rfx"))));

  current_phase = "transform persistence";
  bridge.selectVisualNode(QStringLiteral("lyr_card"), false);
  current_phase = "transform selection";
  require(bridge.hasVisualSelection());
  bridge.submitSelectedTransform(600.0, 900.0, 20.0, 30.0, 1.1, 0.9,
                                 12.0, 0.8);
  current_phase = "transform active revision";
  require(commands->active_snapshot().revision_id.value == 3);
  current_phase = "transform runtime revision";
  require(runtime->active.revision_id.value == 3);
  current_phase = "transform runtime value";
  require(runtime->active.composition->layers.front().transform.position_x ==
          600.0);
  current_phase = "transform admission count";
  require(runtime->accept_count == 2);
  current_phase = "transform journal";
  require(QFileInfo::exists(
      directory.filePath(QStringLiteral(".refusion/Journal/accepted-r3.rfx"))));

  QFile persisted_project(path);
  require(persisted_project.open(QIODevice::ReadOnly));
  const auto persisted_source = persisted_project.readAll();
  require(persisted_source.contains("rfx 5;"));
  require(persisted_source.contains("revision(3)"));
  require(persisted_source.contains("position parent_px(600, 900)"));
  persisted_project.close();

  current_phase = "fill persistence";
  bridge.submitSelectedShapeFill(
      QStringLiteral("solid"),
      QVariantMap{{QStringLiteral("color"), QStringLiteral("#102030FF")}});
  current_phase = "fill active revision";
  require(commands->active_snapshot().revision_id.value == 4);
  current_phase = "fill runtime revision";
  require(runtime->active.revision_id.value == 4);
  current_phase = "fill runtime value";
  require(std::get<refusion::core::ColorRgba8>(
              std::get<refusion::core::ShapeLayerContent>(
                  runtime->active.composition->layers.front().content)
                  .fill) == refusion::core::ColorRgba8{
                           .red = 16,
                           .green = 32,
                           .blue = 48,
                           .alpha = 255,
                       });
  current_phase = "fill admission count";
  require(runtime->accept_count == 3);
  current_phase = "fill journal";
  require(QFileInfo::exists(
      directory.filePath(QStringLiteral(".refusion/Journal/accepted-r4.rfx"))));

  current_phase = "fill project reopen";
  require(persisted_project.open(QIODevice::ReadOnly));
  const auto property_source = persisted_project.readAll();
  current_phase = "fill project revision";
  require(property_source.contains("revision(4)"));
  current_phase = "fill project value";
  require(property_source.contains("fill rgba8(\"#102030FF\")"));

  current_phase = "diagnostic verification";
  QFile diagnostics(
      directory.filePath(QStringLiteral(".refusion/Diagnostics/session.jsonl")));
  require(diagnostics.open(QIODevice::ReadOnly));
  const auto records = diagnostics.readAll();
  require(records.contains("RFX-PROJECT-REVISION-ACCEPTED"));
  require(records.contains("RFX-RFX-PARSE-001"));
  require(records.contains("RFX-UI-REVISION-PERSISTED"));

  current_phase = "concurrent open rejection";
  bool concurrent_open_rejected = false;
  try {
    ProjectLiveReloadController concurrent_controller(
        *commands,
        bridge,
        path,
        QStringLiteral("/test/refusion-cli"),
        property_source);
  } catch (const std::runtime_error& error) {
    concurrent_open_rejected =
        std::string_view(error.what()).find("RFX-PROJECT-LOCKED") !=
        std::string_view::npos;
  }
  require(concurrent_open_rejected);

  current_phase = "copied workspace setup";
  // Simulate copying the entire workspace while the source Studio still owns
  // its lock. The copied lock/context belongs to the source path and must not
  // block or leak host-local diagnostics into the destination project.
  const auto copied_root =
      directory.filePath(QStringLiteral("Copied Workspace"));
  const auto copied_refusion =
      copied_root + QStringLiteral("/.refusion");
  require(QDir().mkpath(copied_refusion + QStringLiteral("/Journal")));
  require(QDir().mkpath(copied_refusion + QStringLiteral("/Diagnostics")));
  require(QDir().mkpath(copied_refusion + QStringLiteral("/Cache")));
  const auto copied_project =
      copied_root + QStringLiteral("/Project.rfx");
  require(QFile::copy(path, copied_project));
  require(QFile::copy(
      directory.filePath(QStringLiteral(".refusion/agent-context.json")),
      copied_refusion + QStringLiteral("/agent-context.json")));
  require(QFile::copy(
      directory.filePath(QStringLiteral(".refusion/session.lock")),
      copied_refusion + QStringLiteral("/session.lock")));
  write_atomically(copied_refusion + QStringLiteral("/Journal/foreign.rfx"),
                   QByteArray("FOREIGN-JOURNAL"));
  write_atomically(
      copied_refusion + QStringLiteral("/Diagnostics/session.jsonl"),
      QByteArray("FOREIGN-DIAGNOSTIC\n"));
  write_atomically(copied_refusion + QStringLiteral("/Cache/foreign.cache"),
                   QByteArray("FOREIGN-CACHE"));

  QFile copied_source_file(copied_project);
  require(copied_source_file.open(QIODevice::ReadOnly));
  const auto copied_source = copied_source_file.readAll();
  const auto copied_compiled = refusion::core::compile_project_rfx(
      std::string_view(copied_source.constData(),
                       static_cast<std::size_t>(copied_source.size())));
  require(copied_compiled.succeeded());
  auto copied_commands = refusion::application::create_application_host(
      *copied_compiled.project);
  StudioBridge copied_bridge(*copied_commands);
  auto copied_runtime =
      std::make_shared<FakeRuntime>(*copied_compiled.project);
  copied_commands->set_candidate_admission_port(copied_runtime);
  current_phase = "copied workspace controller";
  ProjectLiveReloadController copied_controller(
      *copied_commands,
      copied_bridge,
      copied_project,
      QStringLiteral("/copied-host/refusion-cli"),
      copied_source);

  current_phase = "copied workspace verification";
  require(!QFileInfo::exists(
      copied_refusion + QStringLiteral("/Journal/foreign.rfx")));
  require(!QFileInfo::exists(
      copied_refusion + QStringLiteral("/Cache/foreign.cache")));
  QFile copied_diagnostics(
      copied_refusion + QStringLiteral("/Diagnostics/session.jsonl"));
  require(copied_diagnostics.open(QIODevice::ReadOnly));
  const auto copied_records = copied_diagnostics.readAll();
  require(!copied_records.contains("FOREIGN-DIAGNOSTIC"));
  require(copied_records.contains("RFX-PROJECT-OPENED"));

  QFile copied_context(
      copied_refusion + QStringLiteral("/agent-context.json"));
  require(copied_context.open(QIODevice::ReadOnly));
  const auto copied_context_bytes = copied_context.readAll();
  require(copied_context_bytes.contains(copied_project.toUtf8()));
  require(copied_context_bytes.contains("/copied-host/refusion-cli"));
  require(!copied_context_bytes.contains(path.toUtf8()));
  const auto copied_context_object =
      QJsonDocument::fromJson(copied_context_bytes).object();
  require(copied_context_object.value(QStringLiteral("schema_version"))
              .toInt() == 2);
  require(copied_context_object.value(QStringLiteral("active_revision"))
              .toString() == QStringLiteral("4"));
  require(QFileInfo::exists(
      copied_refusion + QStringLiteral("/Journal/accepted-r4.rfx")));
  return 0;
}

int main(int argc, char* argv[]) {
  try {
    return run_test(argc, argv);
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
