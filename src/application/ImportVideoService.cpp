#include "refusion/application/ImportVideoService.hpp"

#include "refusion/core/MediaIndex.hpp"
#include "refusion/core/ProjectClock.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <utility>

namespace refusion::application {
namespace {

constexpr std::uint64_t kNanosecondsPerSecond = 1'000'000'000ULL;

[[nodiscard]] ImportVideoResult rejected(const core::RevisionId revision,
                                         std::string code,
                                         std::string diagnostic) {
  return ImportVideoResult{
      .status = ImportVideoStatus::rejected,
      .active_revision = revision,
      .code = std::move(code),
      .diagnostic = std::move(diagnostic),
  };
}

[[nodiscard]] ImportVideoResult cancelled(const core::RevisionId revision,
                                          std::string diagnostic) {
  return ImportVideoResult{
      .status = ImportVideoStatus::cancelled,
      .active_revision = revision,
      .code = "RFX-MEDIA-IMPORT-CANCELLED",
      .diagnostic = std::move(diagnostic),
  };
}

[[nodiscard]] bool is_cancelled(
    const std::shared_ptr<const MediaCancellationToken>& token) noexcept {
  return token && token->cancelled();
}

void report(ImportVideoProgressPort* progress,
            const ImportVideoStage stage) noexcept {
  if (progress != nullptr) progress->report(stage);
}

[[nodiscard]] std::optional<std::string> digest_token(
    const std::string& digest) {
  constexpr std::string_view prefix = "sha256:";
  if (!digest.starts_with(prefix) || digest.size() != prefix.size() + 64) {
    return std::nullopt;
  }
  const auto hex = digest.substr(prefix.size(), 24);
  if (!std::all_of(hex.begin(), hex.end(), [](const unsigned char value) {
        return (value >= '0' && value <= '9') ||
               (value >= 'a' && value <= 'f');
      })) {
    return std::nullopt;
  }
  return hex;
}

[[nodiscard]] std::optional<std::uint64_t> scaled_unsigned_nanoseconds(
    const std::uint64_t ticks, const core::MediaTimeBase& time_base,
    const bool ceil_result) {
  if (!time_base.valid() || time_base.numerator <= 0) return std::nullopt;

  auto numerator = static_cast<std::uint64_t>(time_base.numerator);
  auto denominator = static_cast<std::uint64_t>(time_base.denominator);
  auto scale = kNanosecondsPerSecond;
  const auto first = std::gcd(numerator, denominator);
  numerator /= first;
  denominator /= first;
  const auto second = std::gcd(scale, denominator);
  scale /= second;
  denominator /= second;

  if (ticks != 0 && numerator >
                        std::numeric_limits<std::uint64_t>::max() / ticks) {
    return std::nullopt;
  }
  const auto ticks_times_numerator = ticks * numerator;
  if (ticks_times_numerator != 0 &&
      scale > std::numeric_limits<std::uint64_t>::max() /
                  ticks_times_numerator) {
    return std::nullopt;
  }
  const auto product = ticks_times_numerator * scale;
  const auto quotient = product / denominator;
  const auto remainder = product % denominator;
  if (ceil_result && remainder != 0) {
    if (quotient == std::numeric_limits<std::uint64_t>::max()) {
      return std::nullopt;
    }
    return quotient + 1;
  }
  return quotient;
}

[[nodiscard]] std::optional<std::int64_t> signed_nanoseconds(
    const std::int64_t ticks, const core::MediaTimeBase& time_base) {
  const auto magnitude = ticks < 0
                             ? static_cast<std::uint64_t>(-(ticks + 1)) + 1
                             : static_cast<std::uint64_t>(ticks);
  const auto converted =
      scaled_unsigned_nanoseconds(magnitude, time_base, false);
  if (!converted || *converted >
                        static_cast<std::uint64_t>(
                            std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  return ticks < 0 ? -static_cast<std::int64_t>(*converted)
                   : static_cast<std::int64_t>(*converted);
}

struct SelectedStreams final {
  const core::MediaStreamDescriptor* video{nullptr};
  const core::MediaStreamDescriptor* audio{nullptr};
};

[[nodiscard]] SelectedStreams select_streams(const core::MediaIndex& index) {
  SelectedStreams selected;
  for (const auto& stream : index.streams) {
    if (stream.kind == core::MediaStreamKind::video && selected.video == nullptr) {
      selected.video = &stream;
    } else if (stream.kind == core::MediaStreamKind::audio &&
               selected.audio == nullptr) {
      selected.audio = &stream;
    }
  }
  return selected;
}

[[nodiscard]] std::string extension_for(
    const core::MediaContainerProfile profile) {
  return profile == core::MediaContainerProfile::quicktime_mov ? "mov" : "mp4";
}

[[nodiscard]] bool has_id_collision(const core::ProjectSnapshot& project,
                                    const std::string& token) {
  const auto asset = "ast_" + token;
  const auto source = "media_" + token;
  const auto linked = "import_" + token;
  const auto video = "vclip_" + token;
  const auto audio = "aclip_" + token;
  if (std::any_of(project.assets.begin(), project.assets.end(),
                  [&](const core::AssetRecord& value) {
                    return value.asset_id.value == asset;
                  }) ||
      std::any_of(project.media_sources.begin(), project.media_sources.end(),
                  [&](const core::MediaSource& value) {
                    return value.media_source_id.value == source;
                  }) ||
      std::any_of(project.linked_imports.begin(), project.linked_imports.end(),
                  [&](const core::LinkedImport& value) {
                    return value.linked_import_id.value == linked;
                  })) {
    return true;
  }
  const auto& composition = *project.composition;
  return std::any_of(
             composition.video_clips.begin(), composition.video_clips.end(),
             [&](const core::VideoClipSnapshot& value) {
               return value.video_clip_id.value == video;
             }) ||
         std::any_of(
             composition.audio_clips.begin(), composition.audio_clips.end(),
             [&](const core::AudioClipSnapshot& value) {
               return value.audio_clip_id.value == audio;
             });
}

[[nodiscard]] std::optional<core::ProjectSnapshot> build_candidate(
    const core::ProjectSnapshot& active, const core::MediaIndex& index,
    const ImportVideoIntent& intent, const std::string& token,
    std::string& failure) {
  const auto selected = select_streams(index);
  if (selected.video == nullptr) {
    failure = "indexed source does not contain the required Video stream";
    return std::nullopt;
  }

  const auto video_origin =
      signed_nanoseconds(selected.video->start, selected.video->time_base);
  const auto video_duration = scaled_unsigned_nanoseconds(
      selected.video->duration, selected.video->time_base, true);
  if (!video_origin || !video_duration || *video_duration == 0) {
    failure = "Video timing cannot be represented in canonical project time";
    return std::nullopt;
  }
  std::optional<std::int64_t> audio_origin;
  std::optional<std::uint64_t> audio_duration;
  if (selected.audio != nullptr) {
    audio_origin =
        signed_nanoseconds(selected.audio->start, selected.audio->time_base);
    audio_duration = scaled_unsigned_nanoseconds(
        selected.audio->duration, selected.audio->time_base, true);
    if (!audio_origin || !audio_duration || *audio_duration == 0) {
      failure = "Audio timing cannot be represented in canonical project time";
      return std::nullopt;
    }
  }

  const auto earliest_origin =
      audio_origin ? std::min(*video_origin, *audio_origin) : *video_origin;
  const auto offset_for = [&](const std::int64_t origin)
      -> std::optional<core::ProjectTimeNs> {
    if (origin < earliest_origin) return std::nullopt;
    std::uint64_t offset = 0;
    if (earliest_origin < 0 && origin >= 0) {
      const auto before_zero =
          static_cast<std::uint64_t>(-(earliest_origin + 1)) + 1;
      const auto after_zero = static_cast<std::uint64_t>(origin);
      if (after_zero > std::numeric_limits<std::uint64_t>::max() - before_zero) {
        return std::nullopt;
      }
      offset = before_zero + after_zero;
    } else {
      offset = static_cast<std::uint64_t>(origin - earliest_origin);
    }
    if (intent.timeline_start >
        std::numeric_limits<core::ProjectTimeNs>::max() - offset) {
      return std::nullopt;
    }
    return intent.timeline_start + offset;
  };
  const auto video_start = offset_for(*video_origin);
  const auto audio_start =
      audio_origin ? offset_for(*audio_origin) : std::nullopt;
  if (!video_start || (audio_origin && !audio_start)) {
    failure = "media start offset exceeds canonical project time";
    return std::nullopt;
  }

  core::ProjectSnapshot candidate = active;
  if (candidate.revision_id.value ==
      std::numeric_limits<std::uint64_t>::max()) {
    failure = "project revision is exhausted";
    return std::nullopt;
  }
  ++candidate.revision_id.value;

  const core::AssetId asset_id{"ast_" + token};
  const core::MediaSourceId source_id{"media_" + token};
  const core::LinkedImportId linked_id{"import_" + token};
  const core::VideoClipId video_clip_id{"vclip_" + token};
  const core::AudioClipId audio_clip_id{"aclip_" + token};
  const std::string original_path = "Assets/Media/" + asset_id.value +
                                    "/original." +
                                    extension_for(index.container_profile);

  candidate.assets.push_back(core::AssetRecord{
      .asset_id = asset_id,
      .content_digest = index.source_digest,
      .byte_size = index.source_byte_size,
      .media_kind = core::AssetMediaKind::video_container,
      .project_relative_original = original_path,
      .original_display_name = intent.original_display_name,
      .provenance = core::AssetProvenance::imported_copy,
  });

  core::MediaSource media_source{
      .media_source_id = source_id,
      .asset_id = asset_id,
      .media_index_contract_version = index.contract_version,
      .media_index_digest = core::media_index_digest(index),
      .resolution = core::MediaResolutionState::resolved,
  };
  for (const auto& stream : index.streams) {
    auto descriptor = stream;
    descriptor.stream_id = core::MediaStreamId{
        "stream_" + token +
        (stream.kind == core::MediaStreamKind::video ? "_v" : "_a")};
    if (stream.kind == core::MediaStreamKind::video) {
      media_source.selected_video_stream = descriptor.stream_id;
    } else {
      media_source.selected_audio_stream = descriptor.stream_id;
    }
    media_source.streams.push_back(std::move(descriptor));
  }
  candidate.media_sources.push_back(std::move(media_source));

  auto& composition = *candidate.composition;
  composition.video_clips.push_back(core::VideoClipSnapshot{
      .video_clip_id = video_clip_id,
      .linked_import_id = linked_id,
      .media_source_id = source_id,
      .stream_id = *candidate.media_sources.back().selected_video_stream,
      .display_name = intent.original_display_name,
      .active_range = {.start = *video_start, .duration = *video_duration},
      .source_range = {.start = selected.video->start,
                       .duration = selected.video->duration},
      .enabled = true,
      .locked = false,
  });

  core::LinkedImport linked{
      .linked_import_id = linked_id,
      .media_source_id = source_id,
      .video_clip_id = video_clip_id,
  };
  core::ProjectTimeNs required_end =
      composition.video_clips.back().active_range.end();
  if (selected.audio != nullptr) {
    composition.audio_clips.push_back(core::AudioClipSnapshot{
        .audio_clip_id = audio_clip_id,
        .linked_import_id = linked_id,
        .media_source_id = source_id,
        .stream_id = *candidate.media_sources.back().selected_audio_stream,
        .display_name = intent.original_display_name,
        .active_range = {.start = *audio_start, .duration = *audio_duration},
        .source_range = {.start = selected.audio->start,
                         .duration = selected.audio->duration},
        .enabled = true,
        .locked = false,
        .gain = 1.0,
        .muted = false,
        .solo = false,
    });
    linked.audio_clip_id = audio_clip_id;
    required_end =
        std::max(required_end, composition.audio_clips.back().active_range.end());
  }
  candidate.linked_imports.push_back(std::move(linked));
  const auto requested_duration = std::max(composition.duration, required_end);
  const core::ProjectClockSpec clock_spec{
      .duration_ns = requested_duration,
      .frame_rate = composition.frame_rate,
      .loop = false,
  };
  if (!clock_spec.valid()) {
    failure = "imported media extent cannot be represented by the ProjectClock";
    return std::nullopt;
  }
  composition.duration = clock_spec.time_at_frame(clock_spec.frame_count());
  if (composition.duration < required_end) {
    failure = "frame-aligned Composition duration cannot contain imported media";
    return std::nullopt;
  }

  const auto validation = core::validate_project(candidate);
  if (!validation.valid) {
    failure = validation.code + ": " + validation.message;
    return std::nullopt;
  }
  return candidate;
}

}  // namespace

ImportVideoService::ImportVideoService(ProjectRevisionService& commands,
                                       MediaIndexingService& indexing,
                                       MediaImportWorkspacePort& workspace,
                                       ImportVideoProgressPort* progress) noexcept
    : commands_(commands),
      indexing_(indexing),
      workspace_(workspace),
      progress_(progress) {}

ImportVideoResult ImportVideoService::execute(ImportVideoIntent intent) {
  report(progress_, ImportVideoStage::validating);
  auto active = commands_.active_snapshot();
  if (!intent.source) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-SOURCE-INVALID",
                    "ImportVideo requires an immutable source lease");
  }
  if (is_cancelled(intent.cancellation)) {
    return cancelled(active.revision_id,
                     "ImportVideo was cancelled before validation");
  }
  const auto token = digest_token(intent.source->content_digest());
  if (!token || intent.source->byte_size() == 0) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-SOURCE-IDENTITY",
                    "source digest or byte size is invalid");
  }
  const auto duplicate = std::find_if(
      active.assets.begin(), active.assets.end(),
      [&](const core::AssetRecord& asset) {
        return asset.content_digest == intent.source->content_digest() &&
               asset.byte_size == intent.source->byte_size();
      });
  if (duplicate != active.assets.end()) {
    const auto source = std::find_if(
        active.media_sources.begin(), active.media_sources.end(),
        [&](const core::MediaSource& value) {
          return value.asset_id == duplicate->asset_id;
        });
    const auto linked = source == active.media_sources.end()
                            ? active.linked_imports.end()
                            : std::find_if(
                                  active.linked_imports.begin(),
                                  active.linked_imports.end(),
                                  [&](const core::LinkedImport& value) {
                                    return value.media_source_id ==
                                           source->media_source_id;
                                  });
    return ImportVideoResult{
        .status = ImportVideoStatus::replayed,
        .active_revision = active.revision_id,
        .asset_id = duplicate->asset_id,
        .media_source_id = source == active.media_sources.end()
                               ? core::MediaSourceId{}
                               : source->media_source_id,
        .linked_import_id = linked == active.linked_imports.end()
                                ? core::LinkedImportId{}
                                : linked->linked_import_id,
        .code = "RFX-MEDIA-IMPORT-ALREADY-PRESENT",
        .diagnostic = "byte-identical media is already imported",
    };
  }
  if (has_id_collision(active, *token)) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-ID-COLLISION",
                    "content-derived media identity collides with project state");
  }

  report(progress_, ImportVideoStage::indexing);
  MediaDemuxResult indexed;
  try {
    indexed = indexing_.index_async(MediaIndexingRequest{
        .source = intent.source,
        .cancellation = intent.cancellation,
        .permit_cache = true,
    }).get();
  } catch (const std::exception& error) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-INDEX-EXCEPTION",
                    error.what());
  } catch (...) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-INDEX-EXCEPTION",
                    "media indexing failed with an unknown exception");
  }
  active = commands_.active_snapshot();
  if (indexed.state == MediaDemuxState::cancelled ||
      is_cancelled(intent.cancellation)) {
    return cancelled(active.revision_id,
                     "ImportVideo was cancelled during indexing");
  }
  if (!indexed.succeeded()) {
    return rejected(active.revision_id, std::move(indexed.code),
                    std::move(indexed.diagnostic));
  }
  if (indexed.index->source_digest != intent.source->content_digest() ||
      indexed.index->source_byte_size != intent.source->byte_size()) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-INDEX-IDENTITY",
                    "MediaIndex does not describe the admitted source bytes");
  }

  report(progress_, ImportVideoStage::preparing_revision);
  std::string failure;
  auto candidate = build_candidate(active, *indexed.index, intent, *token,
                                   failure);
  if (!candidate) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-CANDIDATE-INVALID",
                    std::move(failure));
  }
  const auto& asset = candidate->assets.back();
  const MediaAssetMaterializationReceipt expected{
      .asset_id = asset.asset_id,
      .content_digest = asset.content_digest,
      .byte_size = asset.byte_size,
      .project_relative_original = asset.project_relative_original,
  };

  report(progress_, ImportVideoStage::staging_asset);
  std::unique_ptr<PreparedMediaAsset> prepared;
  try {
    prepared = workspace_.prepare_copy(
        intent.envelope.idempotency_key.value, expected, *intent.source,
        intent.cancellation.get());
  } catch (const std::exception& error) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-STAGE-EXCEPTION",
                    error.what());
  } catch (...) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-STAGE-EXCEPTION",
                    "asset staging failed with an unknown exception");
  }
  if (!prepared) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-STAGE-FAILED",
                    "workspace adapter did not prepare the copied asset");
  }
  if (prepared->receipt() != expected) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-STAGE-IDENTITY",
                    "materialized asset receipt differs from project truth");
  }
  if (is_cancelled(intent.cancellation)) {
    return cancelled(active.revision_id,
                     "ImportVideo was cancelled before asset commit");
  }

  report(progress_, ImportVideoStage::committing_asset);
  if (!prepared->commit()) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-ASSET-COMMIT",
                    "workspace adapter could not atomically publish the asset");
  }
  if (is_cancelled(intent.cancellation)) {
    return cancelled(active.revision_id,
                     "ImportVideo was cancelled before revision publication");
  }

  report(progress_, ImportVideoStage::publishing_revision);
  core::ApplyResult applied;
  try {
    applied = commands_.submit(core::ReplaceProjectCommand{
        .envelope = intent.envelope,
        .candidate = std::move(*candidate),
    });
  } catch (const std::exception& error) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-PUBLISH-EXCEPTION",
                    error.what());
  } catch (...) {
    return rejected(active.revision_id, "RFX-MEDIA-IMPORT-PUBLISH-EXCEPTION",
                    "revision publication failed with an unknown exception");
  }
  if (!applied.accepted()) {
    return rejected(applied.active_snapshot.revision_id,
                    std::move(applied.diagnostic.code),
                    std::move(applied.diagnostic.message));
  }
  prepared->retain();
  report(progress_, ImportVideoStage::completed);
  return ImportVideoResult{
      .status = applied.replayed() ? ImportVideoStatus::replayed
                                  : ImportVideoStatus::accepted,
      .active_revision = applied.committed_revision,
      .asset_id = expected.asset_id,
      .media_source_id = applied.active_snapshot.media_sources.back().media_source_id,
      .linked_import_id =
          applied.active_snapshot.linked_imports.back().linked_import_id,
      .code = applied.replayed() ? "RFX-MEDIA-IMPORT-REPLAYED"
                                 : "RFX-MEDIA-IMPORT-ACCEPTED",
      .diagnostic = "linked Video and Audio clips published atomically",
  };
}

}  // namespace refusion::application
