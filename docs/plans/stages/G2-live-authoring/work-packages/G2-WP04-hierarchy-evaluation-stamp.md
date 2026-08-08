---
id: G2-WP04
kind: work-package
status: proposed
gate: G2
owner_role: evaluation-architecture
evidence: docs/evidence/G2/G2-WP04.md
---

# Outcome

Evaluate the bounded visual hierarchy deterministically through one semantic
evaluator/Render Plan and publish only a revision-compatible EvaluationStamp.

# Dependencies

G2-WP02 schema/registry. Integrates with G2-WP03 publication/recovery.

# Read first

- accepted hierarchy/compositing ADR derived from RFC-0002
- `docs/architecture/INVARIANTS.md`
- G2-WP02 exact time, transform and descriptor contracts

# Allowed paths

Portable evaluation/runtime contracts, common renderer plan, platform render
adapters, golden/measurement tests and evidence. Backend code implements the
common plan; it does not define new project meaning.

# Forbidden paths

Second clock/evaluator/export graph; semantic behavior only in Metal/D3D/QML;
hierarchy cycles reaching GPU submission; implicit platform drawable backdrop;
silent unsupported-effect approximation; CPU video pixel work.

# Deliverables

- recursive LayerGroup evaluation with stable parent/order;
- one backend-neutral Text layout port returning immutable TextBox, logical/ink
  bounds, baselines, line metrics, overflow and resolved Font digest;
- exact local/world transform, anchor/pivot and geometry/logical/ink/mask/
  effect-expanded/world bounds measurement at an exact project time;
- parent and child animation composition at arbitrary exact time;
- pass-through versus isolated group behavior under accepted rules;
- typed port/dependency validation and cycle rejection;
- one backend-neutral Evaluation/Render Plan and semantic digest;
- `EvaluationStamp` containing revision, epoch, device generation, composition,
  exact time and quality profile;
- bounds/transient/cache/pass/GPU observability hooks;
- same semantic evaluator for preview and offline probe.
- one layout/evaluation digest shared by preview, offline probe and Agent
  measurement; derived layout results remain cacheable artifacts, not project
  truth.

# Verification

- VS-01 moves the whole group with one parent curve while the bell animates
  locally and retains one collapsed row projection;
- local/world matrices, bounds and pivot match numeric probes;
- bounded Text-to-Shape center alignment differs by at most `0.25`
  Composition pixel before rasterization for each declared alignment basis;
- Shadow/Glow expand effect bounds without changing Text paragraph metrics;
- seek/repeat gives deterministic results and stale epoch work cannot present;
- cycle/invalid-port candidates reject before render preparation;
- preview/offline semantic digest matches;
- macOS/Windows semantic fixtures match and visual goldens remain within the
  calibrated per-capability tolerance.

# Evidence path

`docs/evidence/G2/G2-WP04.md`.

# Failure and rollback

Retain the previous compatible stamp and report a typed rejection. A backend
that cannot implement accepted semantics is `not-qualified`, never silently
degraded.

# Exact handoff condition

WP05 and WP06 can inspect the same hierarchy, measurements, capability states
and diagnostics that Canvas evaluation consumed.
