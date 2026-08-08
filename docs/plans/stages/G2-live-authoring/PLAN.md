---
id: G2
kind: stage-plan
status: proposed
master_plan: MP-001
owner_role: live-authoring-lead
last_verified: 2026-08-07
research_basis: docs/research/visual-authoring-hierarchy-screening-draft.md
decision_basis: RFC-0001,RFC-0002
---

# G2 — Transactional Live Authoring and Hierarchy Spine

> Proposed next-gate plan. Creating this plan does not activate G2, waive G0/G1
> evidence, or accept RFC-0001/RFC-0002. MP-001 remains the only delivery-order
> authority.

# Outcome

Deliver the first durable Unity-like live-authoring loop: UI and external Agent
edits normalize to the same typed transaction, accepted candidates publish one
coherent revision to Timeline/Inspector/Canvas, invalid candidates preserve
Last-Known-Good, and a bounded group hierarchy proves that visual structure does
not require one Timeline row per primitive.

# User-visible demonstration

On the reference `Subscribe Group` project, the user can:

1. create/open a real project and see one accepted revision;
2. add or edit Text, Shape and Image from UI commands;
3. group the Subscribe parts into one collapsed Timeline row;
4. double-click/drill down to address child parts and return by breadcrumb;
5. animate the group around an explicit anchor while retaining a local bell
   animation;
6. save, close and reopen with stable IDs, exact time and identical semantic
   digest;
7. ask an external Agent to describe, measure, validate and commit the same
   change through file/CLI/MCP surfaces;
8. observe malformed, stale and cyclic edits rejected with matching structured
   diagnostics while the prior visual state keeps playing;
9. install and run development artifacts on macOS and Windows before G2 exit.

# Entry evidence

Normal entry requires:

- G0 and G1 exit evidence, including the required physical Windows lane;
- owner disposition of EXP-001 and EXP-001A;
- accepted decision for the canonical project format and migration authority,
  including disposition of ADR-0008 and RFC-0001;
- accepted hierarchy/compositing decision arising from RFC-0002;
- declared reference projects, device tiers and performance budgets;
- no unresolved P0 contradiction with architecture invariants.

Documentation and bounded experiments may be prepared before entry. G2 cannot
be marked active and its exit cannot be inferred from macOS-only evidence.

# Included scope

- stable typed IDs and explicit units;
- canonical schema/registry and deterministic format/migration rules;
- typed ChangeSets, CAS acceptance, journal/replay and reconciliation;
- one RevisionAuthority, Last-Known-Good and compatible EvaluationStamp;
- exact composition time experiment and named rounding behavior;
- Text, Shape and Image reference descriptors;
- LayerGroup parent/order/collapse/drill-down and one parent animation;
- recursive hierarchy evaluation into one Evaluation/Render Plan;
- schema-driven minimal Inspector and hierarchy Timeline projection;
- Console, CLI, MCP and Agent Skill parity;
- create/open/edit/save/reopen/recovery and development installers;
- macOS and Windows semantic and visual qualification.

# Explicitly excluded

- production Video/Audio import, waveform, hardware playback/export and NLE
  Track/Clip editing, which remain G4;
- broad materials, complete masks/FX, Adjustment, typography breadth and curve
  editor, which remain G3 after this spine is accepted;
- Glass/backdrop effects and Motion Blur implementation;
- a public node editor, expressions or arbitrary project C++/SkSL;
- public native plugin ABI, marketplace, HDR/RAW, 3D, particles, tracking/roto;
- full iOS/Android Studio productization.

# Contracts and invariants

- `docs/architecture/INVARIANTS.md` remains binding.
- RFC-0001 and RFC-0002 must be decided before their proposed models become
  product contracts.
- `ARCH-VA-001` is a candidate until an ADR accepts it.
- UI/file/CLI/MCP are clients of the same Application/Core command service.
- `Layer != Track != Clip != ContentNode != TimelineRow`.
- ProjectClock remains the one mutable project-time authority.
- Preview/export share one semantic evaluator.
- portable project state contains no Qt, Skia, codec, OS or native GPU objects.
- invalid candidates never replace Last-Known-Good.

# Dependencies and kill risks

```text
G2-WP01 Admission Decisions
        |
        v
G2-WP02 Project Schema and Registry
        |--------------------|
        v                    v
G2-WP03 ChangeSets       G2-WP04 Hierarchy Evaluation
        |                    |
        `----------+---------'
                   |--------------------|
                   v                    v
         G2-WP05 Studio Tree     G2-WP06 Agent Parity
                   |                    |
                   `----------+---------'
                              v
                    G2-WP07 Round-trip Exit
```

Stop or return to decision review if an implementation introduces a second
project truth/evaluator/clock, makes UI or watcher authoritative, loses stable
IDs, requires ordinary C++ recompilation, allows cycles to render, introduces
backend types into Core, silently changes platform semantics, or corrupts LKG.

# Workstreams and work packages

1. [`G2-WP01`](work-packages/G2-WP01-admission-decisions.md) — decide project
   format, hierarchy and the bounded reference slice.
2. [`G2-WP02`](work-packages/G2-WP02-project-schema-registry.md) — typed model,
   descriptor registry, exact units/time and canonical serialization.
3. [`G2-WP03`](work-packages/G2-WP03-changesets-journal-reconciliation.md) —
   transactional edits, CAS, journal/replay, watcher reconciliation and LKG.
4. [`G2-WP04`](work-packages/G2-WP04-hierarchy-evaluation-stamp.md) — group
   hierarchy, exact transforms/time maps, recursive evaluator and stamp.
5. [`G2-WP05`](work-packages/G2-WP05-studio-tree-inspector-console.md) —
   Timeline tree, drill-down, schema Inspector and Console projections.
6. [`G2-WP06`](work-packages/G2-WP06-agent-cli-mcp-parity.md) — generated Agent
   guidance, measurement, validation, diff/commit and semantic parity.
7. [`G2-WP07`](work-packages/G2-WP07-roundtrip-installable-exit.md) — project
   round-trip, fault corpus, performance and installable macOS/Windows exit.

# Experiment coverage and owner-revision inputs

Pre-G2 experiments are evidence inputs, not completed G2 work packages. The
2026-08-08 owner review is recorded in
[`EV-VA-0001`](../../../evidence/reviews/EV-VA-0001-reels-authoring-review.md),
and its bounded remediation is planned by
[`EXP-006`](../../experiments/EXP-006-semantic-authoring-measurement.md).

| Area | Demonstrated before G2 | Revision still required | Owning package |
|---|---|---|---|
| hierarchy | ordered LayerGroup, root order, parent transform, collapsed row/drill-down and bounded atomic group/reparent intents | formal generated ChangeSet adoption, Studio reparent UX and desktop evidence | WP02/WP03/WP05 |
| Canvas coordinates | authored Composition dimensions, exact ranges, explicit Transform units and experimental geometry/mask/effect/world bounds at exact time | formal EvaluationStamp/digest adoption and measured authoring commands | WP02/WP04 |
| Text layout | bounded TextBox/paragraph schema, shared Core layout port/result, Skia HarfBuzz/ICU preview/CLI measurement, baseline/logical/ink/clipped metrics and fail-closed Font resolution | positive packaged Font-byte resolver, formal schema/evaluator adoption and cross-platform parity evidence | WP02/WP04 |
| local FX | ordered static Blur, Shadow and Glow stack plus topology-preserving AddEffect and unsupported-animation rejection | generated property capability adoption and owner-local Timeline projection | WP02/WP03/WP05 |
| Agent authoring | RFX validate/describe/live reload, LKG diagnostics, semantic guardrails and generated property catalog bound to the RFX4 Registry digest | inspect/measure/capabilities/lint/diff/commit operations generated from that digest | WP03/WP06 |
| integrated proof | macOS real project, accepted revisions and GPU preview | sanitized Reels regression, save/reopen/parity/failure corpus and physical Windows evidence | WP07 |

The bounded G2 Text requirement is measurable TextBox and node alignment for
the reference fixture, not complete G3 typography. General animation of FX
properties, broad motion/easing, Group isolation and Precomposition remain G3
or later work. Until admitted, those requests fail closed rather than producing
duplicate-Layer approximations.

# Platform matrix

| Lane | Shared contract during implementation | Physical exit evidence |
|---|---|---|
| macOS arm64 | required from first revision | required |
| Windows x64 | required from first revision | required |
| iOS | portable Core/schema/build canary | not a G2 Studio exit |
| Android | portable Core/schema/build canary | not a G2 Studio exit |

Platform adapters may differ only for admitted window/GPU/filesystem/lifecycle
services. The semantic digest and diagnostic codes must match on desktop lanes;
rendered pixels use calibrated tolerance fixtures.

# Performance and reliability budgets

Targets to calibrate at WP01, not claims of current performance:

- 10,000 interleaved UI/file candidates: zero mixed revisions and zero LKG loss;
- 500 layers/5,000 keys: parse plus validation p95 <= 100 ms on the declared M1
  reference tier and a declared Windows peer;
- accepted revision visible within one presented frame;
- no authoring-thread stall over 16 ms for the bounded reference operation;
- baseline 1080x1920@60: CPU evaluation/plan p95 <= 2 ms and GPU p95 <= 12 ms
  on the qualified reference tiers;
- canonical serialize/parse/serialize byte-stable for the same schema;
- migration second pass is a no-op and preserves IDs/digest;
- no cumulative drift in admitted rational-rate fixtures;
- no unbounded memory growth across 10,000 frames or a device-loss cycle.

If measurements show these thresholds are unrealistic, WP01 must revise and
record the budget before implementation is judged; failures may not be hidden.

# Security, privacy and licensing

- Project input is non-executable and bounded by parser/resource limits.
- Paths, file portals, symlinks and external assets follow the accepted sandbox
  and relink policy.
- Diagnostics exclude media/project content by default and redact user paths in
  shareable bundles.
- No dependency enters without official immutable origin, license and SBOM
  intake.
- Native extensions and arbitrary shaders are excluded.

# Diagnostics and observability

Every candidate exposes candidate/base revision, source, parse/validate/apply
duration, stable diagnostics and activation result. Every evaluation exposes
semantic digest, exact stamp, hierarchy/bounds measurements, pass/transient/cache
telemetry and platform capability state. Studio Console, CLI, MCP and project
diagnostic files render the same diagnostic records.

# Release artifact

One traceable development installer/artifact per macOS arm64 and Windows x64
with the same project fixtures, registry digest, CLI/MCP contract, symbols,
dependency provenance and clean-machine create/open/edit/reopen receipts.

This is not a redistributable paid beta, signing entitlement claim, media editor
or complete visual-effects product.

# Acceptance and evidence matrix

| Proof | Required evidence |
|---|---|
| UI/Agent parity | normalized ChangeSet and exact semantic digest comparison |
| Atomic authority | stress trace proving no mixed revision or LKG corruption |
| Hierarchy | Subscribe Group collapse/drill-down/parent-child animation receipt |
| Time/geometry | exact frame/time plus local/logical/ink/effect/world bounds measurements |
| Alignment | TextBox and measured Text-to-Shape alignment within declared tolerance |
| Topology intent | AddEffect preserves Layer/Group/root counts; unsupported animated FX rejects |
| Persistence | create/save/close/reopen and idempotent migration corpus |
| Failure containment | syntax/schema/stale/cycle/port/partial-write fault corpus |
| Evaluator unity | preview/offline probe sharing semantic graph/digest |
| Cross-platform | macOS and Windows physical semantic/visual receipts |
| Installability | clean-machine install/launch/project round-trip/uninstall |

Evidence belongs under `docs/evidence/G2/` and may be created only when the
corresponding run has actually occurred.

# Failure containment and rollback

- Failed candidates keep the last compatible accepted revision/stamp visible.
- Journal recovery produces either the previous or next complete transaction.
- Migrations preserve an untouched recoverable source until accepted.
- A backend preparation/device failure cannot partially activate a candidate.
- Each WP can be reverted at its contract boundary; no WP may require deleting
  user media or rewriting unrelated project history.

# Exit decision

G2 passes only after all seven packages, accepted decision records, desktop
artifacts, failure corpus, budgets and cross-platform evidence pass. A macOS
demo, QML tree, parser extension, screenshot, compile-only Windows lane or Agent
example alone cannot pass G2.
