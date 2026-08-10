#include "composition/StudioVideoPlaybackController.hpp"

#include "adapters/QtMediaImportWorkspace.hpp"

#include "refusion/adapters/media/FfmpegMediaDemuxer.hpp"
#include "refusion/adapters/skia/SkiaGpuContexts.hpp"
#include "refusion/core/MediaIndex.hpp"
#include "refusion/platform/PlatformMediaCapability.hpp"
#include "refusion/runtime/media/MediaIndexDecodeProjection.hpp"
#include "refusion/runtime/presentation/ViewportPresentation.hpp"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace {

using refusion::core::AssetRecord;
using refusion::core::MediaSource;
using refusion::core::MediaStreamDescriptor;
using refusion::core::ProjectSnapshot;
using refusion::core::VideoClipSnapshot;
using refusion::runtime::media::HardwareVideoDecoder;
using refusion::runtime::media::HardwareVideoPlaybackSession;

struct PlaybackBinding final {
  VideoClipSnapshot clip;
  MediaStreamDescriptor stream;
  MediaSource source;
  AssetRecord asset;
  QString absolute_source_path;
  std::string source_key;
};

struct PreparedDecoder final {
  std::string source_key;
  std::uint64_t last_source_frame_index{0};
  std::unique_ptr<HardwareVideoDecoder> decoder;
  std::unique_ptr<HardwareVideoPlaybackSession> session;
};

[[nodiscard]] bool queue_covers_target_with_lookahead(
    const std::shared_ptr<const refusion::runtime::media::DecodedSurfaceQueue>&
        queue,
    const refusion::runtime::media::ExactMediaTime target,
    const std::size_t minimum_lookahead,
    const std::uint64_t last_source_frame_index) {
  if (!queue || queue->empty()) return false;
  const auto selected = queue->select_at(target);
  if (!selected) return false;
  const auto& timing = selected->info().timing;
  if (timing.presentation_time.timescale != timing.duration.timescale ||
      timing.duration.value <= 0 ||
      timing.presentation_time.value >
          std::numeric_limits<std::int64_t>::max() - timing.duration.value ||
      refusion::runtime::media::compare_exact_media_time(
          target,
          {.value = timing.presentation_time.value + timing.duration.value,
           .timescale = timing.presentation_time.timescale}) !=
          std::strong_ordering::less) {
    return false;
  }
  std::size_t selected_index = queue->size();
  for (std::size_t index = 0; index < queue->size(); ++index) {
    if (queue->frame(index)->info().lease_id == selected->info().lease_id) {
      selected_index = index;
      break;
    }
  }
  if (selected_index == queue->size()) return false;
  const auto available = queue->size() - selected_index;
  const bool reaches_end =
      queue->frame(queue->size() - 1)->info().source_frame_index ==
      last_source_frame_index;
  return available > minimum_lookahead || reaches_end;
}

[[nodiscard]] const MediaSource* find_source(
    const ProjectSnapshot& project,
    const refusion::core::MediaSourceId& id) noexcept {
  const auto iterator = std::find_if(
      project.media_sources.begin(), project.media_sources.end(),
      [&id](const auto& source) { return source.media_source_id == id; });
  return iterator == project.media_sources.end() ? nullptr : &*iterator;
}

[[nodiscard]] const MediaStreamDescriptor* find_stream(
    const MediaSource& source,
    const refusion::core::MediaStreamId& id) noexcept {
  const auto iterator = std::find_if(
      source.streams.begin(), source.streams.end(),
      [&id](const auto& stream) { return stream.stream_id == id; });
  return iterator == source.streams.end() ? nullptr : &*iterator;
}

[[nodiscard]] const AssetRecord* find_asset(
    const ProjectSnapshot& project,
    const refusion::core::AssetId& id) noexcept {
  const auto iterator = std::find_if(
      project.assets.begin(), project.assets.end(),
      [&id](const auto& asset) { return asset.asset_id == id; });
  return iterator == project.assets.end() ? nullptr : &*iterator;
}

[[nodiscard]] bool path_is_within(const QString& root,
                                  const QString& path) {
  const auto clean_root = QDir::cleanPath(root);
  const auto clean_path = QDir::cleanPath(path);
  return clean_path == clean_root ||
         clean_path.startsWith(clean_root + QDir::separator());
}

[[nodiscard]] std::optional<PlaybackBinding> select_binding(
    const ProjectSnapshot& project,
    const QString& project_directory,
    QString* diagnostic) {
  if (!project.composition) return std::nullopt;
  const auto clip_iterator = std::find_if(
      project.composition->video_clips.begin(),
      project.composition->video_clips.end(),
      [](const auto& clip) { return clip.enabled; });
  if (clip_iterator == project.composition->video_clips.end()) {
    return std::nullopt;
  }
  const auto* source = find_source(project, clip_iterator->media_source_id);
  const auto* stream =
      source == nullptr ? nullptr : find_stream(*source, clip_iterator->stream_id);
  const auto* asset =
      source == nullptr ? nullptr : find_asset(project, source->asset_id);
  if (source == nullptr || stream == nullptr || asset == nullptr ||
      source->resolution != refusion::core::MediaResolutionState::resolved ||
      stream->kind != refusion::core::MediaStreamKind::video) {
    if (diagnostic != nullptr) {
      *diagnostic = QStringLiteral(
          "RFX-MEDIA-PLAYBACK-BINDING: accepted Video references are unresolved");
    }
    return std::nullopt;
  }

  const auto canonical_root = QFileInfo(project_directory).canonicalFilePath();
  const auto candidate = QDir(project_directory).absoluteFilePath(
      QString::fromStdString(asset->project_relative_original));
  const auto canonical_source = QFileInfo(candidate).canonicalFilePath();
  if (canonical_root.isEmpty() || canonical_source.isEmpty() ||
      !path_is_within(canonical_root, canonical_source)) {
    if (diagnostic != nullptr) {
      *diagnostic = QStringLiteral(
          "RFX-MEDIA-PLAYBACK-ASSET-PATH: accepted original is unavailable or escapes the project");
    }
    return std::nullopt;
  }

  return PlaybackBinding{
      .clip = *clip_iterator,
      .stream = *stream,
      .source = *source,
      .asset = *asset,
      .absolute_source_path = canonical_source,
      .source_key = asset->content_digest + "|" + source->media_index_digest +
                    "|" + stream->stream_id.value,
  };
}

[[nodiscard]] std::optional<PreparedDecoder> prepare_decoder(
    const PlaybackBinding& binding,
    refusion::runtime::gpu::GpuDeviceService& gpu_device_service,
    QString* diagnostic) {
  auto opened =
      open_immutable_compressed_file_source(binding.absolute_source_path);
  if (!opened.succeeded()) {
    if (diagnostic != nullptr) *diagnostic = opened.diagnostic;
    return std::nullopt;
  }
  if (opened.source->content_digest() != binding.asset.content_digest ||
      opened.source->byte_size() != binding.asset.byte_size) {
    if (diagnostic != nullptr) {
      *diagnostic = QStringLiteral(
          "RFX-MEDIA-PLAYBACK-ASSET-DIGEST: accepted original bytes changed");
    }
    return std::nullopt;
  }

  refusion::adapters::media::FfmpegMediaDemuxer demuxer;
  auto demuxed = demuxer.build_index(*opened.source);
  if (!demuxed.succeeded()) {
    if (diagnostic != nullptr) {
      *diagnostic = QString::fromStdString(
          demuxed.code + ": " + demuxed.diagnostic);
    }
    return std::nullopt;
  }
  const auto& index = *demuxed.index;
  if (index.contract_version != binding.source.media_index_contract_version ||
      index.source_digest != binding.asset.content_digest ||
      index.source_byte_size != binding.asset.byte_size ||
      refusion::core::media_index_digest(index) !=
          binding.source.media_index_digest) {
    if (diagnostic != nullptr) {
      *diagnostic = QStringLiteral(
          "RFX-MEDIA-PLAYBACK-INDEX-DIGEST: rebuilt MediaIndex differs from accepted project truth");
    }
    return std::nullopt;
  }

  // Import assigns project-global StreamIds while retaining the container
  // track identity and every stream semantic field. Rebuilt MediaIndex IDs are
  // derived cache identities, so resolve through the stable container track
  // identity and prove that no stream meaning drifted before projection.
  const auto indexed_stream = std::find_if(
      index.streams.begin(), index.streams.end(), [&binding](const auto& stream) {
        return stream.kind == refusion::core::MediaStreamKind::video &&
               stream.container_track_id == binding.stream.container_track_id;
      });
  if (indexed_stream == index.streams.end()) {
    if (diagnostic != nullptr) {
      *diagnostic = QStringLiteral(
          "RFX-MEDIA-PLAYBACK-STREAM-IDENTITY: accepted container Video track is absent");
    }
    return std::nullopt;
  }
  auto admitted_stream = *indexed_stream;
  admitted_stream.stream_id = binding.stream.stream_id;
  if (admitted_stream != binding.stream) {
    if (diagnostic != nullptr) {
      *diagnostic = QStringLiteral(
          "RFX-MEDIA-PLAYBACK-STREAM-DRIFT: rebuilt Video stream differs from accepted project truth");
    }
    return std::nullopt;
  }

  auto projected =
      refusion::runtime::media::project_media_index_for_hardware_decode(
          index, indexed_stream->stream_id);
  if (!projected.succeeded()) {
    if (diagnostic != nullptr) {
      *diagnostic = QString::fromStdString(
          projected.code + ": " + projected.diagnostic);
    }
    return std::nullopt;
  }

  auto decoder = refusion::platform::create_platform_hardware_video_decoder(
      gpu_device_service);
  if (!decoder) {
    if (diagnostic != nullptr) {
      *diagnostic = QStringLiteral(
          "RFX-MEDIA-PLAYBACK-DECODER: platform hardware decoder is unavailable");
    }
    return std::nullopt;
  }
  const auto& projection = *projected.projection;
  auto session = decoder->open_playback({
      .source_path = binding.absolute_source_path.toUtf8().toStdString(),
      .source_byte_size = projection.source_byte_size,
      .expected_profile = projection.expected_profile,
      .codec_configuration = projection.codec_configuration,
      .samples_decode_order = projection.samples_decode_order,
  });
  if (!session) {
    if (diagnostic != nullptr) {
      *diagnostic = QStringLiteral(
          "RFX-MEDIA-PLAYBACK-SESSION: hardware decoder rejected the accepted source");
    }
    return std::nullopt;
  }

  return PreparedDecoder{
      .source_key = binding.source_key,
      .last_source_frame_index =
          projection.samples_decode_order.size() - 1,
      .decoder = std::move(decoder),
      .session = std::move(session),
  };
}

}  // namespace

class StudioVideoPlaybackController::Implementation final {
 public:
  Implementation(
      QString project_path,
      const ProjectSnapshot& initial_project,
      refusion::runtime::gpu::GpuDeviceService& gpu_device_service,
      refusion::adapters::skia::SkiaGpuContexts& renderer,
      refusion::runtime::presentation::ViewportRenderSession& render_session)
      : project_directory_(QFileInfo(std::move(project_path)).absolutePath()),
        gpu_device_service_(gpu_device_service),
        renderer_(renderer),
        render_session_(render_session),
        pending_project_(std::make_shared<const ProjectSnapshot>(initial_project)),
        trace_enabled_(qEnvironmentVariableIsSet("REFUSION_MEDIA_TRACE")),
        worker_([this](const std::stop_token stop) { run(stop); }) {}

  ~Implementation() {
    worker_.request_stop();
    wake_.notify_all();
  }

  void publish_project(const ProjectSnapshot& project) noexcept {
    try {
      auto snapshot = std::make_shared<const ProjectSnapshot>(project);
      {
        std::scoped_lock lock(mutex_);
        pending_project_ = std::move(snapshot);
        ++project_generation_;
      }
      wake_.notify_all();
    } catch (...) {
      qWarning().noquote()
          << QStringLiteral(
                 "RFX-MEDIA-PLAYBACK-PUBLISH: could not retain accepted project snapshot");
    }
  }

 private:
  void report_once(const QString& diagnostic) {
    if (diagnostic.isEmpty() || diagnostic == last_diagnostic_) return;
    last_diagnostic_ = diagnostic;
    qWarning().noquote() << diagnostic;
  }

  void clear_video_queue() {
    static_cast<void>(renderer_.publish_decoded_video_queue({}, nullptr));
  }

  void run(const std::stop_token stop) {
    std::shared_ptr<const ProjectSnapshot> project;
    std::optional<PlaybackBinding> binding;
    std::optional<PreparedDecoder> prepared;
    std::shared_ptr<const refusion::runtime::media::DecodedSurfaceQueue>
        published_queue;
    std::uint64_t observed_generation = 0;
    std::optional<std::uint64_t> traced_source_frame;
    auto traced_at = std::chrono::steady_clock::now();

    while (!stop.stop_requested()) {
      {
        std::scoped_lock lock(mutex_);
        if (observed_generation != project_generation_ || !project) {
          project = pending_project_;
          observed_generation = project_generation_;
        } else {
          project.reset();
        }
      }

      if (project) {
        QString diagnostic;
        auto next_binding =
            select_binding(*project, project_directory_, &diagnostic);
        if (!next_binding) {
          binding.reset();
          prepared.reset();
          published_queue.reset();
          clear_video_queue();
          report_once(diagnostic);
        } else {
          const bool source_changed =
              !prepared || prepared->source_key != next_binding->source_key;
          binding = std::move(next_binding);
          if (source_changed) {
            clear_video_queue();
            prepared.reset();
            published_queue.reset();
            try {
              prepared = prepare_decoder(*binding, gpu_device_service_,
                                         &diagnostic);
            } catch (const std::exception& error) {
              diagnostic =
                  QStringLiteral("RFX-MEDIA-PLAYBACK-PREPARE: %1")
                      .arg(QString::fromUtf8(error.what()));
            } catch (...) {
              diagnostic = QStringLiteral(
                  "RFX-MEDIA-PLAYBACK-PREPARE: unknown playback preparation failure");
            }
            if (!prepared) report_once(diagnostic);
          }
        }
      }

      if (binding && prepared) {
        const auto playback = render_session_.playback_state();
        const auto source_time =
            refusion::core::video_source_time_at_project_time(
                binding->clip, binding->stream, playback.position_ns);
        if (source_time) {
          const refusion::runtime::media::ExactMediaTime target{
              .value = source_time->value,
              .timescale = source_time->timescale,
          };
          // Use distinct low/high watermarks. VideoToolbox commonly needs
          // about 40 ms to extend this 4K H.264 stream. Waiting until the
          // published queue is almost empty makes that work visible to the
          // render loop. Refill while roughly 330 ms are still published and
          // require the decoder to restore a larger high watermark before a
          // replacement queue is admitted. Eight published source frames are
          // roughly 267 ms at 30 fps, still above the measured worst refill.
          // The distinct decoder high watermark prevents the short-window
          // refill loop that originally starved presentation.
          constexpr std::size_t kMaximumResidentSurfaces = 14;
          constexpr std::size_t kPublishedLowWatermark = 8;
          constexpr std::size_t kDecoderHighWatermark = 12;
          if (!queue_covers_target_with_lookahead(
                  published_queue, target, kPublishedLowWatermark,
                  prepared->last_source_frame_index)) {
            try {
              const auto decode_started = std::chrono::steady_clock::now();
              auto window = prepared->session->decode_window({
                  .target_presentation_time = target,
                  .maximum_surface_count = kMaximumResidentSurfaces,
                  .lookahead_surface_count = kDecoderHighWatermark,
              });
              if (window.admitted() &&
                  renderer_.publish_decoded_video_queue(
                      binding->stream.stream_id.value, window.queue)) {
                if (trace_enabled_) {
                  const auto elapsed =
                      std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::steady_clock::now() - decode_started);
                  qInfo().noquote()
                      << QStringLiteral(
                             "RFX-MEDIA-TRACE window target=%1/%2 frames=%3..%4 resident=%5 decode_us=%6 sessions=%7")
                             .arg(source_time->value)
                             .arg(source_time->timescale)
                             .arg(window.queue->frame(0)
                                      ->info()
                                      .source_frame_index)
                             .arg(window.queue->frame(window.queue->size() - 1)
                                      ->info()
                                      .source_frame_index)
                             .arg(window.queue->size())
                             .arg(elapsed.count())
                             .arg(window.counters.hardware_decoder_sessions);
                }
                published_queue = std::move(window.queue);
                last_diagnostic_.clear();
                if (!playback.running) {
                  const auto frame = render_session_.render_once();
                  if (frame.status == refusion::runtime::presentation::
                                          FrameStatus::rejected) {
                    report_once(QString::fromStdString(frame.diagnostic));
                  }
                }
              } else {
                report_once(QString::fromStdString(
                    window.code + ": " + window.diagnostic));
              }
            } catch (const std::exception& error) {
              report_once(QStringLiteral("RFX-MEDIA-PLAYBACK-DECODE: %1")
                              .arg(QString::fromUtf8(error.what())));
            } catch (...) {
              report_once(QStringLiteral(
                  "RFX-MEDIA-PLAYBACK-DECODE: unknown hardware decode failure"));
            }
          }
        }
      }

      if (trace_enabled_) {
        const auto selected = renderer_.selected_video_source_frame_index();
        if (selected && selected != traced_source_frame) {
          const auto now = std::chrono::steady_clock::now();
          const auto elapsed =
              std::chrono::duration_cast<std::chrono::microseconds>(
                  now - traced_at);
          qInfo().noquote()
              << QStringLiteral(
                     "RFX-MEDIA-TRACE presented source_frame=%1 project_frame=%2 delta_us=%3")
                     .arg(*selected)
                     .arg(render_session_.playback_state().frame_index)
                     .arg(elapsed.count());
          traced_source_frame = selected;
          traced_at = now;
        }
      }

      std::unique_lock lock(mutex_);
      wake_.wait_for(lock, std::chrono::milliseconds(8), [this, observed_generation,
                                                         &stop] {
        return stop.stop_requested() ||
               project_generation_ != observed_generation;
      });
    }
    clear_video_queue();
  }

  QString project_directory_;
  refusion::runtime::gpu::GpuDeviceService& gpu_device_service_;
  refusion::adapters::skia::SkiaGpuContexts& renderer_;
  refusion::runtime::presentation::ViewportRenderSession& render_session_;
  std::mutex mutex_;
  std::condition_variable wake_;
  std::shared_ptr<const ProjectSnapshot> pending_project_;
  std::uint64_t project_generation_{1};
  QString last_diagnostic_;
  bool trace_enabled_{false};
  std::jthread worker_;
};

StudioVideoPlaybackController::StudioVideoPlaybackController(
    QString project_path,
    const ProjectSnapshot& initial_project,
    refusion::runtime::gpu::GpuDeviceService& gpu_device_service,
    refusion::adapters::skia::SkiaGpuContexts& renderer,
    refusion::runtime::presentation::ViewportRenderSession& render_session)
    : implementation_(std::make_unique<Implementation>(
          std::move(project_path), initial_project, gpu_device_service, renderer,
          render_session)) {}

StudioVideoPlaybackController::~StudioVideoPlaybackController() = default;

void StudioVideoPlaybackController::publishProject(
    const ProjectSnapshot& project) noexcept {
  implementation_->publish_project(project);
}
