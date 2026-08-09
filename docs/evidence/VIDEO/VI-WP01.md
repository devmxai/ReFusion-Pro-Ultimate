---
id: EVID-VIDEO-VI-WP01
kind: portable-media-project-schema-evidence
plan: PLAN-VIDEO-VS-001
work_package: VI-WP01
status: macos-passed-msvc-pending
source_branch: feature/shared-video-import-v1
last_verified: 2026-08-09
---

# VI-WP01 — Portable Asset and media project schema

## Honest claim

The shared Core implementation is complete and physically verified with
AppleClang on macOS. The same conformance executable and fixed canonical digest
are present for MSVC, but have not been run on Windows yet. VI-WP01 therefore
is not promoted or closed across Desktop.

This package does not import a file, demux a container, decode video, draw a
Video frame, output Audio or expose the product Import UI. Those belong to
VI-WP02 onward.

## Implemented portable truth

- content-addressed `AssetRecord` with `Assets/Media/<AssetId>/...` relative
  Originals path and imported-copy provenance;
- `MediaSource` plus typed Video/Audio Stream descriptors, exact signed time
  base/start/duration and first-profile video/audio metadata;
- stable `LinkedImport`, `VideoClip` and `AudioClip` IDs;
- separate Video and Audio enable/lock state and Audio gain/mute/solo state;
- Clip composition ranges stored as integer project nanoseconds so an A/V
  offset between frames survives exactly; Composition/Layers remain frame-based;
- source in/out stored as signed-start/unsigned-duration media ticks;
- whole-project validation at compiler, serializer, Agent outline and Revision
  admission boundaries;
- RFX6 canonical persistence while RFX1–RFX5 remain readable and media-empty
  projects retain byte-compatible RFX5 canonical output;
- unresolved media resolution states round-trip without discarding Stream or
  Clip identity;
- shared Agent/Timeline/Inspector seed projection exposes the accepted Assets,
  Sources, links and Clips; Studio rendering remains a later package;
- CLI validate/outline projections consume the same accepted records.

## Conformance receipt

The `refusion.media_project_schema` test constructs one linked H.264/AAC
synthetic import, serializes it, reopens it and proves exact object equality,
canonical-byte stability and semantic-digest stability. It also verifies exact
project-nanosecond/source-tick timing, missing-source round-trip, absolute-path rejection and
broken-link rejection.

Fixed cross-toolchain receipt:

```text
rfx-project-fnv1a64:b4c2660862925e34
```

macOS commands and results:

```text
cmake --preset macos-core
cmake --build --preset macos-core --target refusion-media-project-schema-tests
ctest --test-dir out/build/macos-core --output-on-failure \
  -R 'refusion.media_project_schema|refusion.project_rfx|refusion.project_authority|refusion.agent_introspection'
4/4 passed
```

Existing RFX5 migration/round-trip tests remained green.

## Remaining exit evidence

On the Windows host, build and run `refusion-media-project-schema-tests` with
MSVC from this exact source checkpoint. The emitted digest must equal the fixed
receipt above. Any difference blocks VI-WP01 and must be fixed in shared Core;
no Windows-only project grammar or numeric patch is permitted.
