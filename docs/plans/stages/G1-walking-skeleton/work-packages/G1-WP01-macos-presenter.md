---
id: G1-WP01
kind: work-package
status: active
gate: G1
owner_role: apple-gpu
evidence: docs/evidence/G1/G1-WP01.md
---

# Outcome

Render a Skia Text/Shape fixture into an engine-owned CAMetalLayer presenter
embedded in the Qt shell, using the admitted Metal device and explicit queue,
resource and completion-fence leases.

# Qualified device

Initial lab device `MAC-LAB-001`: MacBookAir10,1, Apple M1, 7-core GPU, 16 GB,
Metal 4. Additional Desktop-v1 tiers require separate evidence.

# Required proof

- Qt owns shell controls only; Engine owns project viewport presentation.
- One physical adapter/device generation; zero competing Skia device/queue.
- Draw, submit, present and 10,000-frame soak with zero validation failure.
- Resize, occlusion, sleep/wake and device-loss/fail-closed diagnostics.
- Arabic/Latin fixture geometry and preview/export semantic digest recorded.

# Cross-platform constraint

The presenter API, fixture description, commands, diagnostics and acceptance
metrics are portable C++ contracts shared with G1-WP02. Only CAMetalLayer/Metal
implementation belongs to the Apple adapter. No Apple conditional or native
type may enter Core, Application, Runtime public contracts, Studio commands, or
project state. Windows remains `not-run`, not waived.

# Kill criteria

Reject any permanent CPU project-pixel bridge, UI-owned project frame, ambiguous
fence/lifetime, uninstrumented copy, or hidden fallback.

# Current delivery

The first visual slice is code-complete at
`4c91df7f6abf0734a47cfb549a726d15ef35281e`: portable presentation/session
contracts, engine composition authority, CAMetalLayer presenter, Skia
HarfBuzz/ICU Arabic+Latin fixture, Qt native host, resize/visibility checks and
the 10,000-frame zero-CPU-transfer soak all passed. Keep this package `active`
until the remaining physical occlusion, sleep/wake and device-loss receipts are
retained.

Source commit `b850a84ce8aff860bf8aac786440b396eb77024e` replaced the
hard-coded visual fixture in the walking application with a strictly validated,
file-backed 30-second Reels Composition. Portable Core owns its IDs, Canvas,
half-open integer time ranges, Layers and scalar keyframes; Runtime owns the
30 fps looping clock; Skia evaluates the accepted Composition snapshot. This is
a bounded project-open seed under proposed ADR-0008, not a stable file-format,
G2, Video Layer, media-decode, save/reopen or export completion claim.
