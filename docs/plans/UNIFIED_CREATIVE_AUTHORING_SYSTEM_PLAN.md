---
id: PLAN-UCAS-001
kind: cross-stage-enabling-plan
status: approved
execution_state: gated-by-MP-001
version: 1
master_plan: MP-001
research_basis: RESEARCH-AAPR-001
guardrails: PLAN-XPLAT-FIX-001
owner_role: creative-authoring-and-agent-systems
stage_routes:
  - G2
  - G3
  - G5
  - G9
  - G10
canonical_for: unified-creative-authoring-execution
last_verified: 2026-08-09
approved_by: product-owner-instruction-2026-08-09
---

# Unified Creative Authoring System

## Authority and non-authority

This is the official cross-stage execution plan for ReFusion creative
authoring, parameterized Recipes, professional motion, Agent/CLI/MCP parity,
Style Packs and future extension exposure. It converts the screened
[`RESEARCH-AAPR-001`](../research/agent-authoring-preset-recipe-system-draft.md)
into ordered, testable work packages.

This plan is not a second Master Plan. [`MP-001`](MASTER_PLAN.md) remains the
only delivery-order authority. Approval of this plan fixes the architecture,
dependencies, evidence requirements and forbidden shortcuts; it does not
activate G2, G3, G5, G9 or G10. A work package may start only when its owning
Master Plan gate and its own entry conditions are satisfied.

Every package remains subject to
[`PLAN-XPLAT-FIX-001`](FIX_CROSS_PLATFORM_ARCHITECTURE.md): creative semantics
are implemented once in portable Core/Application/RenderPlan/common Skia;
Metal, D3D12 and Vulkan adapters contain mechanics only.

## Outcome

Deliver one professional creative-authoring system in which a UI user, file
candidate, CLI client, local MCP Agent and later a mobile/remote Agent can:

- discover qualified properties, effects, curves, generators and Recipes;
- create or update a parameterized visual result with one bounded intent;
- inspect exact project-space geometry, hierarchy and exact project time;
- preview a normalized atomic ChangeSet before committing it;
- commit through the one RevisionAuthority with CAS, durable idempotency and
  Last-Known-Good protection;
- edit Recipe parameters through sliders or typed commands without rebuilding
  the project by hand;
- detach a manually edited channel from Recipe ownership without silent loss;
- render the same materialized project meaning through Preview and Export on
  every qualified backend;
- extend the catalog later without adding another project, command, animation,
  renderer or plugin engine.

The first success is not a large preset count. It is one non-trivial Recipe
that travels through UI and Agent/MCP, produces the same normalized ChangeSet,
publishes one revision, lowers one RenderPlan and passes macOS Metal and Windows
D3D12 qualification.

## One legal route

```text
UI / File Candidate / CLI / MCP / future Mobile Gateway
                         |
                         v
             Unified AuthoringService
  Context + Catalog + Inspect + Measure + Plan + Commit
                         |
                         v
       PlanRequest -> server PlanReceipt -> CommitRequest
                         |
                         v
        Normalized atomic typed ChangeSet + CAS + LKG
                         |
                         v
              AcceptedRevisionBundle
                 /                 \
                v                   v
      Materialized Project       Authoring Receipt
       only visual truth       provenance/ownership only
                |
                v
      exact-time evaluator -> immutable VisualRenderPlan
                |
                v
          common Skia execution / Offline execution
                |
        Metal mechanics | D3D12 mechanics | Vulkan mechanics
```

The renderer, evaluator and platform backends must never read Recipe, Style or
Preset names, nor use an authoring Receipt as visual input. They execute only
materialized, registered project entities and resolved RenderPlan operations.

## Non-negotiable decisions

The following decisions must be recorded as ADRs before their dependent code
package starts:

1. **Recipe truth and Receipt lifecycle** — materialized entities are the only
   visual truth; Receipts manage provenance and ownership through Apply,
   Update, Detach, Reattach, Reset, Bake, Remove and explicit Upgrade.
2. **Federated catalog** — domain registries retain ownership and expose one
   deterministic capability/search view; a God Registry is forbidden.
3. **Atomic authoring transaction** — one versioned IntentEnvelope normalizes
   into one multi-operation ChangeSet with CAS, audit identity, durable
   idempotency and typed pre/postconditions.
4. **Recipe package and bounded IR** — a pure deterministic compiler expands a
   versioned, locked Recipe into a ChangeSet; no native code, network, wall
   clock, OS font lookup or backend knowledge is allowed during compilation.
5. **Version, dependency and migration policy** — immutable versions and
   digests, exact locks, explicit migrations and explicit Upgrade; never
   resolve `latest` while opening a project.
6. **Animation and curve contract** — one exact-time curve truth; Graph Editor,
   presets and Agent operations are projections over it, never UI/native
   animations.
7. **Package trust and extension safety** — identity, signatures, permissions,
   budgets, licensing, provenance, revocation and the extension tiers already
   governed by the cross-platform plan.

Deferred decisions block the dependent work package. They may not be guessed
inside implementation code.

## Target package boundaries

```text
src/core/
  project/             IDs, time, entities, animation tracks, receipts
  authoring/           typed operations and normalized ChangeSet meaning
  descriptors/         shared descriptor header and domain registries
  recipes/             bounded Recipe IR and pure compiler contracts

src/application/
  authoring/           Context, Inspect, Measure, Plan, Commit, diagnostics
  admission/           capability/resource preparation and atomic publish
  catalogs/            federated search/capability/qualification projections
  assets/              content-addressed ingest and provenance

src/runtime/render/
  RenderPlanCompiler   accepted revision + exact time -> immutable plan

src/adapters/skia/
  common compositor/text/resources only; no platform or Recipe semantics

src/platform/
  Metal/D3D12/Vulkan/media/window/sync mechanics only

apps/studio/            descriptor-driven projections and input adapters
apps/cli/               AuthoringService client
services/mcp/           local protocol adapter; never project authority
services/plugin_host/   later out-of-process extension execution only
```

## Stage routing

| Concern | Work packages | Owning Master Plan gate |
|---|---|---|
| Baseline, corpus and blocking decisions | UCAS-WP00–01 | preparation; decisions at G2 admission |
| Catalog, transaction, Agent surfaces, Recipe/Receipt and base curves | UCAS-WP02–08A | bounded G2 foundation |
| Professional curves, backgrounds, text, shapes, FX/materials and choreography | UCAS-WP08B–13 | G3 |
| Dual-desktop qualification and complete creator loop | UCAS-WP14 | G5, with required G4 dependencies |
| Mobile contract canaries and product-safe Agent gateway | UCAS-WP15 | canaries early; productization at G9 |
| Public declarative/native extension exposure | UCAS-WP16 | G10, after internal contracts stabilize |

Research taxonomy may grow before a gate. Production registration, shipping
claims and public APIs may not.

G2 may admit only a tiny built-in set of **materialized, bounded Recipes** as a
conformance proof. Downloadable packages, public Recipe graphs and permanently
live declarative graphs remain G10 after G5, exactly as required by MP-001.

## Cross-plan execution and evidence ownership

For G2, this plan is an acceptance overlay over the existing stage packages;
it does not create duplicate implementation owners. One assertion has one
canonical evidence owner:

| UCAS scope | G2 implementation/evidence owner | Rule |
|---|---|---|
| UCAS-WP00 | UCAS-WP00 | research baseline only |
| UCAS-WP01 | G2-WP01 | decisions are recorded once in G2 admission evidence |
| UCAS-WP02 | G2-WP02 | schema/registry/catalog proof stays in G2-WP02 |
| UCAS-WP03 | G2-WP03 | ChangeSet/CAS/journal proof stays in G2-WP03 |
| UCAS-WP04 measurement | G2-WP04 | evaluation/measurement evidence stays in G2-WP04 |
| UCAS-WP04 external surfaces | G2-WP06 | Agent/CLI/MCP/Skill evidence stays in G2-WP06 |
| UCAS-WP05–06 bounded Recipe proof | G2-WP06 | no separate UCAS implementation or duplicate receipt |
| UCAS-WP07 bounded evaluator/plan | G2-WP04 plus PLAN-XPLAT-FIX-001 | semantic proof and platform proof retain their existing owners |
| UCAS-WP08A | G2-WP02 plus G2-WP04 | schema and exact evaluator assertions remain separated |
| UCAS G2 integration milestone | G2-WP07 | first UI/Agent/MCP Recipe and desktop round-trip exit |

Where a later G3/G5/G9/G10 stage plan does not yet exist, UCAS supplies the
approved dependency baseline only. That future stage plan must derive its work
packages from UCAS and become the canonical evidence owner when activated.

## Dependency graph

```text
WP00 Baseline and coverage corpus
  |
  v
WP01 Decisions -> WP02 Federated catalog -> WP03 Atomic ChangeSet/Plan/Commit
                                      |
        +-------------------+---------+---------+-------------------+
        |                   |                   |                   |
        v                   v                   v                   v
WP04 Unified surfaces  WP05 Recipe compiler  WP07 Render funnel  WP08A Base curves
                            |
                            v
                   WP06 Receipt lifecycle
        |                   |                   |                   |
        +-------------------+---------+---------+-------------------+
                                      |
                                      v
                    G2 integration owned by G2-WP07
                                      |
                                      v
                         WP08B Professional motion (G3)
                           |
          +----------------+--------------------------------+
          |                                                 |
          v                                                 v
WP09A Assets                                      WP11 Shapes/Vectors
          |------------------|                              |
          v                  v                              |
WP09 Backgrounds         WP10 Text                          |
          |                  |                              |
          +------------------+------------------------------+
                             |
                             v
                    WP12 FX and materials
                           |
                           v
                 WP12A Adjustment
                           |
                           v
               WP13 Choreography/Style Packs
                           |
                           v
          WP14 Desktop qualification and creator loop
                   /                         \
                  v                           v
        WP15 Mobile/canaries          WP16 Public extensions
```

## UCAS-G2-INT-001 — first unified integration milestone

This is not another work package. It is an acceptance milestone owned by
`G2-WP07`, after the bounded portions of UCAS-WP02–08A have passed under their
crosswalk owners. Its canonical evidence is `docs/evidence/G2/G2-WP07.md`.

The milestone passes only when one bounded built-in Recipe:

1. is found through the federated catalog with a fixed catalog/dependency lock;
2. is planned through UI and local MCP into equal normalized ChangeSet meaning;
3. commits once through CAS and atomic Runtime preparation;
4. materializes one legal hierarchy with owner-local FX/animation and a
   non-rendering Receipt;
5. updates one Recipe parameter, detaches one manually edited channel, and
   preserves the edit during reapply;
6. saves, reopens, replays Undo/Redo and survives missing-Recipe diagnostics;
7. lowers the same exact-time RenderPlan on the admitted macOS and Windows
   profiles and records semantic plus required visual/performance evidence;
8. leaves Timeline, Inspector and Canvas on one accepted revision after every
   accepted or rejected operation.

Failure keeps G2 exit blocked and returns to the owning G2/UCAS package; it may
not be repaired by duplicate Layers, raw project rewrite, backend code or a
second evidence receipt.

## Work-package register

| ID | Outcome | Status | Plan |
|---|---|---|---|
| UCAS-WP00 | Baseline, vocabulary and professional workflow corpus | proposed | [WP00](unified-creative-authoring/work-packages/UCAS-WP00-baseline-corpus.md) |
| UCAS-WP01 | Blocking architecture decisions | proposed | [WP01](unified-creative-authoring/work-packages/UCAS-WP01-decision-package.md) |
| UCAS-WP02 | Federated descriptor catalog, locks and qualification view | proposed | [WP02](unified-creative-authoring/work-packages/UCAS-WP02-federated-catalog.md) |
| UCAS-WP03 | Atomic ChangeSet and Plan/Commit transaction | proposed | [WP03](unified-creative-authoring/work-packages/UCAS-WP03-atomic-authoring-transaction.md) |
| UCAS-WP04 | Unified UI/File/CLI/MCP surfaces and digital eye | proposed | [WP04](unified-creative-authoring/work-packages/UCAS-WP04-agent-surfaces-skill.md) |
| UCAS-WP05 | Versioned Recipe package, bounded IR and pure compiler | proposed | [WP05](unified-creative-authoring/work-packages/UCAS-WP05-recipe-package-compiler.md) |
| UCAS-WP06 | Materialized Recipe Receipt and ownership lifecycle | proposed | [WP06](unified-creative-authoring/work-packages/UCAS-WP06-receipt-lifecycle.md) |
| UCAS-WP07 | Descriptor-to-RenderPlan-to-common-Skia funnel | proposed | [WP07](unified-creative-authoring/work-packages/UCAS-WP07-render-funnel.md) |
| UCAS-WP08A | Canonical base animation curves | proposed | [WP08A](unified-creative-authoring/work-packages/UCAS-WP08A-base-animation-curves.md) |
| UCAS-WP08B | Professional motion, spatial paths and graph editing | proposed | [WP08B](unified-creative-authoring/work-packages/UCAS-WP08B-professional-motion.md) |
| UCAS-WP09A | Asset ingest, generation, provenance and licensing | proposed | [WP09A](unified-creative-authoring/work-packages/UCAS-WP09A-assets-generation.md) |
| UCAS-WP09 | Background, generator and surface Recipe catalog | proposed | [WP09](unified-creative-authoring/work-packages/UCAS-WP09-background-system.md) |
| UCAS-WP10 | Professional Text Animator and typography Recipes | proposed | [WP10](unified-creative-authoring/work-packages/UCAS-WP10-text-system.md) |
| UCAS-WP11 | Shape, vector, paint and operator Recipes | proposed | [WP11](unified-creative-authoring/work-packages/UCAS-WP11-shape-vector-system.md) |
| UCAS-WP12 | FX, Layer Styles and materials | proposed | [WP12](unified-creative-authoring/work-packages/UCAS-WP12-fx-materials.md) |
| UCAS-WP12A | Bounded Adjustment and color operations | proposed | [WP12A](unified-creative-authoring/work-packages/UCAS-WP12A-adjustment-system.md) |
| UCAS-WP13 | Choreography and Style Packs, including editorial collage | proposed | [WP13](unified-creative-authoring/work-packages/UCAS-WP13-choreography-style-packs.md) |
| UCAS-WP14 | Dual-desktop qualification and complete creator loop | proposed | [WP14](unified-creative-authoring/work-packages/UCAS-WP14-desktop-qualification.md) |
| UCAS-WP15 | Mobile canaries, gateway and productization | proposed | [WP15](unified-creative-authoring/work-packages/UCAS-WP15-mobile-authoring.md) |
| UCAS-WP16 | Public declarative and isolated native extension SDK | proposed | [WP16](unified-creative-authoring/work-packages/UCAS-WP16-extension-sdk.md) |

## Required evidence model

Each package without an activated stage owner writes its planning receipt beneath
`docs/evidence/UCAS/`. If the crosswalk names a stage work package, that stage
evidence file is canonical and UCAS links to it rather than duplicating it. Each
canonical receipt records:

- exact source commit, artifact and toolchain;
- project, schema, registry, catalog, package and dependency-lock digests;
- exact positive, negative, fault and rollback commands/results;
- platform/device/backend and declared capability profile;
- compile, physical-run, semantic, visual, performance and qualified states as
  separate booleans;
- p50/p95/p99 latency, bytes, items, memory and GPU/pass budgets where relevant;
- sanitized UI/CLI/MCP transcripts and affected stable IDs;
- what remains not run, unsupported or blocked.

No package is complete because a type compiles, one backend runs, a screenshot
looks acceptable or a prose Skill exists.

## Initial bounded budgets

These are planning bounds and must be accepted or revised in WP01 before they
become qualification limits:

| Surface | Initial bound |
|---|---:|
| Core MCP tools | at most 12 |
| Project context | at most 8 KiB |
| Outline page | at most 100 nodes / 16 KiB |
| Inspect response | at most 25 properties / 12 KiB |
| Catalog search | at most 20 hits / 12 KiB |
| Diagnostics page | at most 50 records / 12 KiB |
| ChangeSet | at most 256 operations / 512 created nodes |
| Local context/search/inspect p95 | at most 100 ms |
| Simple plan p95 | at most 100 ms |
| Bounded Recipe plan p95 | at most 250 ms |
| MCP adapter overhead p95 | at most 20 ms over AuthoringService |
| Publication visibility | within one presented frame |
| Transaction stress | 10,000 operations, zero mixed revision/LKG loss |

Provider/LLM/network generation latency is reported separately from engine plan
and commit latency. The engine must not hide one inside the other.

## Bulk-preset admission barrier

Before UCAS-WP02–08A pass their admitted scope:

- production `recipe.apply` registration is forbidden;
- Style Pack activation is forbidden;
- downloadable declarative graphs are forbidden;
- catalog growth beyond a tiny conformance set remains research-only.

Before UCAS-WP14 desktop qualification, only one integration Style Pack may be
used as the cross-capability reference. A Recipe that needs a missing primitive
must return to the owning primitive package; it may not approximate the result
with hidden duplicate Layers, raster fallback or backend-specific code.

## Capability and Recipe Definition of Done

```text
Descriptor/version/digest
-> typed parameters, units, ranges and compatibility
-> package, dependency lock and migration
-> Intent/ChangeSet + PlanReceipt/Commit
-> CAS/admission/LKG + durable idempotency
-> canonical persistence and unresolved round-trip
-> UI/File/CLI/MCP parity where lossless
-> generated Skill and executable examples
-> deterministic compiler/evaluator
-> RenderPlan and common execution
-> Preview/Export parity
-> semantic/visual/performance qualification
-> diagnostics, security, recovery and rollback
```

`qualified=true` is derived from the least-qualified required dependency for a
named platform/profile. It is never a manually asserted field in a Recipe.

## Stop conditions

Stop the active package and return to its decision owner if implementation
introduces any of the following:

- a second project, command, revision, animation, render or plugin truth;
- Agent direct mutation of supported local edits through whole-file rewrite;
- Receipt or Recipe recompilation as renderer/open-time authority;
- a monolithic registry that owns all domain semantics;
- UI, MCP, platform or backend code that validates or invents visual meaning;
- hidden MCP session state instead of explicit revision-bound handles;
- unbounded context/catalog/diagnostic responses;
- OS fonts, wall clock, network or nondeterministic generators in compilation;
- FX, animation or internal primitives represented as root Timeline Layers;
- platform-specific visual fallback or silent capability degradation;
- broad G3 work inside G2, mobile product claims before G9, or public extension
  promises before G10;
- claims of unlimited or 60–70% coverage without a named corpus and evidence.

## Exact execution start

The first permitted action under this approved plan is **UCAS-WP00**, and it is
documentation/evidence preparation only. The first code package is blocked
until **UCAS-WP01** records the required accepted decisions and `MP-001`
activates the owning stage. No later package may be started to bypass this
order.
