---
id: G0-WP05
kind: work-package
status: active
gate: G0
owner_role: gpu-release-lead
evidence: docs/evidence/G0/G0-WP05.md
last_verified: 2026-08-07
---

# Outcome

Convert every G1 workstream into a bounded proof package with exact devices,
fixtures, counters, budgets, commands, kill criteria, owners, and evidence schema.

# Entry gate

G0-WP04A passed; G0-WP04B local controls passed; Windows Core and required
commercial/device/signing owners have explicit evidence or blocking status.

# Deliverables

Separate packages for macOS/Windows presenter, Skia render fixture, Windows
D3D/Dawn bake-off, Apple/Windows native media surfaces, synchronization/device
loss/counters, packaging/signing, and iOS/Android compile canaries.

# Handoff

An owner performs a criterion-by-criterion G0 exit review. G1 cannot activate
from a skeleton, inactive backend, or compile-only result.

# Current state

The eight G1 proof packages and the macOS lab device are recorded. Windows
device/driver identity, Qt entitlement and signing/notarization ownership remain
the exact external inputs required before the G0 exit review can pass.
