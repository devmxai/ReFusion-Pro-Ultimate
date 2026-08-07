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
