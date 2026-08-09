---
id: PLAN-VIDEO-VS-001
kind: owner-authorized-cross-stage-vertical-slice-plan
status: approved
execution_state: VI-WP03-atomic-import-macos-passed-client-relink-msvc-pending
version: 7
master_plan: MP-001
guardrails: PLAN-XPLAT-FIX-001,ARCH-XPLAT-001
owner_role: media-import-and-playback
stage_routes:
  - G1
  - G2
  - G4
  - G5
canonical_for: first-video-import-hardware-playback-vertical-slice
last_verified: 2026-08-09
approved_by: product-owner-instruction-2026-08-09
---

# Video Import and Hardware Playback Vertical Slice

## Authority and purpose

This is the official execution and two-host handoff plan for ReFusion's first
real Video import and playback slice. It authorizes a bounded macOS-first
implementation now, followed by the matching Windows native adapter and a
same-commit dual-platform qualification.

This plan is subordinate to [`MP-001`](MASTER_PLAN.md), not a second Master
Plan and not a claim that G4 is complete. It deliberately pulls forward one
end-to-end risk/value slice while preserving the accepted stage meanings:

- G1 owns hardware decoder/native GPU surface proof;
- the accepted project/command/revision spine is reused and extended without a
  second authority;
- G4 remains the owner of complete media, audio, edit and export breadth;
- G5 remains the owner of the complete installable creator loop.

Every source change is governed by
[`PLAN-XPLAT-FIX-001`](FIX_CROSS_PLATFORM_ARCHITECTURE.md): shared media/time/
project/render meaning is written once; VideoToolbox/Metal and Media
Foundation/D3D code provide native mechanics only.

## Honest starting point

The repository already has:

- one portable strict H.264/NV12/SDR Rec.709 decode profile;
- exact compressed-sample timing, immutable decoded-surface leases, a bounded
  PTS queue and dependency-aware seek scheduler;
- Core-owned ProjectClock, transport epoch and Play/Pause/Seek commands;
- a physically proven macOS VideoToolbox -> CoreVideo -> Metal -> Skia path for
  a bounded long-GOP fixture with B-frames, variable durations and non-zero PTS;
- zero-CPU-video-pixel/fallback/readback/cross-adapter counters;
- a Windows media port that currently rejects all decode requests fail-closed.

The repository does not yet have:

- MP4/MOV import, container demux/index or a portable Media Asset record;
- Video/Audio content in the canonical Project document;
- linked but independently editable Video and Audio clips/tracks;
- a production `DrawVideoFrame` RenderPlan operation;
- real imported-video Canvas/Timeline playback;
- audio decode/output/waveform or an audio ClockSource;
- Media Foundation hardware decode and D3D native-surface import;
- save/reopen/relink, production seek/drift/performance or Export evidence.

Nothing in the current fixture proof may be described as product Video import.

## Declared first profile

The target profile is deliberately narrow. VI-WP00 must accept or narrow it
before implementation:

| Dimension | First target |
|---|---|
| Container | ISO BMFF MP4 and MOV subset, exact selection decided by ADR |
| Video | H.264/AVC, 8-bit 4:2:0, SDR Rec.709, hardware decode required |
| Native decoded format | NV12 or the accepted equivalent native two-plane surface |
| Timing | VFR, B-frames, non-zero source PTS/DTS, rational/integer project mapping |
| Audio target | AAC-LC mono/stereo, 44.1/48 kHz; final native route decided by ADR |
| Canvas | 1080x1920 Reels reference plus one 1920x1080 landscape fixture |
| Desktop lanes | macOS arm64 Metal and Windows x64 D3D12 |

HEVC, AV1, ProRes, HDR, alpha video, image sequences, proxies, multicam,
multichannel/surround audio, effects breadth, stabilization, transcription and
production Export are excluded from this first slice unless a later owner
revision adds a separate qualified package.

## One shared architecture

```text
System File Portal
      |
      v
ImportVideoIntent(path token, target composition, insertion time)
      |
      v
Application Import Planner
  validate -> fingerprint -> demux/index -> profile admission -> plan assets
      |
      v
Atomic ChangeSet
  AssetRecord + MediaSource + VideoClip + AudioClip + link identity
      |
      v
One accepted Project revision / Last-Known-Good
      |
      +--------------------------+
      |                          |
      v                          v
Timeline projection       Runtime media scheduler
Video + Audio rows        exact ProjectTime -> source PTS
                                 |
                                 v
                         Native decoder adapter
                      macOS                  Windows
                VideoToolbox            Media Foundation
                CoreVideo/Metal         D3D surface/D3D12
                      \                    /
                       v                  v
                       NativeVideoSurfaceLease
                                 |
                                 v
                  DrawVideoFrame VisualRenderPlan operation
                                 |
                                 v
                    common Skia compositing -> Canvas
```

The UI chooses a file and submits an intent. It owns no container, decoder,
queue, frame, transport clock, Canvas image or project mutation.

## Portable versus native ownership

### Implement once in shared code

- AssetId, content digest, relative workspace location and provenance;
- StreamId, source time base, PTS/DTS/duration, sync/dependency index and
  color/audio profile descriptors;
- VideoClip, AudioClip and stable linked-import identity;
- exact source-to-project time mapping, trim/in/out and selection policy;
- ImportVideo ChangeSet and all validation/diagnostics;
- project serialization, migration, save/reopen/relink and unresolved state;
- Timeline Video/Audio projections and Inspector descriptors;
- ProjectClock/transport requests and exact frame/audio sample scheduling;
- RenderPlan `DrawVideoFrame` semantics, bounds, color and opacity;
- capability admission, observability and zero-CPU-video-pixel policy;
- UI/CLI/future MCP intent parity.

### macOS-only mechanics

- security-scoped/native file portal token resolution inside the adapter;
- VideoToolbox session and hardware confirmation;
- CVPixelBuffer/CoreVideo lifetime and Metal texture-plane import;
- native audio decode/output endpoint selected by the accepted audio ADR;
- device generation, fences and Metal submission/present mechanics.

### Windows-only mechanics

- Win32/Qt file-portal token resolution inside the adapter;
- Media Foundation hardware MFT selection and rejection of software MFTs;
- adapter LUID equality, D3D surface lifetime and D3D11-on-12/direct D3D12
  route selected by physical evidence;
- native audio decode/WASAPI endpoint selected by the accepted audio ADR;
- D3D resource states, fences and DXGI presentation mechanics.

Windows code must not reimplement MP4 meaning, project schema, timestamp
selection, clip semantics, color policy, Timeline layout or RenderPlan meaning.

## Work-package order

```text
VI-WP00 Decisions/profile/fixtures
        |
        v
VI-WP01 Asset + media project schema
        |
        +--------------------+
        v                    v
VI-WP02 Demux/index      VI-WP03 Import transaction/recovery
        |                    |
        +---------+----------+
                  v
VI-WP04 Exact playback scheduler + DrawVideoFrame
                  |
        +---------+----------+
        v                    v
VI-WP05 macOS native     VI-WP06 Audio/linked-track spine
        |                    |
        +---------+----------+
                  v
VI-WP07 Studio import/Timeline/Canvas
                  |
                  v
VI-WP08 macOS end-to-end qualification
                  |
                  v
VI-HANDOFF-001 shared branch published
                  |
                  v
VI-WP09 Windows Media Foundation/D3D implementation
                  |
                  v
VI-WP10 same-commit macOS/Windows qualification and promotion
```

No later package may be used to bypass an undecided profile, non-atomic import
or missing project migration.

## VI-WP00 — Decisions, profile and immutable fixtures

**Owner:** shared media architecture. **Evidence:** `docs/evidence/VIDEO/VI-WP00.md`.

Deliver and accept ADRs for:

- container demux provider and its origin/revision/license/update policy;
- canonical packet/sample/index contract independent of the demux provider;
- native audio decode/output route and audio-master behavior;
- Asset/Media/VideoClip/AudioClip identity, linked-import and migration;
- color/rotation/pixel-aspect/aperture interpretation;
- copy-versus-reference import policy and relink behavior;
- first profile, fixtures, device tiers and explicit performance bounds;
- unsupported/malformed/encrypted media behavior;
- persistent Windows dependency workflow described later in this plan.

Fixtures must include VFR, B-frames, non-zero origin, rotation metadata,
audio start offset, deliberate unsupported codec and corrupt-container cases.
Every fixture must have immutable origin/license/digest.

**Exit:** all blocking decisions are accepted or the dependent package remains
blocked. No demux or project-format behavior may be guessed in platform code.

### VI-WP00 preparation state — 2026-08-09

The technical screening and formal decision package are now written:

| Area | Authority | Current state |
|---|---|---|
| Shared demux and canonical media index | [ADR-0012](../decisions/adrs/ADR-0012-shared-iso-bmff-demux-and-media-index.md) | accepted for engineering; release legal receipt remains required |
| Native audio pipeline and clock correlation | [ADR-0013](../decisions/adrs/ADR-0013-native-audio-pipeline-and-clock-correlation.md) | accepted |
| Asset/MediaSource/linked clips/import/relink | [ADR-0014](../decisions/adrs/ADR-0014-portable-media-assets-linked-clips-and-relink.md) | accepted |
| First media profile, metadata and corpus | [ADR-0015](../decisions/adrs/ADR-0015-first-desktop-video-import-profile-and-corpus.md) | accepted |
| Machine profile | [`desktop-video-import-v1.json`](../../contracts/media/desktop-video-import-v1.json) | accepted |
| Fixture corpus | [`video-import-v1-fixtures.json`](../../contracts/media/video-import-v1-fixtures.json) | accepted and materialized; 6/6 receipts verified |
| Demux dependency intake | [`ffmpeg-demux.md`](../../deps/intake/ffmpeg-demux.md) | official source verified; macOS demux-only build passed; Windows profile not run |
| Evidence/resume point | [`VI-WP00.md`](../evidence/VIDEO/VI-WP00.md) | passed |

The recommended route is one dynamic, demux-only, zero-decoder FFmpeg
`libavformat` adapter pinned to official `n8.0.3`, while video decode remains
VideoToolbox/Media Foundation and audio decode remains AudioToolbox/Media
Foundation. The shared media index, project identities, time/color/rotation
rules and failure codes are portable and provider-neutral.

The owner accepted all four decisions on 2026-08-09. The immutable six-row
corpus and its oracles are materialized, and the official pinned source builds
on macOS with only `MOV_DEMUXER` enabled. VI-WP00 is passed. Product import is
still unavailable because VI-WP02 and later implementation packages have not
completed.

## VI-WP01 — Portable Asset and media project schema

**Dependencies:** VI-WP00. **Evidence:** `docs/evidence/VIDEO/VI-WP01.md`.

Deliver:

- content-addressed AssetRecord and project-relative Originals location;
- MediaSource and StreamDescriptor with exact time bases and metadata;
- VideoClip and AudioClip content types with stable IDs and linked-import ID;
- independent enable/lock/edit state while retaining explicit linkage;
- Composition insertion time, source in/out and exact duration;
- canonical persistence, versioning, migration and unresolved round-trip;
- generated Inspector/Timeline/Agent descriptors;
- path-free portable project truth: no absolute macOS/Windows path in the
  canonical project.

This shared Asset spine is also the implementation seed later consumed by
[`UCAS-WP09A`](unified-creative-authoring/work-packages/UCAS-WP09A-assets-generation.md);
it must not be duplicated.

**Exit:** a synthetic Video+Audio import round-trips with identical stable IDs,
canonical bytes and semantic digest under AppleClang/MSVC conformance fixtures.

### VI-WP01 implementation state — 2026-08-09

The portable contract is implemented on `feature/shared-video-import-v1`:
RFX6 persists content-addressed Assets, exact Stream descriptors, one stable
LinkedImport and independent Video/Audio Clips without host paths. Project
compiler, serializer, Agent outline and Revision admission all use whole-project
validation. Media-empty projects continue to serialize as RFX5.

The AppleClang conformance test passes with fixed canonical receipt
`rfx-project-fnv1a64:b4c2660862925e34`; RFX5 regression tests remain green.
RFX6 Clip project ranges are integer nanoseconds while source ranges remain
exact Stream ticks; the initial frame-only Clip spelling remains a readable
migration input. This preserves real inter-frame Audio offsets without changing
the Composition frame clock.
The matching MSVC execution is pending, so VI-WP01 is locally code-complete but
not cross-Desktop closed. See [`EVID-VIDEO-VI-WP01`](../evidence/VIDEO/VI-WP01.md).
No platform-specific schema fork is permitted if MSVC differs.

## VI-WP02 — Container demux and exact media index

**Dependencies:** VI-WP00–01. **Evidence:** `docs/evidence/VIDEO/VI-WP02.md`.

Deliver one portable packet/index projection regardless of the admitted demux
provider:

- tracks, codec configuration, PTS, DTS, duration, time base and sync flags;
- dependency-window inputs, sample byte ranges and immutable source identity;
- rotation/aperture/pixel-aspect/color/audio metadata;
- deterministic validation and diagnostic codes;
- bounded asynchronous indexing, cancellation and cache invalidation;
- no video decode, pixel conversion or renderer authority in demux code.

CPU metadata/container parsing is allowed; CPU video-pixel materialization and
software video decode remain forbidden.

**Exit:** the fixture index is canonical across desktop toolchains and the
existing bounded HardwareVideoDecodeScheduler consumes it without platform
container parsers.

### VI-WP02 implementation state — 2026-08-09

The shared package is implemented and verified on macOS. `MediaIndex v1`
contains exact selected Streams, codec configuration bytes, decode-order sample
ranges/timestamps/dependencies and immutable source identity. The pinned
demux-only FFmpeg adapter obtains that contract through bounded path-free AVIO.
A bounded asynchronous indexing service supplies cancellation and validated
content-addressed cache reuse. One common projection derives the scheduler's
decode order and stable presentation-frame identity from this same index; no
Metal/D3D container parser is involved.

The three admitted fixture index digests and the synthetic contract digest are
fixed in [`EVID-VIDEO-VI-WP02`](../evidence/VIDEO/VI-WP02.md). All positive and
negative corpus cases pass locally, including unsupported-container,
cancellation, corrupt cache and capacity tests. The matching MSVC run remains
pending, so VI-WP02 is macOS-passed but not cross-Desktop closed. Product import
and Video decoding remain unavailable until later packages.

## VI-WP03 — Atomic import, workspace materialization and recovery

**Dependencies:** VI-WP01–02. **Evidence:** `docs/evidence/VIDEO/VI-WP03.md`.

Deliver:

- typed ImportVideo intent and normalized atomic ChangeSet;
- staged copy/reference verification, content digest and collision-safe naming;
- one accepted revision containing Asset, VideoClip, AudioClip and root order;
- import progress/cancellation without partial project publication;
- journal/recovery, duplicate import/idempotency and clean rollback;
- missing/moved source diagnostics and explicit relink command;
- file candidate/UI/CLI parity through the one admission authority.

**Exit:** interruption at every materialization phase produces the previous or
next complete project only; invalid media preserves Last-Known-Good.

### VI-WP03 implementation state — 2026-08-09

The shared `ImportVideoService` and Desktop Qt filesystem adapter are now
implemented and physically passed on macOS. One typed intent indexes the
path-free immutable source, derives collision-safe project-global media IDs,
stages and SHA-256 verifies one copied original, and publishes Asset,
MediaSource, LinkedImport, VideoClip and optional AudioClip in exactly one
Revision. Runtime rejection rolls back the committed asset; a byte-identical
retry is idempotent; incomplete transaction journals remove unreferenced
originals on reopen. New workspaces expose `Assets/Media`, RFX6 in
`refusion.lock`, and a bundled Agent language-v6 guide.

The physical positive corpus test imports the VFR/B-frame/AAC-offset fixture,
preserves its 70 ms Audio offset and exact source ticks, materializes the
verified original and reopens canonical RFX6 without any host path. macOS
Visual now includes the same demux target used by product Studio; macOS Visual
passes 60/60 and macOS Demux 38/38.

VI-WP03 is not closed yet: the system file-portal/UI/CLI client, typed exact
relink operation and matching MSVC transaction receipt remain. These must use
this service/port and may not introduce another import or persistence path. See
[`EVID-VIDEO-VI-WP03`](../evidence/VIDEO/VI-WP03.md).

## VI-WP04 — Exact playback scheduler and RenderPlan video operation

**Dependencies:** VI-WP01–03. **Evidence:** `docs/evidence/VIDEO/VI-WP04.md`.

Deliver:

- exact ProjectTime -> source PTS mapping for forward play, pause and seek;
- bounded dependency-aware decode windows and stale epoch/device rejection;
- immutable selected-surface lease at one EvaluationStamp;
- `DrawVideoFrame` RenderPlan operation with destination/source rectangles,
  transform, opacity, clip, color profile and resource identity;
- common Skia execution that receives one selected native image bridge result;
- preview/offline semantic seed without implementing product Export;
- queue, deadline, drop/repeat and A/V drift observability.

The current G1 decoded-video overlay is quarantined and must not become product
compositing.

**Exit:** exact seeks select the expected VFR/B-frame source frames; failure or
stale output cannot advance the accepted revision/Canvas.

## VI-WP05 — macOS VideoToolbox/Metal production adapter

**Dependencies:** VI-WP00–04. **Evidence:** `docs/evidence/VIDEO/VI-WP05.md`.

Promote the bounded Apple proof behind the production media ports:

- consume demuxed codec configuration and compressed samples;
- require and confirm VideoToolbox hardware acceleration;
- own CVPixelBuffer and CVMetalTexture lifetimes through opaque leases;
- bind NV12 planes on the engine Metal device/generation;
- enforce common color/profile metadata and explicit synchronization;
- remove product reliance on the fixture Annex-B parser and overlay helper;
- retain zero software-decoder, CPU video-pixel, conversion, readback and
  cross-adapter counters.

**Exit:** a real imported MP4/MOV fixture displays, plays, pauses and seeks on
Canvas through the product Project/RenderPlan path.

## VI-WP06 — Linked Audio track, waveform and clock-source spine

**Dependencies:** VI-WP00–04. **Evidence:** `docs/evidence/VIDEO/VI-WP06.md`.

Deliver:

- independently addressable AudioClip linked to the imported VideoClip;
- exact sample rate/channel/start-offset/duration mapping;
- native decode/output adapter, bounded PCM/audio buffers and real waveform
  artifact; the no-CPU rule applies to video pixels, not audio samples;
- mute/gain/solo seed and independent audio edit ownership;
- qualified audio endpoint as ClockSource during forward playback, while Core
  ProjectClock remains canonical project-time authority;
- drift/discontinuity/underflow diagnostics and pause/seek resynchronization.

Noise removal, enhancement and the full Audio Studio are later packages.

**Exit:** Video and Audio tracks are visually separate, remain linked by ID,
play in sync, can be independently selected/disabled and survive save/reopen.

## VI-WP07 — Studio import, Timeline and Canvas integration

**Dependencies:** VI-WP03–06. **Evidence:** `docs/evidence/VIDEO/VI-WP07.md`.

Deliver:

- Video toolbar action opens the system file portal and submits ImportVideo;
- progress and typed rejection diagnostics without UI authority;
- one Video Timeline row and one linked Audio row with real waveform;
- track extents derived from exact clip time, not UI pixels;
- one playhead/transport projection from ProjectClock;
- Canvas renders the selected frame through DrawVideoFrame;
- Inspector selects Video or Audio descriptors without hidden duplicate Layers;
- project close/reopen/relink and invalid-edit Last-Known-Good behavior.

**Exit:** the user can create a blank Reels project, import one qualified file,
see two tracks, play/pause/seek, select either track and reopen the same project.

## VI-WP08 — macOS end-to-end qualification

**Dependencies:** VI-WP00–07. **Evidence:** `docs/evidence/VIDEO/VI-WP08.md`.

On physical macOS, qualify the declared fixture corpus and record separately:

- canonical import/project/index/RenderPlan digests;
- hardware decode confirmation and native surface identity;
- frame selection, A/V start offset, drift, pause/seek and end-of-stream;
- queue/memory/submission/presentation latency and missed deadlines;
- zero forbidden video-pixel/fallback/readback/conversion counters;
- save/reopen/relink, corrupt/unsupported/cancel and device-loss recovery;
- Studio user-visible demonstration and exact source commit.

Passing VI-WP08 authorizes publication of the shared feature branch for Windows
work. It does not authorize merge to `main` or claim Windows parity.

## VI-HANDOFF-001 — Git publication after macOS passes

### macOS integration owner

Start from a clean current `origin/main`:

```bash
git fetch origin
git switch main
git pull --ff-only
git switch -c feature/shared-video-import-v1
```

Implement VI-WP00–08 in reviewable commits. After the complete macOS receipt:

```bash
git status --short
git push -u origin feature/shared-video-import-v1
```

Do not merge or push this incomplete cross-platform feature to `main` yet.
Generated dependencies, media caches, user media and `out/` are never committed.
Only small licensed fixture assets explicitly admitted by VI-WP00 may enter Git.

## Persistent Windows dependency rule

The current repository bootstrap stores verified dependency sources and Skia
build artifacts beneath the active checkout's `out/`. It deliberately rejects
an arbitrary external cache. Therefore the currently admitted no-redownload
workflow is:

1. keep one canonical Windows checkout in a stable path;
2. switch/fetch branches inside that checkout instead of cloning again;
3. preserve `out/deps-src`, `out/deps-build/skia` and the installed Qt kit;
4. never use `-FreshDependencies` or `--fresh` during normal branch updates;
5. run `python tools/bootstrap.py verify-skia-materialization` before building;
6. if verification fails, stop and diagnose the exact drift—do not delete the
   cache, reclone, copy another checkout or silently redownload.

An external content-addressed dependency store/junction is not accepted until a
separate reviewed bootstrap change supports and verifies it. A Windows-only
local workaround is not project policy.

## VI-WP09 — Windows Agent handoff and native implementation

The Agent on Windows must read this complete plan, `AGENTS.md`,
`README_FOR_WINDOWS.md`, `CURRENT.md` and active guardrails before editing.

### Obtain the exact macOS-qualified branch without a new clone

From the existing persistent checkout:

```powershell
git status --short
git fetch origin --prune
git switch feature/shared-video-import-v1
git pull --ff-only origin feature/shared-video-import-v1
git rev-parse HEAD
python tools/bootstrap.py verify-skia-materialization
git switch -c feature/windows-video-import-v1
```

If `git status --short` is not empty, stop and preserve the existing work. Do
not reset, delete `out/` or create another clone to hide the condition.

### Allowed Windows implementation scope

- Media Foundation capability/MFT/session/sample/native-surface adapter;
- D3D surface lease and Skia D3D native-video image bridge;
- WASAPI/native audio endpoint adapter selected by the shared contract;
- Windows file-portal/path-token adapter;
- Windows CMake/runtime deployment and physical tests;
- Windows evidence and causal shared fixes when required.

### Forbidden Windows scope

- changing VideoClip/AudioClip/project/import/timing/RenderPlan meaning only for
  Windows;
- copying the Apple Annex-B parser or creating a Windows container model;
- software MFT, WARP, CPU video-pixel lock/map/conversion/upload or silent
  fallback;
- D3D11/D3D12 cross-adapter use without equality/synchronization evidence;
- editing QML/Timeline behavior as a Windows-only feature;
- pushing directly to `main` or weakening tests/tolerances.

### Required Windows execution

1. run Core before media/Graphics/Visual;
2. implement Media Foundation behind the existing portable ports;
3. add real long-GOP/VFR/seek and native-surface tests;
4. run the same import/project/index/RenderPlan conformance corpus;
5. run Studio with the same qualified media file and copied macOS project;
6. record hardware decoder/MFT, adapter LUID, native surface, fence, A/V sync,
   performance and zero forbidden counters;
7. update `docs/evidence/VIDEO/VI-WP09-windows.md` with exact commit/commands;
8. push only the Windows feature branch:

```powershell
git status --short
git add <reviewed source/tests/evidence files>
git commit -m "media: implement Windows hardware video import path"
git push -u origin feature/windows-video-import-v1
```

Large logs, build output, downloaded weights and user media stay under `out/`
or the local workspace and are not committed.

## VI-WP10 — Same-commit reconciliation and promotion

The macOS integration owner fetches and reviews the Windows branch. Shared
semantic fixes use a shared fix commit; native-only fixes stay in the Windows
adapter. Then create one integration head containing both platform routes.

Both hosts must fetch and test the **exact same integration commit**:

- macOS: Core, Graphics/Visual, media corpus and physical Studio import/playback;
- Windows: Core, Graphics/Visual, Media Foundation corpus and physical Studio;
- same macOS-created project opens on Windows with equal stable IDs, canonical
  project/index meaning and RenderPlan digest;
- same Windows-created project opens on macOS;
- calibrated Canvas comparison uses the accepted tolerance and unchanged
  assets/fonts/color profile;
- A/V sync, seek, device loss, cancellation, save/reopen and unsupported corpus
  pass on both;
- no platform reports a forbidden CPU video-pixel or fallback counter.

Only after both receipts bind the exact integration head may the macOS
integration owner promote it to `main` through the protected review path. The
promotion must not change the qualified source tree. Windows never pushes
directly to `main`.

## Exit definition

This vertical slice is complete only for the declared profile when:

```text
Create blank real project
-> Import qualified MP4/MOV
-> materialize verified asset and exact media index
-> publish linked VideoClip + AudioClip in one revision
-> show Video and waveform Audio tracks
-> hardware-decode video into same-device GPU surfaces
-> play/pause/seek with exact ProjectClock and bounded A/V sync
-> render through DrawVideoFrame/common Skia
-> save/reopen/relink/recover
-> pass same-commit macOS + Windows qualification
```

The result does not imply every codec/container, full Audio Studio, editing
breadth, Export, mobile playback or G4 exit.

## Stop conditions

Stop the active package and preserve Last-Known-Good if any implementation
requires:

- a UI/file watcher/platform adapter as project or time authority;
- platform-specific project, clip, index or RenderPlan semantics;
- software video decode or CPU video-pixel transfer;
- hidden fallback, WARP, cross-adapter copy or unbounded queue;
- decimal/UI-pixel time truth, missing PTS/DTS or guessed A/V offsets;
- partial import publication or absolute host paths in canonical project state;
- two independent Video/Audio imports without stable linkage;
- unlicensed/unpinned demux or fixture dependencies;
- merging to `main` before exact-commit macOS and Windows evidence.

## Exact next action

Keep all work on `feature/shared-video-import-v1`. Complete VI-WP03 through the
single `ImportVideoService`: connect the Desktop file-portal/Studio client,
persist accepted RFX6 through the existing observer transaction, and add typed
exact-byte relink plus client-parity receipts. Do not put file copying,
MediaIndex construction or project mutation in QML. In parallel, Windows must
run the updated VI-WP01–03 MSVC conformance/transaction tests and reproduce the
new RFX6 receipt before these packages may close. VI-WP04 exact playback and
`DrawVideoFrame` start only after this VI-WP03 boundary is green.
