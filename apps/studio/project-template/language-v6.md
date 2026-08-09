# RFX6 portable linked media declarations

Projects without media may remain canonical RFX5. The first accepted media
import moves canonical output to RFX6; RFX1–RFX5 remain readable migration
inputs. Agents do not invent media IDs, copy original bytes or write derived
indexes. Submit the typed `ImportVideo` operation and inspect the accepted IDs.

RFX6 adds one content-addressed original, one source and one linked import. A
container with Video and Audio creates two independently editable Clips that
share the same source and linked-import identity:

```rfx
asset id("ast_<engine-id>") digest("sha256:<bytes>") bytes(1234)
  kind(video_container)
  original("Assets/Media/ast_<engine-id>/original.mp4")
  name("source.mp4") provenance(imported_copy);

video_clip id("vclip_<engine-id>") link("import_<engine-id>")
  source("media_<engine-id>") stream("stream_<engine-id>_v")
  name("source.mp4") {
    range ns(0, 2966666667);
    source_range ticks(15990, 89000);
    enabled(true);
    locked(false);
  }
```

The complete `media_source` Stream descriptors, selected Stream IDs and
`linked_imports` block are engine-generated from the verified MediaIndex. Do
not copy a partial declaration from documentation into Project.rfx.

Clip project ranges use integer nanoseconds because a real Audio start offset
may fall between Composition frames. Source ranges retain the container's
signed start and duration in its exact Stream time base. Never round an A/V
offset to a frame, persist an absolute host path, duplicate the original for
Audio, or substitute missing media with black/silence.

The file under `Assets/Media/<asset-id>/` and `Project.rfx` are one import
transaction. If an import is interrupted, use the engine recovery/relink tools;
do not hand-edit cache or journal files.

## Typed Desktop media commands

First run `refusion-cli capabilities` and require
`media_commands.import_video=true`. Import with:

```text
refusion-cli commit import-video <Project.rfx> <source.mp4|mov> <time-ns>
```

This command fingerprints and indexes the source, copies the verified original,
publishes one linked Video/Audio Revision and atomically persists Project.rfx.
Do not construct the RFX6 declarations yourself.

If an accepted original is missing, require
`media_commands.relink_exact=true`, obtain the stable Asset ID from `outline`,
and restore only byte-identical content:

```text
refusion-cli commit relink-exact <Project.rfx> <asset-id> <source.mp4|mov>
```

Exact relink does not create a Revision or change Clips. An identity mismatch
must remain unresolved; never substitute a similar file. If either command is
reported unavailable, stop instead of editing project or cache files manually.
