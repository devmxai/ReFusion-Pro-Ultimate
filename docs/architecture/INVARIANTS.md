---
id: ARCH-INV-001
kind: invariants
status: accepted
owner_role: principal-architecture
canonical_for: architecture-invariants
last_verified: 2026-08-07
accepted_by: product-owner-user-instruction-2026-08-07
---

# Architecture invariants

## Authority

1. `RevisionAuthority` is the only service that accepts project revisions.
2. UI, files, CLI, and MCP are clients; none may publish partial state.
3. Timeline, Canvas, Inspector, Console, audio, preview, and export consume one
   accepted revision and a compatible `EvaluationStamp`.
4. Stale or invalid candidates never replace Last-Known-Good.

## Portable semantic core

- Core owns stable typed IDs, exact time, project schemas, commands, revisions,
  diagnostics, capability registry, evaluator semantics, and migrations.
- Core public APIs contain no Qt, Skia, codec, operating-system, or native GPU
  objects. Platform-specific objects are opaque, ephemeral leases.
- Platform implementations may differ for window/surface, GPU, media, audio,
  filesystem portal, lifecycle, signing, and store integration only; they may
  not redefine project meaning.
- Every shared contract is designed for macOS, Windows, iOS, and Android from
  its first revision. Absence of a device may change the evidence state to
  `not-run`; it may never authorize a one-platform semantic shortcut.

## UI boundary

Qt/QML may own windows, controls, panels, input collection, view models, and
diagnostic presentation. It sends typed intents and displays immutable engine
snapshots. It owns no project model, decoder, transport clock, frame cache,
render graph, project-viewport swapchain, or acceptance queue. Qt may own the
shell window/control rendering required for Studio itself; that authority never
extends to project Canvas presentation or engine frames.

Qt values such as `QPointF` may exist only while adapting an event; the adapter
must immediately convert them to explicit engine coordinate/unit types.

## Video and GPU policy

The production invariant is **Zero CPU Pixel Transfer**, not the impossible
claim that the CPU is never used. CPU orchestration may parse compressed data,
metadata, commands, files, diagnostics, glyph preparation, and audio DSP.

Decoded production video pixels must remain on GPU/native hardware surfaces
through preview, compositing, and hardware export. Forbidden paths include
software decode, CPU frame maps/locks, CPU YUV/RGB conversion, CPU RGBA upload,
GPU readback, WARP, and silent fallback. Same-adapter GPU copies/conversions are
allowed only when explicit, bounded, synchronized, instrumented, and qualified.

One `GpuDeviceService` selects and owns the physical adapter/device/queues.
Skia and media bridges borrow its admitted resources. Skia is a 2D/text content
producer inside the render graph, not the project schema or scheduler.

## Time and coordinates

- Project time uses checked rational/integer domains and half-open ranges.
- VFR frame selection uses presentation timestamps/indexes, never average FPS.
- Audio addressing uses integer sample domains.
- `1 Composition Unit = 1 Composition Pixel`; viewport zoom/DPI are not saved.
- The engine provides measurement/time-resolution tools; agents never guess.

## Product and extension safety

- Preview/export share semantic evaluation.
- Capability claims require a qualified platform/profile state.
- Native extensions run out of process and cannot enter the UI, command,
  revision, or main render process. Public native ABI is post-v1.
- Mobile does not download executable native plugins; only packaged or
  declarative content is eligible, subject to store policy.
