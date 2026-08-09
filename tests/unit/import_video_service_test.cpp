#include "refusion/application/ImportVideoService.hpp"
#include "refusion/application/ExactAssetRelinkService.hpp"

#include "refusion/core/ContentDigest.hpp"
#include "refusion/core/ProjectCreation.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace refusion::application;
using namespace refusion::core;

void require(const bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

class MemorySource final : public ImmutableCompressedSourceLease {
 public:
  explicit MemorySource(const std::uint8_t value = 0x51)
      : bytes_(4096, value), digest_(sha256_content_digest(bytes_)) {}

  [[nodiscard]] std::string content_digest() const override { return digest_; }
  [[nodiscard]] std::uint64_t byte_size() const noexcept override {
    return bytes_.size();
  }
  [[nodiscard]] CompressedSourceReadResult read_at(
      const std::uint64_t offset,
      const std::span<std::uint8_t> destination) noexcept override {
    if (offset > bytes_.size()) {
      return {.state = CompressedSourceReadState::failed};
    }
    if (offset == bytes_.size()) {
      return {.state = CompressedSourceReadState::end_of_source};
    }
    const auto count = std::min(
        destination.size(), bytes_.size() - static_cast<std::size_t>(offset));
    std::copy_n(bytes_.data() + static_cast<std::size_t>(offset), count,
                destination.data());
    return {.state = CompressedSourceReadState::read, .bytes_read = count};
  }

 private:
  std::vector<std::uint8_t> bytes_;
  std::string digest_;
};

[[nodiscard]] MediaIndex indexed_source(
    const ImmutableCompressedSourceLease& source) {
  const std::vector<std::uint8_t> video_configuration{1, 100, 0, 40};
  const std::vector<std::uint8_t> audio_configuration{18, 16};
  const auto video_digest = sha256_content_digest(video_configuration);
  const auto audio_digest = sha256_content_digest(audio_configuration);
  MediaIndex index{
      .contract_version = 1,
      .source_digest = source.content_digest(),
      .source_byte_size = source.byte_size(),
      .container_profile = MediaContainerProfile::iso_bmff_mp4,
  };
  index.streams.push_back(MediaStreamDescriptor{
      .stream_id = MediaStreamId{"stream_track_1"},
      .container_track_id = 1,
      .kind = MediaStreamKind::video,
      .codec = MediaCodec::h264_avc,
      .codec_configuration_digest = video_digest,
      .time_base = {.numerator = 1, .denominator = 30'000},
      .start = 15'990,
      .duration = 89'000,
      .format = VideoStreamFormat{
          .coded_extent = {.width_pixels = 640, .height_pixels = 360},
          .display_extent = {.width_pixels = 640, .height_pixels = 360},
          .presentation_rate = {.numerator = 30, .denominator = 1},
          .bit_depth = 8,
          .chroma_subsampling_x = 2,
          .chroma_subsampling_y = 2,
          .color_range = MediaColorRange::video,
          .color_primaries = "bt709",
          .color_transfer = "bt709",
          .color_matrix = "bt709",
          .orientation_degrees = 0,
          .sample_aspect_numerator = 1,
          .sample_aspect_denominator = 1,
      },
  });
  index.streams.push_back(MediaStreamDescriptor{
      .stream_id = MediaStreamId{"stream_track_2"},
      .container_track_id = 2,
      .kind = MediaStreamKind::audio,
      .codec = MediaCodec::aac_lc,
      .codec_configuration_digest = audio_digest,
      .time_base = {.numerator = 1, .denominator = 48'000},
      .start = 28'944,
      .duration = 145'024,
      .format = AudioStreamFormat{.sample_rate_hz = 48'000, .channels = 1},
  });
  index.codec_configurations.push_back(MediaCodecConfiguration{
      .stream_id = MediaStreamId{"stream_track_1"},
      .sample_description_index = 1,
      .content_digest = video_digest,
      .bytes = video_configuration,
  });
  index.codec_configurations.push_back(MediaCodecConfiguration{
      .stream_id = MediaStreamId{"stream_track_2"},
      .sample_description_index = 1,
      .content_digest = audio_digest,
      .bytes = audio_configuration,
  });
  index.samples_decode_order.push_back(CompressedSample{
      .stream_id = MediaStreamId{"stream_track_1"},
      .sample_index = 0,
      .byte_offset = 0,
      .byte_size = 100,
      .presentation_timestamp = 15'990,
      .decode_timestamp = 15'990,
      .duration = 1000,
      .time_base = {.numerator = 1, .denominator = 30'000},
      .sync_sample = true,
      .sample_description_index = 1,
  });
  index.samples_decode_order.push_back(CompressedSample{
      .stream_id = MediaStreamId{"stream_track_2"},
      .sample_index = 0,
      .byte_offset = 100,
      .byte_size = 50,
      .presentation_timestamp = 28'944,
      .decode_timestamp = 28'944,
      .duration = 1024,
      .time_base = {.numerator = 1, .denominator = 48'000},
      .sync_sample = true,
      .sample_description_index = 1,
  });
  return index;
}

class FakeDemux final : public MediaDemuxPort {
 public:
  [[nodiscard]] MediaDemuxResult build_index(
      ImmutableCompressedSourceLease& source,
      const MediaCancellationToken*) override {
    ++calls;
    return {.state = MediaDemuxState::indexed,
            .index = indexed_source(source),
            .code = "RFX-MEDIA-INDEX-READY"};
  }

  std::uint64_t calls{0};
};

struct WorkspaceState final {
  std::uint64_t prepares{0};
  std::uint64_t commits{0};
  std::uint64_t retains{0};
  std::uint64_t rollbacks{0};
};

class FakePrepared final : public PreparedMediaAsset {
 public:
  FakePrepared(MediaAssetMaterializationReceipt receipt,
               std::shared_ptr<WorkspaceState> state,
               std::function<void()> on_commit)
      : receipt_(std::move(receipt)),
        state_(std::move(state)),
        on_commit_(std::move(on_commit)) {}
  ~FakePrepared() override {
    if (!retained_) ++state_->rollbacks;
  }

  [[nodiscard]] const MediaAssetMaterializationReceipt& receipt()
      const noexcept override {
    return receipt_;
  }
  [[nodiscard]] bool commit() noexcept override {
    ++state_->commits;
    committed_ = true;
    if (on_commit_) on_commit_();
    return true;
  }
  void retain() noexcept override {
    require(committed_, "uncommitted asset was retained");
    ++state_->retains;
    retained_ = true;
  }

 private:
  MediaAssetMaterializationReceipt receipt_;
  std::shared_ptr<WorkspaceState> state_;
  std::function<void()> on_commit_;
  bool committed_{false};
  bool retained_{false};
};

class FakeWorkspace final : public MediaImportWorkspacePort {
 public:
  explicit FakeWorkspace(std::shared_ptr<WorkspaceState> state)
      : state_(std::move(state)) {}

  [[nodiscard]] std::unique_ptr<PreparedMediaAsset> prepare_copy(
      const std::string&, const MediaAssetMaterializationReceipt& expected,
      ImmutableCompressedSourceLease& source,
      const MediaCancellationToken*) override {
    ++state_->prepares;
    require(source.content_digest() == expected.content_digest &&
                source.byte_size() == expected.byte_size,
            "workspace received the wrong immutable source");
    auto callback = std::move(on_next_commit);
    return std::make_unique<FakePrepared>(expected, state_,
                                          std::move(callback));
  }

  std::function<void()> on_next_commit;

 private:
  std::shared_ptr<WorkspaceState> state_;
};

class Progress final : public ImportVideoProgressPort {
 public:
  void report(const ImportVideoStage stage) noexcept override {
    stages.push_back(stage);
  }
  std::vector<ImportVideoStage> stages;
};

class Cancellation final : public MediaCancellationToken {
 public:
  [[nodiscard]] bool cancelled() const noexcept override { return value; }
  bool value{false};
};

class RejectAdmission final : public ProjectCandidateAdmissionPort {
 public:
  [[nodiscard]] CandidatePreparationResult prepare(
      const ProjectSnapshot&) override {
    return {
        .diagnostic = {.code = "RFX-TEST-REJECT",
                       .message = "test rejection",
                       .blocking = true},
    };
  }
};

[[nodiscard]] ProjectSnapshot blank_project() {
  auto result = create_initial_project({
      .display_name = "Import Test",
      .composition_preset_id = "reels-9x16",
      .resolution_id = "1080p",
      .frame_rate = 60,
      .duration_seconds = 1,
  });
  require(result.succeeded(), "could not create import test project");
  return std::move(*result.project);
}

[[nodiscard]] ImportVideoIntent intent(
    const ProjectSnapshot& project,
    const std::shared_ptr<ImmutableCompressedSourceLease>& source,
    const std::shared_ptr<const MediaCancellationToken>& cancellation = {}) {
  return ImportVideoIntent{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_import_001"},
          .expected_revision = project.revision_id,
          .idempotency_key = IdempotencyKey{"import-test-001"},
      },
      .source = source,
      .cancellation = cancellation,
      .original_display_name = "portrait-source.mp4",
      .timeline_start = 2'000'000'000ULL,
  };
}

}  // namespace

int main() {
  auto source = std::make_shared<MemorySource>();
  FakeDemux demux;
  MediaIndexingService indexing(demux, nullptr, 1);
  auto state = std::make_shared<WorkspaceState>();
  FakeWorkspace workspace(state);
  Progress progress;
  auto initial = blank_project();
  auto commands = create_application_host(initial);
  ImportVideoService service(*commands, indexing, workspace, &progress);

  const auto imported = service.execute(intent(initial, source));
  require(imported.status == ImportVideoStatus::accepted,
          imported.code + ": " + imported.diagnostic);
  require(imported.active_revision.value == initial.revision_id.value + 1,
          "ImportVideo did not publish exactly one revision");
  const auto active = commands->active_snapshot();
  require(active.assets.size() == 1 && active.media_sources.size() == 1 &&
              active.linked_imports.size() == 1 &&
              active.composition->video_clips.size() == 1 &&
              active.composition->audio_clips.size() == 1,
          "linked media truth was not published atomically");
  require(active.composition->video_clips.front().active_range.start ==
              2'000'000'000ULL &&
              active.composition->audio_clips.front().active_range.start ==
                  2'070'000'000ULL,
          "exact source start offset was not preserved on the Timeline");
  require(active.composition->video_clips.front().active_range.duration ==
              2'966'666'667ULL &&
              active.composition->audio_clips.front().active_range.duration ==
                  3'021'333'334ULL,
          "source duration was not deterministically rounded outward");
  require(active.media_sources.front().streams.front().stream_id.value !=
              "stream_track_1" &&
              active.media_sources.front().streams.at(1).stream_id.value !=
                  "stream_track_2",
          "container-local stream identity leaked into global project IDs");
  require(state->prepares == 1 && state->commits == 1 &&
              state->retains == 1 && state->rollbacks == 0,
          "successful asset transaction lifecycle is incorrect");
  require(progress.stages ==
              std::vector<ImportVideoStage>{
                  ImportVideoStage::validating, ImportVideoStage::indexing,
                  ImportVideoStage::preparing_revision,
                  ImportVideoStage::staging_asset,
                  ImportVideoStage::committing_asset,
                  ImportVideoStage::publishing_revision,
                  ImportVideoStage::completed},
          "import progress stages are incomplete or reordered");

  const auto replay = service.execute(intent(active, source));
  require(replay.status == ImportVideoStatus::replayed &&
              replay.active_revision == active.revision_id && demux.calls == 1 &&
              state->prepares == 1,
          "byte-identical duplicate import was not an idempotent replay");

  ExactAssetRelinkService relink(*commands, workspace);
  const auto relinked = relink.execute(RelinkExactAssetIntent{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_relink_001"},
          .expected_revision = active.revision_id,
          .idempotency_key = IdempotencyKey{"relink-test-001"},
      },
      .asset_id = active.assets.front().asset_id,
      .source = source,
  });
  require(relinked.succeeded() &&
              relinked.active_revision == active.revision_id &&
              commands->active_snapshot() == active && state->prepares == 2 &&
              state->commits == 2 && state->retains == 2 &&
              state->rollbacks == 0,
          "exact relink did not restore bytes without a semantic Revision");

  auto different_source = std::make_shared<MemorySource>(0x52);
  const auto mismatched = relink.execute(RelinkExactAssetIntent{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_relink_002"},
          .expected_revision = active.revision_id,
          .idempotency_key = IdempotencyKey{"relink-test-002"},
      },
      .asset_id = active.assets.front().asset_id,
      .source = different_source,
  });
  require(mismatched.status == RelinkExactAssetStatus::rejected &&
              mismatched.code == "RFX-MEDIA-RELINK-IDENTITY-MISMATCH" &&
              state->prepares == 2 && commands->active_snapshot() == active,
          "different bytes were not rejected before exact relink staging");

  const auto stale = relink.execute(RelinkExactAssetIntent{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_relink_003"},
          .expected_revision = initial.revision_id,
          .idempotency_key = IdempotencyKey{"relink-test-003"},
      },
      .asset_id = active.assets.front().asset_id,
      .source = source,
  });
  require(stale.status == RelinkExactAssetStatus::rejected &&
              stale.code == "RFX-MEDIA-RELINK-STALE-REVISION" &&
              state->prepares == 2,
          "stale exact relink intent reached the filesystem adapter");

  workspace.on_next_commit = [&] {
    const auto current = commands->active_snapshot();
    const auto changed = commands->submit(RenameProjectCommand{
        .envelope = CommandEnvelope{
            .command_id = CommandId{"cmd_relink_concurrent_change"},
            .expected_revision = current.revision_id,
            .idempotency_key =
                IdempotencyKey{"relink-concurrent-change"},
        },
        .requested_name = "Concurrent Revision",
    });
    require(changed.accepted(), "could not inject concurrent Revision");
  };
  const auto concurrent = relink.execute(RelinkExactAssetIntent{
      .envelope = CommandEnvelope{
          .command_id = CommandId{"cmd_relink_004"},
          .expected_revision = active.revision_id,
          .idempotency_key = IdempotencyKey{"relink-test-004"},
      },
      .asset_id = active.assets.front().asset_id,
      .source = source,
  });
  require(concurrent.status == RelinkExactAssetStatus::rejected &&
              concurrent.code == "RFX-MEDIA-RELINK-STALE-AFTER-COPY" &&
              state->prepares == 3 && state->commits == 3 &&
              state->retains == 2 && state->rollbacks == 1,
          "concurrent Revision did not roll back staged relink bytes");

  const auto after_concurrent = commands->active_snapshot();
  auto cancellation = std::make_shared<Cancellation>();
  cancellation->value = true;
  auto other_source = std::make_shared<MemorySource>();
  const auto cancelled_result =
      service.execute(intent(after_concurrent, other_source, cancellation));
  require(cancelled_result.status == ImportVideoStatus::cancelled &&
              commands->active_snapshot() == after_concurrent,
          "pre-cancelled import changed project truth");

  auto rejected_initial = blank_project();
  auto rejected_commands = create_application_host(rejected_initial);
  rejected_commands->set_candidate_admission_port(
      std::make_shared<RejectAdmission>());
  FakeDemux rejected_demux;
  MediaIndexingService rejected_indexing(rejected_demux, nullptr, 1);
  auto rejected_state = std::make_shared<WorkspaceState>();
  FakeWorkspace rejected_workspace(rejected_state);
  ImportVideoService rejected_service(*rejected_commands, rejected_indexing,
                                      rejected_workspace);
  const auto rejected_result =
      rejected_service.execute(intent(rejected_initial, source));
  require(rejected_result.status == ImportVideoStatus::rejected &&
              rejected_commands->active_snapshot() == rejected_initial &&
              rejected_state->commits == 1 &&
              rejected_state->retains == 0 &&
              rejected_state->rollbacks == 1,
          "rejected candidate did not preserve LKG and roll back the asset");

  std::cout << "atomic ImportVideo application transaction tests passed\n";
  return 0;
}
