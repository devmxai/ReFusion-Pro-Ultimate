---
id: EXP-002
kind: work-package
status: implementation-passed-owner-evaluation-partial
gate: pre-G2-experiment
owner_role: visual-authoring-architecture
evidence: docs/evidence/experiments/EXP-002.md
---

# Outcome

Prove the smallest professional hierarchy slice before G2 activation: one
portable LayerGroup with ordered typed children, explicit root order, one parent
transform/animation, one collapsed Timeline row and child drill-down, evaluated
by Core and consumed by the existing Skia path.

# Authorization and decision boundary

The product owner instructed implementation after reviewing the proposed G2
plan on 2026-08-07. This authorizes a bounded experiment only. It does not pass
G0/G1, activate G2, accept RFC-0001/RFC-0002, adopt RFX2 as the shipping format,
or waive Windows evidence.

# Dependencies

- EXP-001/EXP-001A typed project and live-reload spine;
- proposed RFC-0002 and `ARCH-VA-001`;
- accepted architecture invariants and ADR-0009 clock authority;
- existing macOS Metal/Skia walking product.

# Included

- `LayerGroupId`, ordered `VisualNodeRef`, explicit Composition roots;
- RFX1 read migration plus canonical experimental RFX2 writing;
- transform anchor and `T * R * S * T(-anchor)` parent evaluation;
- one-parent, known-reference, group-range and cycle validation;
- pass-through groups with explicit rejection of group opacity/isolation;
- backend-neutral evaluated world matrices and effective layer opacity;
- Skia consumption of the Core-evaluated flat visual result;
- collapsed root Group row, drill-down/breadcrumb and stable child rows;
- CLI Group/child/root description and project-local RFX2 Agent guidance;
- positive, negative, exact round-trip, Studio and GPU-render tests.

# Excluded

Group masks/FX/opacity isolation, precomposition, NLE Track/Clip, Image/SVG,
general property registry, cubic easing, Undo journal, Glass, Motion Blur, broad
G3 visual features, production Video/Audio import and Windows qualification.

# Kill criteria

Reject the experiment if Group becomes a UI-only folder, a Timeline Track, a
clock, a second evaluator, backend-specific project state, an implicit offscreen
approximation, or loses child IDs/order during RFX round-trip.

# Verification

```bash
python3 tools/rfdev.py docs-doctor
python3 tools/rfdev.py architecture-check
cmake --workflow --preset macos-core
cmake --workflow --preset macos-studio
cmake --workflow --preset macos-graphics
cmake --workflow --preset macos-visual
refusion-cli validate examples/projects/rfx-authoring-experiment-01/Project.rfx
refusion-cli describe examples/projects/rfx-authoring-experiment-01/Project.rfx
```

# Evidence path

`docs/evidence/experiments/EXP-002.md`.

# Failure and rollback

RFX1 remains readable and the previous flat experiment remains the rollback
model. Invalid RFX2 candidates never replace Last-Known-Good. Removing EXP-002
must not require changing ProjectClock, RevisionAuthority or platform GPU/media
ownership.

# Exact handoff

The owner visually confirms root collapse, double-click drill-down and coherent
parent motion in the real macOS app. RFC-0002 then uses this evidence at the G2
entry decision; downstream Group isolation or broad FX work remains blocked.

# Owner evaluation result

The 2026-08-08 Reels review confirmed that the Subscribe component projects as
one collapsed Group with addressable children. The mechanism therefore passed
its bounded owner evaluation. General Agent topology authoring remains partial:
the composed Background was still authored as independent root Layers because
typed GroupNodes/reparent intents and topology recipes are not yet available.
That remediation is routed to EXP-006 and G2-WP03/WP06; it does not invalidate
the implemented LayerGroup proof or mark RFC-0002/G2 accepted.
