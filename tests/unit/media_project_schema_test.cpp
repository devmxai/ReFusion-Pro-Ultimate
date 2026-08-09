#include "refusion/core/AgentIntrospection.hpp"
#include "refusion/core/ProjectDocument.hpp"
#include "refusion/core/ProjectRfx.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

using namespace refusion::core;

void require(const bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

[[nodiscard]] std::string digest(const char fill) {
  return "sha256:" + std::string(64, fill);
}

[[nodiscard]] ProjectSnapshot media_project() {
  ProjectSnapshot project{
      .project_id = ProjectId{"prj_media_roundtrip"},
      .revision_id = RevisionId{3},
      .display_name = "Portable Media Project",
      .composition = CompositionSnapshot{
          .composition_id = CompositionId{"cmp_main"},
          .display_name = "Main",
          .canvas = CanvasExtent{.width_pixels = 1080, .height_pixels = 1920},
          .frame_rate = RationalRate{.numerator = 30, .denominator = 1},
          .duration = 10'000'000'000ULL,
      },
  };
  project.assets.push_back(AssetRecord{
      .asset_id = AssetId{"ast_video_01"},
      .content_digest = digest('a'),
      .byte_size = 1'048'576,
      .media_kind = AssetMediaKind::video_container,
      .project_relative_original =
          "Assets/Media/ast_video_01/imported-video.mp4",
      .original_display_name = "imported-video.mp4",
      .provenance = AssetProvenance::imported_copy,
  });

  MediaSource source{
      .media_source_id = MediaSourceId{"media_video_01"},
      .asset_id = AssetId{"ast_video_01"},
      .media_index_contract_version = 1,
      .media_index_digest = digest('b'),
      .resolution = MediaResolutionState::resolved,
  };
  source.streams.push_back(MediaStreamDescriptor{
      .stream_id = MediaStreamId{"stream_video_01"},
      .container_track_id = 1,
      .kind = MediaStreamKind::video,
      .codec = MediaCodec::h264_avc,
      .codec_configuration_digest = digest('c'),
      .time_base = MediaTimeBase{.numerator = 1, .denominator = 90'000},
      .start = 15'990,
      .duration = 450'000,
      .format = VideoStreamFormat{
          .coded_extent =
              CanvasExtent{.width_pixels = 1080, .height_pixels = 1920},
          .display_extent =
              CanvasExtent{.width_pixels = 1080, .height_pixels = 1920},
          .presentation_rate =
              RationalRate{.numerator = 30'000, .denominator = 1'001},
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
  source.streams.push_back(MediaStreamDescriptor{
      .stream_id = MediaStreamId{"stream_audio_01"},
      .container_track_id = 2,
      .kind = MediaStreamKind::audio,
      .codec = MediaCodec::aac_lc,
      .codec_configuration_digest = digest('d'),
      .time_base = MediaTimeBase{.numerator = 1, .denominator = 48'000},
      .start = 8'528,
      .duration = 240'000,
      .format = AudioStreamFormat{.sample_rate_hz = 48'000, .channels = 2},
  });
  source.selected_video_stream = MediaStreamId{"stream_video_01"};
  source.selected_audio_stream = MediaStreamId{"stream_audio_01"};
  project.media_sources.push_back(source);

  project.composition->video_clips.push_back(VideoClipSnapshot{
      .video_clip_id = VideoClipId{"vclip_01"},
      .linked_import_id = LinkedImportId{"import_01"},
      .media_source_id = MediaSourceId{"media_video_01"},
      .stream_id = MediaStreamId{"stream_video_01"},
      .display_name = "Video",
      .active_range = TimeRangeNs{.start = 1'000'000'000ULL,
                                  .duration = 5'000'000'000ULL},
      .source_range = MediaTickRange{.start = 15'990, .duration = 450'000},
      .enabled = true,
      .locked = false,
  });
  project.composition->audio_clips.push_back(AudioClipSnapshot{
      .audio_clip_id = AudioClipId{"aclip_01"},
      .linked_import_id = LinkedImportId{"import_01"},
      .media_source_id = MediaSourceId{"media_video_01"},
      .stream_id = MediaStreamId{"stream_audio_01"},
      .display_name = "Audio",
      .active_range = TimeRangeNs{.start = 1'000'000'000ULL,
                                  .duration = 5'000'000'000ULL},
      .source_range = MediaTickRange{.start = 8'528, .duration = 240'000},
      .enabled = true,
      .locked = false,
      .gain = 0.75,
      .muted = false,
      .solo = false,
  });
  project.linked_imports.push_back(LinkedImport{
      .linked_import_id = LinkedImportId{"import_01"},
      .media_source_id = MediaSourceId{"media_video_01"},
      .video_clip_id = VideoClipId{"vclip_01"},
      .audio_clip_id = AudioClipId{"aclip_01"},
  });
  return project;
}

}  // namespace

int main() {
  using namespace refusion::core;

  const auto project = media_project();
  const auto validation = validate_project(project);
  require(validation.valid, validation.code + ": " + validation.message);

  const auto canonical = serialize_project_rfx(project);
  require(canonical.starts_with("rfx 6;"), "media project did not emit RFX6");
  require(canonical.find("Assets/Media/ast_video_01/imported-video.mp4") !=
              std::string::npos,
          "portable Originals path is absent");
  require(canonical.find("/Users/") == std::string::npos &&
              canonical.find(":\\\\") == std::string::npos,
          "canonical project leaked a host absolute path");

  const auto reopened = compile_project_rfx(canonical);
  require(reopened.succeeded(),
          reopened.diagnostics.empty() ? "RFX6 reopen failed"
                                       : reopened.diagnostics.front().message);
  require(*reopened.project == project, "RFX6 round-trip changed project truth");
  require(serialize_project_rfx(*reopened.project) == canonical,
          "RFX6 canonical bytes changed after reopen");
  const auto canonical_digest = project_snapshot_digest(project);
  require(project_snapshot_digest(*reopened.project) == canonical_digest,
          "RFX6 snapshot digest changed after reopen");
  require(canonical_digest == "rfx-project-fnv1a64:36eb03d9474173bc",
          "RFX6 canonical bytes differ from the AppleClang/MSVC receipt");
  std::cout << "canonical_digest=" << canonical_digest << '\n';
  const auto outline = agent_project_outline(*reopened.project);
  require(outline.assets.size() == 1 && outline.media_sources.size() == 1 &&
              outline.linked_imports.size() == 1 &&
              outline.video_clips.size() == 1 &&
              outline.audio_clips.size() == 1,
          "accepted media truth is absent from the shared Agent/UI projection");

  const auto& composition = *reopened.project->composition;
  require(composition.video_clips.front().active_range.start ==
              1'000'000'000ULL &&
              composition.video_clips.front().source_range.start == 15'990,
          "VideoClip project/source timing changed");
  require(composition.audio_clips.front().active_range.start ==
              1'000'000'000ULL &&
              composition.audio_clips.front().source_range.start == 8'528,
          "AudioClip project/source timing changed");

  auto missing = project;
  missing.media_sources.front().resolution = MediaResolutionState::missing;
  const auto missing_source = serialize_project_rfx(missing);
  const auto missing_reopened = compile_project_rfx(missing_source);
  require(missing_reopened.succeeded() &&
              missing_reopened.project->media_sources.front().resolution ==
                  MediaResolutionState::missing,
          "unresolved media state did not round-trip without data loss");

  auto absolute = project;
  absolute.assets.front().project_relative_original =
      "C:/Users/user/imported-video.mp4";
  require(!validate_project(absolute).valid,
          "absolute Windows media path was admitted");

  auto broken_link = project;
  broken_link.composition->video_clips.front().linked_import_id =
      LinkedImportId{"import_other"};
  require(!validate_project(broken_link).valid,
          "VideoClip outside its LinkedImport was admitted");

  std::cout << "media project schema tests passed\n";
  return 0;
}
