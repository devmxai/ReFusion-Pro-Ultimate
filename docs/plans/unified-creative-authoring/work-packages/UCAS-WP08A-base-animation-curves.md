---
id: UCAS-WP08A
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G2-bounded
owning_gate: G2
depends_on: UCAS-WP01,UCAS-WP02,UCAS-WP03
decision_dependencies: base-animation-curve-ADR
cross_plan_dependencies: MP-001,G2-WP02,G2-WP04
evidence_owner: G2-WP02-and-G2-WP04
owner_role: animation-core
evidence: docs/evidence/G2/G2-WP04.md
---

# Outcome

Establish one canonical exact-time animation track that later motion presets,
Graph Editor views and Agent commands extend without replacing.

# Dependencies

UCAS-WP01–03, exact ProjectTime and the accepted property descriptor contract.

# Deliverables

- typed `AnimationTrack<T>` and canonical key/segment representation;
- exact key times and deterministic Hold/Step, Linear and bounded Cubic Bezier;
- typed property/range/duplicate-time/handle validation;
- explicit interpolation, extrapolation, rounding and serialization policy;
- temporal and spatial curve separation in the schema;
- cross-toolchain evaluator and migration fixtures;
- no broad Graph Editor, Spring or animated FX claim in this bounded package.

# Verification and exit

- pure exact-time evaluation and canonical save/reopen round-trip;
- identical samples/digests under AppleClang and MSVC;
- invalid keys/handles/ranges reject before accepted publication;
- UI and Agent construction of the same curve normalizes equally;
- QML/native animation APIs contain no project motion semantics.

# Failure and rollback

Retain the prior linear reader and reject unsupported new curve kinds. Never
silently bake or reinterpret them as Linear.
