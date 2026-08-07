---
id: ADR-0008
kind: adr
status: proposed
title: Versioned JSON project-document seed
owner_role: core-project
decision_due: G2-entry
last_verified: 2026-08-07
---

# Proposed decision

Use a versioned JSON document as the bounded G1/G2 project-open seed. Parse it
through a native adapter into portable Core snapshots, validate all stable IDs,
exact nanosecond ranges, Canvas dimensions, frame rate, layer content and
keyframes, then admit the snapshot through the private Application Host.

The current seed is intentionally narrow: one Composition, Shape/Text content,
Transform2D scalar animation, and one immutable opened revision. It is an actual
file-backed project used by the macOS walking product, but it is not yet the
stable v1 save format or a G2 completion claim.

# Boundaries

- Serialized state contains no Qt, Skia, Metal or native handles.
- Qt JSON values exist only inside the file adapter and are converted before
  the Application Host receives the candidate.
- Missing, malformed, unsupported or semantically invalid files fail closed
  with `RFX-PROJECT-OPEN`; no hard-coded fallback scene is admitted.
- Project time is integer nanoseconds and layer ranges are half-open.
- Canvas and Skia consume the same validated immutable Composition snapshot.
- UI receives metadata/telemetry only and owns neither the document nor the
  playback/render clock.

# Deferred G2 decisions

Directory bundle layout, crash-safe journal/save, migrations, canonical byte
ordering, schema registry, dependency/source databases, watcher reconciliation,
CAS ChangeSets, Undo/Redo and agent file transactions remain G2 work. This ADR
must be accepted or superseded before the seed becomes a shipping file-format
contract.
