---
id: EVID-VIDEO-VI-WP03
kind: atomic-video-import-transaction-evidence
plan: PLAN-VIDEO-VS-001
work_package: VI-WP03
status: macos-4k-intake-corrected-msvc-reproduction-pending
source_branch: feature/shared-video-import-v1
last_verified: 2026-08-10
---

# VI-WP03 — Atomic import and workspace materialization

## Honest claim

The shared atomic ImportVideo transaction, Desktop Qt filesystem adapter,
product Studio file-portal client, typed CLI clients and exact-byte relink are
implemented and physically passed on macOS. Matching MSVC execution remains
pending; VI-WP03 is locally complete but cross-Desktop active, not closed.

No video decoder, decoded pixel, Canvas Video operation, Audio output or QML
project mutation was added by this package.

## Accepted transaction

```text
typed ImportVideoIntent
-> immutable path-free compressed-source lease
-> bounded shared MediaIndexingService / FFmpeg demux
-> normalized Asset + MediaSource + LinkedImport + Clips candidate
-> journaled staged copy and SHA-256/size verification
-> atomic asset rename
-> one ReplaceProject admission through ProjectCommandService
-> retain asset only after accepted Revision publication
```

The copied original is stored once at
`Assets/Media/<asset-id>/original.<mp4|mov>`. Video and optional Audio retain
separate Clip IDs and exact source tick ranges but share the same Asset,
MediaSource and LinkedImport. Container-local track IDs are rebased to
project-global stable Stream IDs; `container_track_id` remains unchanged.

RFX6 now writes Clip Timeline ranges as integer nanoseconds. This was required
by the physical corpus: its Audio starts 70 ms after Video, which is 4.2 frames
at 60 fps and cannot be represented by the initial frame-only spelling without
data loss. Composition duration remains frame-aligned; source timing remains in
the exact signed time-base/tick domain.

## Failure and recovery receipts

- cancellation before admission publishes no Asset or Revision;
- Runtime candidate rejection preserves Last-Known-Good and rolls back the
  committed original;
- byte-identical retry returns the existing linked import without another
  Revision, demux or copy;
- staging verifies both reported source identity and streamed SHA-256/size;
- interrupted journals remove unreferenced committed originals on next open;
- a journal never removes an original referenced by accepted project truth;
- canonical RFX6 contains no selected host path.

## Physical macOS corpus receipt

The integration test imports
`mp4-vfr-bframes-aac-offset/source.mp4` through the real Qt source lease, pinned
FFmpeg demux, materializer and Application authority. The accepted project has
one Asset, one MediaSource, one LinkedImport, one VideoClip and one AudioClip;
the copied byte count matches the source and canonical serialize/reopen is
identical.

```text
RFX6 canonical project receipt:
  rfx-project-fnv1a64:b4c2660862925e34

macos-demux:
  38/38 passed

macos-visual:
  62/62 passed
```

## Studio client and accepted-file receipt

Source commit `3df1719` connects the `VID` command surface to a native Qt
`FileDialog`, but QML submits only the selected local-file token. A bounded
worker owns source fingerprinting, shared FFmpeg indexing and streamed asset
materialization. Only the initial immutable snapshot and final
`ReplaceProjectCommand` cross back to the engine thread. The existing accepted
observer persists the resulting Revision, so UI commands, Agent file changes
and ImportVideo do not create separate project-file authorities.

`refusion.studio_media_import_bridge` opens the real VFR/B-frame/AAC-offset MP4
through that client, waits on Qt's event loop, observes exactly one accepted
Revision, and reopens the persisted `Project.rfx` with one Asset, MediaSource,
LinkedImport, VideoClip and AudioClip and no selected host path. The test first
reproduced a worker-stack overflow from a stack-resident 1 MiB copy buffer; the
buffer is now heap-resident and the same test passes. Studio exposes progress,
cancellation and the typed final diagnostic while remaining free of container,
copy or project authority.

## Exact relink receipt

Source commit `872d36b` adds `ExactAssetRelinkService` over the same immutable
source lease and rollback-owned materialization port. It admits only the
accepted `AssetId`, SHA-256 digest, byte size and canonical relative original.
Different bytes fail before staging and are explicitly reserved for the future
`ReplaceMediaSource` ChangeSet. Relink creates no project Revision and does not
touch MediaSource, Clip identity, ranges or stream timing.

The service rechecks accepted Asset truth after committing the staged file. A
concurrent Revision causes rollback rather than retaining bytes against stale
truth. The physical transaction test deletes the imported original, restores
it from the exact fixture and verifies that project Revision and canonical RFX6
remain unchanged.

## CLI parity and persistence receipt

Source commit `0d0badc` adds typed `commit import-video` and
`commit relink-exact` clients to the media-enabled product CLI. Capability
output reports both operations explicitly, while project Agent guidance directs
callers to these commands and forbids hand-authoring Asset, MediaSource,
LinkedImport or Clip records.

Both commands invoke the same `ImportVideoService` and
`ExactAssetRelinkService` used by Studio. Their project-file Revision proxy
first performs in-memory Application admission, then atomically CAS-persists
canonical RFX6; a persistence failure returns rejection so the prepared Asset
is rolled back rather than becoming orphan truth. Native Qt arguments and
UTF-8 filesystem paths preserve Unicode project and media names.

The physical CLI test imports the real VFR/B-frame/AAC-offset MP4, verifies the
copied Asset and path-free canonical project, deletes the copied original,
restores it with exact relink, proves `Project.rfx` is byte-unchanged by relink,
and validates canonical reopen.

```text
macos-core:
  37/37 passed

macos-visual:
  62/62 passed

architecture-check:
  132 source files; zero problems; zero visual-boundary debt
```

## Bounded 4K intake correction receipt

Physical use with a 2160x3840 H.264 High Level 5.1 MP4 exposed three intake
assumptions that were narrower than the declared Desktop product boundary:
Level 4.2/1080p admission, reliance on container-level transfer metadata and
rejection of ancillary Timecode as a second media track. ADR-0018 replaces
those assumptions once in the shared demux/profile contract; there is no
macOS-only exception.

The corrected profile admits SDR H.264 8-bit 4:2:0 through Level 5.2 and
3840x2160 coded pixels in either orientation. Missing codec parameters may be
recovered only from the same AVC SPS VUI. An unspecified transfer is normalized
to BT.709 only when video range, BT.709 primaries and BT.709 matrix are already
explicit; the normalization notice participates in the canonical MediaIndex
digest. Unencrypted ancillary `tmcd` is ignored, while unknown or encrypted
non-media streams still fail closed.

The immutable corpus now includes a six-frame 2160x3840, High Level 5.1,
video-only MP4 carrying Timecode and the bounded missing-transfer case:

```text
source sha256:
  d389c5fea01ca727b9ff967eac4955b04f643de58aed6e858a517ede5c088d2a

canonical MediaIndex digest:
  sha256:5d4aa884bcc403f7f2cecf6a711727c9103e444ad8ae2e820a89bfbb1f37248e

fixture corpus:
  7/7 receipts verified

macos-core:
  37/37 passed

macos-visual:
  63/63 passed

architecture-check:
  133 source files; zero problems; zero visual-boundary debt
```

The original 98,158,612-byte physical source that produced
`RFX-MEDIA-IMPORT-PROFILE-UNSUPPORTED` now passes the product CLI transaction,
copies exactly once, publishes canonical RFX6 with one VideoClip and reopens
successfully. It contains no Audio stream, so the accepted project correctly
contains no fabricated AudioClip. Studio now persists terminal accepted and
rejected import diagnostics into the project session journal for Agent
inspection.

This receipt proves import admission/materialization only. It does not claim a
decoded Canvas frame, hardware playback or Audio output; those remain VI-WP04
through VI-WP07 after the same intake receipt passes under MSVC.

## Remaining exit evidence

1. Run the same RFX6, service, real-copy, Studio client, CLI client, relink and
   interruption tests under MSVC from the exact shared source.
2. Record the Windows toolchain, pinned dependency materialization, source
   commit and physical receipts before declaring VI-WP01–03 closed.

VI-WP04 must not reinterpret Clip/source timing or bypass this transaction.
