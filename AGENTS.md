# ReFusion repository instructions

These rules apply to every human- or agent-authored change in this repository.

## Start protocol

Before editing:

1. On a physical Windows build/test host, first read
   `README_FOR_WINDOWS.md`; it is the execution and diagnostic handoff protocol,
   while the canonical authorities below remain unchanged.
2. Read `docs/status/CURRENT.md`.
3. Read every plan named by its `active_guardrails` field.
4. Read the active stage plan linked there.
5. Read only the ADRs, contracts, and work-package files linked by those plans.
6. Run `python3 tools/rfdev.py context` for the minimal context manifest.
7. Confirm the requested change belongs to the active gate and allowed paths.

Do not reread `docs/research/foundation-screening-draft.md` unless the active
plan explicitly routes to unresolved research.

## Sources of truth

- Delivery order: `docs/plans/MASTER_PLAN.md`
- Current gate and exact resume point: `docs/status/CURRENT.md`
- Product scope: `docs/product/PRODUCT_CONTRACT.md`
- Non-negotiable boundaries: `docs/architecture/INVARIANTS.md`
- Cross-platform branch, handoff and evidence workflow:
  `docs/architecture/CROSS_PLATFORM_POLICY.md#canonical-two-host-git-workflow`
- Visual/FX/plugin cross-platform remediation:
  `docs/plans/FIX_CROSS_PLATFORM_ARCHITECTURE.md`
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
- Every visual feature follows one route: portable descriptor/evaluator ->
  immutable `VisualRenderPlan` -> common Skia compositor -> thin native target.
  Metal/D3D12/Vulkan code owns resource mechanics only. Native renderer files
  call the common visual-program executor; they may not include project/compiler
  headers or switch on Layer/Mask/Blend/FX kinds.
- A candidate's semantics, assets, capabilities and render program are prepared
  before one accepted bundle is published to Timeline, Inspector and Canvas.
- Project time is exact rational/integer time. Decimal seconds and timeline
  pixels are presentation only.
- Portable project and Agent text uses locale-free canonical numbers,
  case-sensitive path-free ASCII IDs, and validated byte-preserved UTF-8. No
  platform may normalize or reinterpret accepted project truth implicitly.
- Derived/UI-authored pixel commands use the Core `1/1024 px` commit grid.
  Platform adapters and agents may not introduce another rounding policy.
- Qualified text uses project-relative digest-verified font bytes and the
  common Skia/HarfBuzz/ICU/FreeType policy. System fonts and implicit platform
  fallback are unqualified and cannot close cross-platform evidence.
- Production video paths are hardware-only and GPU-resident for decoded pixels.
  No software decode, CPU pixel map/readback/upload, or silent fallback.
- Unsupported capability fails closed with structured diagnostics while the
  Last-Known-Good accepted revision remains active.
- Preview and export share semantic evaluation; scheduling may differ.
- A capability is not complete until it has command, validation, persistence,
  migration, UI, agent introspection, preview, export, diagnostics, and tests.
- An FX/plugin is not cross-platform because it compiles once. It must retain
  one descriptor, state, migration, plan lowering, diagnostics and conformance
  corpus on every required lane; missing support fails closed.

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
- integer/native GPU device, queue, target or viewport handles in common
  contracts; use full-identity, lifetime-bearing opaque backend leases and keep
  private access inside the matching native adapter;
- floating tags, `main`, unpinned downloads, or network fetches during normal
  configure/build.

Qt event types may exist inside the Qt adapter only and must be converted at the
boundary to typed engine values. The ban is on crossing the boundary, not on
Qt's private implementation.

## Change discipline

- Work only inside the active work package unless the user explicitly changes
  program scope.
- Never develop concurrently on `main`. Classify work as shared feature,
  shared fix, native-platform fix or evidence, create the corresponding branch
  from the latest `origin/main`, and follow the canonical two-host Git workflow.
- Qt/QML UI, Timeline/Inspector projection and common engine changes are shared
  product work. They must not be appended to a platform evidence branch.
- Only one integration owner promotes a reviewed branch to `main`. Other hosts
  must fetch that exact commit before starting their next change or evidence
  run. No force push or history rewrite is permitted for an evidenced commit.
- Preserve user changes and avoid destructive Git operations.
- New architecture requires a proposed RFC or ADR; an agent may draft but not
  silently mark a product decision accepted.
- Existing visual-boundary violations are a ratcheted cleanup ledger: changes
  may only reduce them. Do not add FX semantics to a native backend, platform
  branches to the common compositor, or FX-specific defaults to Studio/QML.
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

`PLAN-XPLAT-FIX-001` `XPF-WP00A` is active in `architecture-check`. Its frozen
manifest is shrink-only: never add an allowance or increase a count. A green
check proves the guarded source boundary and current debt ceiling; physical
cross-platform visual qualification still requires the plan's device evidence.

## Definition of done

A change is done only when its declared outcome, tests, diagnostics, docs,
failure behavior, and evidence are complete. A stage is done only when every
exit criterion passes and its installable artifact and rollback evidence exist.
