---
id: G1-WP05
kind: work-package
status: planned
gate: G1
owner_role: gpu-runtime
evidence: docs/evidence/G1/G1-WP05.md
---

# Outcome

Introduce typed device generation, presenter/resource/fence leases, device-loss
state, and runtime counters/traces shared by Skia, media and presentation proofs.

# Absolute admission thresholds

- CPU video-pixel map/readback/conversion/upload: exactly zero.
- Software decoder, WARP, silent fallback and cross-adapter events: exactly zero.
- Unattributed GPU copy/conversion or submission: exactly zero.
- Stale-generation resource accepted after device loss: exactly zero.

Latency, memory and thermal budgets are recorded per named device tier; no
aggregate cross-device number can qualify a platform.
