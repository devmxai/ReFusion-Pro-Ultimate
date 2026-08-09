#include "StudioMediaImportBridge.hpp"

#include "StudioBridge.hpp"
#include "adapters/QtMediaImportWorkspace.hpp"

#include "refusion/adapters/media/FfmpegMediaDemuxer.hpp"

#include <QFileInfo>
#include <QMetaObject>
#include <QThread>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>

namespace {

using namespace refusion::application;

class AtomicMediaCancellation final : public MediaCancellationToken {
 public:
  [[nodiscard]] bool cancelled() const noexcept override {
    return cancelled_.load(std::memory_order_relaxed);
  }
  void cancel() noexcept { cancelled_.store(true, std::memory_order_relaxed); }

 private:
  std::atomic_bool cancelled_{false};
};

class MainThreadRevisionProxy final : public ProjectRevisionService {
 public:
  MainThreadRevisionProxy(ProjectCommandService& commands,
                          StudioBridge& studio_bridge, QObject& dispatcher)
      : commands_(commands),
        studio_bridge_(studio_bridge),
        dispatcher_(dispatcher) {}

  [[nodiscard]] refusion::core::ProjectSnapshot active_snapshot()
      const override {
    if (QThread::currentThread() == dispatcher_.thread()) {
      return commands_.active_snapshot();
    }
    refusion::core::ProjectSnapshot snapshot;
    const auto invoked = QMetaObject::invokeMethod(
        &dispatcher_, [&] { snapshot = commands_.active_snapshot(); },
        Qt::BlockingQueuedConnection);
    if (!invoked) return {};
    return snapshot;
  }

  [[nodiscard]] refusion::core::ApplyResult submit(
      const refusion::core::ReplaceProjectCommand& command) override {
    refusion::core::ApplyResult result;
    const auto submit_on_engine_thread = [&] {
      result = commands_.submit(command);
      if (result.accepted()) {
        studio_bridge_.publishAcceptedWorkflow(result.active_snapshot);
      }
    };
    if (QThread::currentThread() == dispatcher_.thread()) {
      submit_on_engine_thread();
      return result;
    }
    if (!QMetaObject::invokeMethod(&dispatcher_, submit_on_engine_thread,
                                   Qt::BlockingQueuedConnection)) {
      return refusion::core::ApplyResult{
          .status = refusion::core::ApplyStatus::rejected,
          .command_id = command.envelope.command_id,
          .active_snapshot = {},
          .diagnostic = {.code = "RFX-MEDIA-IMPORT-ENGINE-DISPATCH",
                         .message = "could not dispatch import admission to the engine thread",
                         .blocking = true},
      };
    }
    return result;
  }

 private:
  ProjectCommandService& commands_;
  StudioBridge& studio_bridge_;
  QObject& dispatcher_;
};

[[nodiscard]] QString stage_name(const ImportVideoStage stage) {
  switch (stage) {
    case ImportVideoStage::validating:
      return QStringLiteral("Validating source");
    case ImportVideoStage::indexing:
      return QStringLiteral("Indexing container");
    case ImportVideoStage::staging_asset:
      return QStringLiteral("Copying verified original");
    case ImportVideoStage::preparing_revision:
      return QStringLiteral("Preparing linked tracks");
    case ImportVideoStage::committing_asset:
      return QStringLiteral("Committing project asset");
    case ImportVideoStage::publishing_revision:
      return QStringLiteral("Publishing revision");
    case ImportVideoStage::completed:
      return QStringLiteral("Import complete");
  }
  return QStringLiteral("Importing video");
}

}  // namespace

class StudioMediaImportBridge::Implementation final {
 public:
  Implementation(StudioMediaImportBridge& owner,
                 ProjectCommandService& commands,
                 StudioBridge& studio_bridge,
                 QString project_directory,
                 std::function<refusion::core::ProjectTimeNs()> time_provider)
      : owner_(owner),
        revision_proxy_(commands, studio_bridge, owner),
        indexing_(demuxer_, nullptr, 1),
        workspace_(std::move(project_directory)),
        import_(revision_proxy_, indexing_, workspace_, &owner),
        time_provider_(std::move(time_provider)) {}

  ~Implementation() {
    cancel();
    if (worker_.joinable()) worker_.join();
  }

  void start(const QUrl& selected_url) {
    const auto selected_file = selected_url.toLocalFile();
    if (selected_file.isEmpty()) {
      finish(false, QStringLiteral(
                        "RFX-MEDIA-IMPORT-SOURCE-OPEN: select a local MP4 or MOV file"));
      return;
    }
    const auto base = revision_proxy_.active_snapshot();
    const auto sequence = ++command_sequence_;
    const auto timeline_start = time_provider_ ? time_provider_() : 0;
    cancellation_ = std::make_shared<AtomicMediaCancellation>();
    if (worker_.joinable()) worker_.join();
    worker_ = std::jthread(
        [this, selected_file, base, sequence, timeline_start](std::stop_token) {
          const auto source =
              open_immutable_compressed_file_source(selected_file);
          if (!source.succeeded()) {
            finish(false, source.diagnostic);
            return;
          }
          const auto suffix = std::to_string(sequence);
          const auto result = import_.execute(ImportVideoIntent{
              .envelope = refusion::core::CommandEnvelope{
                  .command_id =
                      refusion::core::CommandId{"cmd_qt_import_video_" + suffix},
                  .expected_revision = base.revision_id,
                  .idempotency_key = refusion::core::IdempotencyKey{
                      "qt-import-video-" + suffix},
              },
              .source = source.source,
              .cancellation = cancellation_,
              .original_display_name =
                  QFileInfo(selected_file).fileName().toStdString(),
              .timeline_start = timeline_start,
          });
          finish(result.succeeded(),
                 QString::fromStdString(result.code + ": " +
                                        result.diagnostic));
        });
  }

  void cancel() noexcept {
    if (cancellation_) cancellation_->cancel();
  }

 private:
  void finish(const bool accepted, QString diagnostic) {
    QMetaObject::invokeMethod(
        &owner_,
        [this, accepted, diagnostic = std::move(diagnostic)]() mutable {
          owner_.diagnostic_ = std::move(diagnostic);
          owner_.busy_ = false;
          emit owner_.diagnosticChanged();
          emit owner_.busyChanged();
          emit owner_.importCompleted(accepted);
        },
        Qt::QueuedConnection);
  }

  StudioMediaImportBridge& owner_;
  MainThreadRevisionProxy revision_proxy_;
  refusion::adapters::media::FfmpegMediaDemuxer demuxer_;
  MediaIndexingService indexing_;
  QtMediaImportWorkspace workspace_;
  ImportVideoService import_;
  std::function<refusion::core::ProjectTimeNs()> time_provider_;
  std::shared_ptr<AtomicMediaCancellation> cancellation_;
  std::jthread worker_;
  std::uint64_t command_sequence_{0};
};

StudioMediaImportBridge::StudioMediaImportBridge(
    ProjectCommandService& commands, StudioBridge& studio_bridge,
    QString project_directory,
    std::function<refusion::core::ProjectTimeNs()> timeline_time_provider,
    QObject* parent)
    : QObject(parent),
      implementation_(std::make_unique<Implementation>(
          *this, commands, studio_bridge, std::move(project_directory),
          std::move(timeline_time_provider))) {}

StudioMediaImportBridge::~StudioMediaImportBridge() = default;

bool StudioMediaImportBridge::busy() const noexcept { return busy_; }
QString StudioMediaImportBridge::stage() const { return stage_; }
QString StudioMediaImportBridge::diagnostic() const { return diagnostic_; }

void StudioMediaImportBridge::importSelectedFile(const QUrl& selected_file) {
  if (busy_) {
    diagnostic_ = QStringLiteral("RFX-MEDIA-IMPORT-BUSY: one import is already active");
    emit diagnosticChanged();
    return;
  }
  busy_ = true;
  stage_ = QStringLiteral("Opening selected source");
  diagnostic_.clear();
  emit busyChanged();
  emit stageChanged();
  emit diagnosticChanged();
  implementation_->start(selected_file);
}

void StudioMediaImportBridge::cancelImport() {
  implementation_->cancel();
}

void StudioMediaImportBridge::report(const ImportVideoStage value) noexcept {
  const auto text = stage_name(value);
  QMetaObject::invokeMethod(
      this,
      [this, text] {
        stage_ = text;
        emit stageChanged();
      },
      Qt::QueuedConnection);
}
