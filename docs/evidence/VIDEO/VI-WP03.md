---
id: EVID-VIDEO-VI-WP03
kind: atomic-video-import-transaction-evidence
plan: PLAN-VIDEO-VS-001
work_package: VI-WP03
status: macos-studio-client-exact-relink-passed-cli-msvc-pending
source_branch: feature/shared-video-import-v1
last_verified: 2026-08-09
---

# VI-WP03 — Atomic import and workspace materialization

## Honest claim

The shared atomic ImportVideo transaction, Desktop Qt filesystem adapter and
product Studio file-portal client and typed exact-byte relink are implemented
and physically passed on macOS. CLI client parity and matching MSVC execution
remain pending; VI-WP03 is therefore active, not closed.

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
  61/61 passed
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
remain unchanged. Full macOS Visual remains 61/61.

## Remaining exit evidence

1. Add the CLI client over the same shared service and record UI/file/CLI
   parity; future MCP calls the same typed intent.
2. Run the same RFX6, service, real-copy, client, relink and interruption tests under
   MSVC.

VI-WP04 must not reinterpret Clip/source timing or bypass this transaction.
