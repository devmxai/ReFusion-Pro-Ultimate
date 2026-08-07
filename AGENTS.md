# ReFusion repository instructions

These rules apply to every human- or agent-authored change in this repository.

## Start protocol

Before editing:

1. Read `docs/status/CURRENT.md`.
2. Read the active stage plan linked there.
3. Read only the ADRs, contracts, and work-package files linked by that plan.
4. Run `python3 tools/rfdev.py context` for the minimal context manifest.
5. Confirm the requested change belongs to the active gate and allowed paths.

Do not reread `docs/research/foundation-screening-draft.md` unless the active
plan explicitly routes to unresolved research.

## Sources of truth

- Delivery order: `docs/plans/MASTER_PLAN.md`
- Current gate and exact resume point: `docs/status/CURRENT.md`
- Product scope: `docs/product/PRODUCT_CONTRACT.md`
- Non-negotiable boundaries: `docs/architecture/INVARIANTS.md`
- Accepted architectural decisions: `docs/decisions/adrs/`
- Machine-readable contracts: `contracts/`
- Qualification evidence: `docs/evidence/`
- Research and rejected alternatives: `docs/research/`

Never create a second master plan, status file, capability catalog, or project
truth. A summary must link to the canonical source instead of copying it.

## Core invariants

- UI is commands out and immutable snapshots in. It owns no project, timeline,
  transport, decoder, GPU-frame, render-graph, or accepted-revision authority.
- UI and agent edits converge through one typed Command/ChangeSet/Revision path.
- Core/project contracts expose no Qt, Skia, OS, codec, or native GPU types.
- Platform adapters implement engine-owned ports; they do not redefine project
  semantics.
- Project time is exact rational/integer time. Decimal seconds and timeline
  pixels are presentation only.
- Production video paths are hardware-only and GPU-resident for decoded pixels.
  No software decode, CPU pixel map/readback/upload, or silent fallback.
- Unsupported capability fails closed with structured diagnostics while the
  Last-Known-Good accepted revision remains active.
- Preview and export share semantic evaluation; scheduling may differ.
- A capability is not complete until it has command, validation, persistence,
  migration, UI, agent introspection, preview, export, diagnostics, and tests.

Read `docs/architecture/INVARIANTS.md` before touching boundaries, media,
rendering, time, persistence, extensions, or Studio integration.

## Forbidden production paths

Do not introduce these into ReFusion-owned project Canvas, media, render, or
export targets:

- `QImage`, `QPixmap`, `QPainter`, `QPrinter`, `QVideoFrame`, `QVideoSink`,
  `QMediaPlayer`, `QQuickPaintedItem`, or Qt Multimedia playback;
- CPU software video decoders, `sws_scale`, CPU YUV/RGB conversion, CPU RGBA
  frame materialization, GPU readback, native buffer lock/map, or WARP;
- UI timers, QML animation clocks, repaint callbacks, or file-watch events as
  project transport/revision authority;
- raw `SkImage`, `AVFrame`, `CVPixelBuffer`, D3D, Metal, Vulkan, Android, or Qt
  handles in serialized project state or common contracts;
- floating tags, `main`, unpinned downloads, or network fetches during normal
  configure/build.

Qt event types may exist inside the Qt adapter only and must be converted at the
boundary to typed engine values. The ban is on crossing the boundary, not on
Qt's private implementation.

## Change discipline

- Work only inside the active work package unless the user explicitly changes
  program scope.
- Preserve user changes and avoid destructive Git operations.
- New architecture requires a proposed RFC or ADR; an agent may draft but not
  silently mark a product decision accepted.
- Generated files are never hand-edited.
- Dependencies require official origin, immutable revision, license, build
  options, owner, update policy, and qualification evidence.
- Update `docs/status/CURRENT.md` and create a checkpoint when a work package
  reaches a real resume point. Do not report percentage-complete estimates.

## Required checks

Run the smallest relevant set and record exact results:

```bash
python3 tools/rfdev.py docs-doctor
python3 tools/rfdev.py architecture-check
cmake --workflow --preset macos-core
```

Studio, GPU, media, packaging, signing, and device changes require the specific
stage-plan gates and real-platform evidence. A compile-only result is never
runtime or cross-platform qualification.

## Definition of done

A change is done only when its declared outcome, tests, diagnostics, docs,
failure behavior, and evidence are complete. A stage is done only when every
exit criterion passes and its installable artifact and rollback evidence exist.

