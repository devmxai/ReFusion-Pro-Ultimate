---
id: EVID-VIDEO-VI-WP03
kind: atomic-video-import-transaction-evidence
plan: PLAN-VIDEO-VS-001
work_package: VI-WP03
status: macos-shared-transaction-passed-client-relink-msvc-pending
source_branch: feature/shared-video-import-v1
last_verified: 2026-08-09
---

# VI-WP03 — Atomic import and workspace materialization

## Honest claim

The shared atomic ImportVideo transaction and Desktop Qt filesystem adapter are
implemented and physically passed on macOS. Product Studio file-portal wiring,
typed exact-byte relink and matching MSVC execution remain pending; VI-WP03 is
therefore active, not closed.

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
  60/60 passed
```

## Remaining exit evidence

1. Connect the Studio file portal and progress/diagnostic projection to this
   exact service, with no QML filesystem or project authority.
2. Add typed exact-byte relink through the same materialization contract.
3. Prove accepted Project.rfx persistence/save/reopen in the product client.
4. Run the same RFX6, service, real-copy and interruption tests under MSVC.
5. Record client parity for UI/file/CLI; future MCP calls the same typed intent.

VI-WP04 must not reinterpret Clip/source timing or bypass this transaction.
