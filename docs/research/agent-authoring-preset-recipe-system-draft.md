---
id: RESEARCH-AAPR-001
kind: research-draft
status: screened
canonical_for: none
owner_role: agent-authoring-and-creative-systems
last_verified: 2026-08-09
research_basis: RESEARCH-VA-001,MP-001,PLAN-XPLAT-FIX-001
adopted_by: PLAN-UCAS-001
---

# Agent Authoring, Preset, and Recipe System — Screening Draft

> **Non-authoritative research draft.** This document is a design screening
> record. It is not an accepted architecture, an implementation specification,
> a release promise, a capability claim, an activated stage, or a second Master
> Plan. It does not move broad animation, materials, plugins, mobile
> productization, or public extension graphs into an earlier stage. Any
> implementation work derived from this draft requires the appropriate
> RFC/ADR, stage-plan routing, work package, evidence, and owner acceptance.

> **Formal execution route:** the screened conclusions are now sequenced by
> [`PLAN-UCAS-001`](../plans/UNIFIED_CREATIVE_AUTHORING_SYSTEM_PLAN.md). This
> research file remains the taxonomy and screening record; it is not the
> execution or status authority.

## Review question

How can ReFusion let a UI user or an external Agent create professional
backgrounds, text, shapes, effects, animation and complete scene choreography
quickly, with parameterized presets rather than hand-writing every primitive and
keyframe, while preserving:

- one legal project truth;
- one typed Command/ChangeSet/Revision path;
- exact project time and deterministic evaluation;
- one layer owner for its effects and animation;
- editable parameters rather than fixed visual templates;
- compact Agent context and searchable catalogs;
- identical typed-intent semantics for UI, CLI, MCP and future mobile APIs,
  with file candidates sharing admission semantics and intent parity only when
  their semantic diff normalizes losslessly;
- fail-closed capability admission and Last-Known-Good;
- one shared cross-platform implementation lowered through RenderPlan and the
  common Skia executor;
- the same preview and export meaning on every qualified backend?

## Executive decision candidate

The professional direction is not to create thousands of hard-coded commands,
one Tool per preset, one renderer path per style, or one Timeline layer per
effect. The preferred direction is:

1. build a small set of complete, typed and versioned creative primitives;
2. register every property, effect, curve, generator and recipe in one
   queryable catalog;
3. expose parameterized Preset Recipes with real units, ranges, constraints,
   compatibility and qualification metadata;
4. compile a preset application into one normalized atomic ChangeSet;
5. persist normal project entities plus a non-rendering
   PresetApplicationReceipt that records provenance, owned channels and
   parameters;
6. let sliders or Agent commands update the receipt-owned channels
   deterministically;
7. detach an individually edited channel from preset ownership instead of
   silently overwriting the user;
8. keep the renderer unaware of marketing style names: it executes only
   registered primitives through the shared RenderPlan path.

This materialized-recipe approach is the preferred near-term screening
candidate because it gives fast one-command authoring and editable parameters
without adding a second live evaluator. A permanently live declarative Recipe
Graph remains a later decision that needs an explicit ADR and Master Plan
routing.

The target is not an impossible claim that every creative style has been
enumerated. Creative styles are open-ended and change over time. A defensible
target is broad workflow coverage through an extensible taxonomy, followed by
measured coverage criteria. A future claim such as “60–70% of the targeted
professional motion-design workflows” must be backed by a named corpus and
metrics; it is not a current product claim.

## Relationship to existing authority

This draft is subordinate to:

- [MP-001](../plans/MASTER_PLAN.md), the only delivery-order authority;
- [PLAN-XPLAT-FIX-001](../plans/FIX_CROSS_PLATFORM_ARCHITECTURE.md), the active
  cross-platform architecture guardrail;
- [Architecture invariants](../architecture/INVARIANTS.md);
- [Product Contract](../product/PRODUCT_CONTRACT.md);
- [Visual authoring model](../architecture/VISUAL_AUTHORING_MODEL.md);
- [RESEARCH-VA-001](visual-authoring-hierarchy-screening-draft.md), the prior
  hierarchy, ownership and extensibility screening;
- the proposed [G2 stage plan](../plans/stages/G2-live-authoring/PLAN.md),
  especially G2-WP02 for typed animation/schema work and G2-WP06 for bounded
  Agent recipes and generated guidance.

The stage boundary must remain explicit:

| Concern | Current planning home |
|---|---|
| Typed ChangeSet/CAS, bounded hierarchy, generated schemas and bounded recipe intents | proposed G2 |
| Broad animation curves, Text Animator, materials, animated FX, Curve Editor, Glass/backdrop and professional creative breadth | G3 |
| Complete desktop creator loop and shippable desktop integration | G5 |
| Mobile application productization and mobile-safe Agent API | G9 |
| Public extension SDK, downloadable declarative graphs and third-party plugin conformance | G10, after G5 where required |

Research and catalog design may proceed now. Broad implementation may not be
silently pulled into G2.

## Evidence reviewed

The local review covered:

- Core ProjectDocument, ProjectAuthority, exact-time and Revision contracts;
- VisualPropertyRegistry and VisualContributionRegistry;
- current RFX parser and current project Skill/template output;
- current Studio Timeline projection and Agent CLI surface;
- common RenderPlan/Skia compositor and cross-platform contribution rules;
- G2/G3/G5/G9/G10 scope boundaries;
- the existing visual hierarchy research and cross-platform fix plan.

External references were used as interaction and taxonomy evidence, not as
permission to copy another product’s implementation:

- Adobe effects and animation presets;
- Adobe animation graphs, keyframe interpolation and Speed Graph;
- Adobe Text Animator and selector behavior;
- Adobe shape paths, paint operations and layer styles;
- Apple Motion generators, behaviors and keyframe editor;
- W3C easing, color, gradients, SVG paint and filter semantics;
- Unicode, ICU and HarfBuzz text behavior;
- Skia gradients, image filters and registered runtime effects;
- the Model Context Protocol server, resource, tool and authorization model.

## Current repository baseline

The current implementation is a useful foundation, not the proposed creative
system:

- one Shape content kind is currently a rounded rectangle;
- current fills are solid, linear gradient and radial gradient;
- current Text has one logical string, one font identity, basic TextBox,
  direction, alignment, wrapping, line height and letter spacing;
- current layer animation has scalar Position X/Y, Scale X/Y, Rotation and
  Opacity channels;
- ScalarKeyframe stores time and value only;
- current evaluation is Linear;
- Gaussian Blur, Drop Shadow and Glow exist as fixed layer-local effects;
- effect-property animation is intentionally rejected;
- typed external commands cover only a bounded subset such as grouping,
  reparenting, adding Glow and alignment;
- Background creation currently expands to a bounded Shape/gradient result;
- current generated project guidance is partially registry-derived, while
  important workflow prose and examples are still static;
- no production MCP server, mobile authoring gateway or general atomic
  multi-operation Agent API exists;
- Image, Video, Audio, SVG, procedural generators, Glass/backdrop, particles,
  3D, Motion Blur, arbitrary FX animation and public plugins are not current
  qualified capabilities.

Therefore this draft must never be read as a supported-capabilities list.

## Pre-code audit blockers

The current repository proves useful foundations but is not ready for bulk
Recipe implementation:

- ProjectCommandService and ProjectAuthority expose one overload and recording
  branch per command instead of one generic atomic multi-operation ChangeSet;
- durable recipe, receipt, dependency-lock and migration contracts do not
  exist;
- external CLI writes are typed only for a bounded command set; other edits
  still fall back to a complete RFX candidate;
- the idempotency ledger is process-memory state and cannot resolve a commit
  result after restart;
- there is no production MCP server;
- current outline/inspect/measure outputs need fields, filters, pagination and
  bounded response sizes for large projects;
- current Mask/FX Registry covers a small fixed set and the RenderPlan variants
  remain bounded to the current Shape/Text/Fill/FX experiment;
- Studio still contains a Shape-Fill adaptation path that interprets fill kind,
  parameters and default stops before creating Core ShapeFill state;
- Runtime candidate preparation currently probes one time rather than a
  capability-declared critical-time/range set;
- Windows, iOS, Android and Vulkan qualification remains incomplete even where
  shared source exists.

The Studio Shape-Fill path is especially important: UI may collect fill
parameters, but it must submit descriptor-addressed intent. It may not interpret
gradient semantics or create Core fill variants differently from Agent/MCP.

## Target code and package architecture

The exact filenames require an accepted work package, but the dependency shape
should be:

~~~text
contracts/authoring/
  PlanRequest, PlanReceipt, CommitRequest, ChangeSet and Diagnostic schemas

contracts/creative/
  common DescriptorHeader schema
  domain Registry schemas
  Recipe package and Receipt schemas
  migration, dependency-lock and conformance schemas

src/core/
  ProjectChangeSet and operation variants
  RecipeReceipt project state and round-trip
  AnimationCurve and exact-time evaluators
  portable descriptor value types
  validation, migration, stable-ID and digest rules
  RevisionAuthority

src/application/
  AuthoringService
  CatalogQueryService
  RecipeCompiler
  RecipeOwnershipService
  Choreography and constraint solver
  PlanStore and durable idempotency ports
  candidate admission and AcceptedRevisionBundle publication

src/runtime/render/
  Evaluation and VisualRenderPlan compiler
  no Recipe, Preset, Receipt, UI or MCP dependency

src/adapters/skia/
  common compositor and admitted shared SkSL/pass execution
  no Recipe, Preset, Receipt or platform semantic branch

src/platform/
  Metal, D3D12 and Vulkan device/target/import/sync/submit/present only

apps/studio/
  descriptor-generated controls and immutable projections

apps/cli/ and future MCP adapter/
  schema adapters over the same AuthoringService

services/plugin_host/ after its accepted stage
  isolated native/worker extension execution only
~~~

Dependency direction:

~~~text
Contracts -> Core
Core -> Application authoring
Core -> Runtime RenderPlan
Studio / CLI / MCP adapters -> Application authoring
Application coordinates Runtime preparation through ports
Runtime RenderPlan -> Common Skia -> native backend binding
~~~

Runtime and Skia do not depend on Recipe Catalog or Receipt. Platform code does
not depend on ProjectDocument, creative registries or AuthoringService.

### Bounded Recipe IR

Built-in and later Tier-1 declarative packages need a versioned, non-Turing-
complete Recipe IR rather than one C++ function per visual style.

Candidate operations:

- require capability or asset slot;
- create owner/group/content with stable Slot ID;
- set registered property;
- add/update registered Paint, Mask or Effect;
- create normal AnimationTrack with a registered Curve;
- bind a Recipe parameter to one or more target parameters;
- align/place through a registered measurement intent;
- sequence or stagger bounded owners;
- declare dependencies and conflict rules;
- assert topology, bounds, safe-area and cost postconditions.

Conditions are limited to typed parameter variants and bounded lists. Recursion,
node count, dependency depth and operation count have hard limits. There are no
network calls, filesystem discovery, wall-clock reads, platform APIs, arbitrary
expressions, C++, or free project-authored SkSL.

The IR is compiled and validated at package/build time and again during
admission. A new Style normally adds data. A genuinely new visual primitive
enters the accepted contribution tiers:

- Tier 0 built-in contribution;
- Tier 1 declarative preset/graph;
- Tier 2 certified bounded SkSL;
- Tier 3 isolated worker for import/analysis/generation;
- Tier 4 out-of-process native extension after the post-v1 public SDK stage.

This layered model is the defensible meaning of unlimited extensibility:
combinations grow without engine changes, while new primitive classes still
enter through one reviewed extension contract.

## Vocabulary and semantic taxonomy

Terminology must be stable before the catalog grows.

| Term | Meaning |
|---|---|
| Primitive capability | Small typed semantic operation the engine understands, such as a linear gradient, rounded rectangle, Gaussian Blur or cubic curve |
| Paint | A typed surface description such as solid, gradient, pattern, image or registered procedural field |
| Effect | A registered operation applied to an owner’s visual result, with explicit bounds, color, alpha, edge and cost rules |
| Material recipe | A parameterized composition of paints, effects and passes that produces an appearance such as paper, neon or qualified glass |
| Generator | A deterministic source that evaluates from exact project time, parameters, algorithm version and explicit seed |
| Behavior/controller | A deterministic time-dependent driver such as Noise Evolution, an oscillator or a baked audio control track |
| Curve preset | A reusable interpolation policy or canonical curve parameters |
| Motion preset | A parameterized authoring recipe that creates or updates animation channels |
| Recipe | A versioned, typed authoring macro that expands into a normalized ChangeSet and declares topology, dependencies and postconditions |
| Style profile | A multi-axis set of defaults, constraints and tags that guides recipe selection; never a renderer type |
| Style pack | A catalog distribution unit containing compatible recipes, palettes, fonts, assets and guidance |
| Asset source | Immutable ingested media identified by AssetId and digest |
| Scene choreography | Explicit roles, z-order, timing relations, layout constraints and beats for multiple owners |
| Project template | A project-level starting package; not a replacement for typed authoring |

Names such as editorial explainer, paper collage, viral reels, cinematic,
brutalist or glass UI are style/profile tags. They must not become new Layer
kinds, backend switches or bespoke project languages.

## One unified authoring route

~~~text
Studio UI command ─────┐
CLI command ────────────┤
MCP tool call ──────────┤
Mobile/Cloud intent ────┘
                               |
                               v
                     AuthoringService
                               |
                      plan_authoring_intent
                               |
                    Resolve catalog + inspect
                    + measure + normalize
                               |
                               v
                    Atomic typed ChangeSet
                    + server PlanReceipt
                               |
                          commit_plan
                               |
              schema/capability/topology/budget
              text/layout/asset/time validation
                               |
                               v
                 CAS + prepared runtime revision
                               |
                               v
                    AcceptedRevisionBundle
                    /        |        |       \
               Timeline  Inspector  Canvas  Diagnostics
                               |
                               v
                   shared Evaluation/RenderPlan
                               |
                               v
                    common Skia execution
                               |
                  Metal / D3D12 / Vulkan binding
~~~

No surface may construct private project objects, curves, keyframes or render
passes. The preferred fast Agent path is a typed Intent/ChangeSet, not rewriting
Project.rfx.

A file watcher remains a compatibility adapter:

~~~text
RFX candidate
  -> parse and validate
  -> semantic diff
  -> normalize to a known ChangeSet when lossless
  -> otherwise submit a separately authorized ReplaceProject candidate
  -> use the same CAS, runtime preparation, publication and LKG rules
~~~

Therefore UI, CLI, MCP and Mobile can be intent-equivalent. A whole-file
candidate is always admission-equivalent, but is intent-equivalent only when
normalization produces the same normalized ChangeSet digest.

## Plan, PlanReceipt and Commit contract

Planning and writing must be separate operations. A client does not send a
trusted normalized diff or toggle a dry-run boolean. The server creates the
normalized plan, binds it to the accepted base and returns an opaque,
short-lived receipt.

### PlanRequest

A future request should include at least:

~~~json
{
  "schema": "refusion.authoring.plan.v1",
  "project": {
    "id": "prj_example",
    "base_revision": 42,
    "base_snapshot_sha256": "sha256:..."
  },
  "request": {
    "request_id": "req_uuid",
    "actor": {
      "kind": "ui|agent|cli|mcp|mobile|file",
      "id": "actor-id"
    },
    "origin": "client/build identity"
  },
  "contracts": {
    "engine_contract": "refusion-authoring-v1",
    "federated_catalog_digest": "sha256:...",
    "requested_target_profile": "desktop-sdr-v1"
  },
  "operations": [
    {
      "op_id": "op_1",
      "kind": "recipe.apply",
      "target": {"stable_ref": "composition:scene_02"},
      "args": {},
      "preconditions": [],
      "depends_on": []
    }
  ],
  "limits": {
    "max_operations": 256,
    "max_generated_nodes": 512,
    "max_cost_class": "interactive"
  }
}
~~~

Rules:

- schemas reject unknown fields;
- mutation targets use stable IDs, never display names;
- a name query may return candidates but ambiguous mutation fails;
- new entities use temporary batch references resolved deterministically;
- every operation has scope, dependencies and pre/postconditions;
- actor identity is audit input, not authorization;
- the requested target profile is a request only; Application derives and
  revalidates the effective qualified profile.

### Server-issued PlanReceipt

~~~text
PlanReceipt
  plan_id opaque handle
  plan_digest
  expiry
  project ID
  base revision and snapshot digest
  actor and authorization-scope digest
  federated catalog and dependency-lock digests
  normalized ChangeSet digest
  resulting semantic digest
  affected stable IDs
  diff summary
  estimated CPU/GPU/memory/pass cost
  topology postconditions
  required confirmations
  structured diagnostics
~~~

The handle is explicit application state. It is not hidden transport-session
state. Planning may return ready, rejected, conflict or input-required.

### CommitRequest

~~~json
{
  "schema": "refusion.authoring.commit.v1",
  "project_id": "prj_example",
  "expected_revision": 42,
  "plan_id": "opaque_handle",
  "plan_digest": "sha256:...",
  "command_id": "cmd_uuid",
  "idempotency_key": "idem_uuid",
  "approval_token": "optional"
}
~~~

Commit compares revision, snapshot, plan, catalog, dependency, authorization and
effective-profile bindings. It then prepares Runtime and publishes exactly one
AcceptedRevisionBundle.

Idempotency must become durable:

- key by tenant/project/actor/idempotency key;
- same request digest replays the original result after restart;
- a different request under the same key is rejected;
- result lookup resolves timeout uncertainty;
- an expired plan or stale revision returns conflict and requires replan;
- no silent rebase.

The result exposes accepted/rejected/conflict/replayed status, revision,
semantic and ChangeSet digests, affected IDs, EvaluationStamp and structured
diagnostics. Last-Known-Good remains unchanged on rejection.

## Registry architecture

Do not build one God Registry. Build a read-only Federated Capability Catalog
View over domain registries with independent lifecycle:

~~~text
Federated Capability Catalog View
  Property Registry
  Effect and Mask Registry
  Paint and Geometry Registry
  Curve Registry
  Generator Registry
  Recipe and Style Registry
  Asset Type Registry
  Installed Extension Registry
  Qualification Ledger projection
~~~

Every registry shares one DescriptorHeader and query model. The federated view
provides deterministic ordering, search, compatibility closure and a composite
digest without making one monolithic owner responsible for every domain.

Common DescriptorHeader:

~~~text
descriptor kind and stable ID
schema version
semantic version
immutable content digest
package and publisher identity
engine contract range
required capability IDs
required profile class
migration IDs
deprecation and replacement
~~~

Versions are immutable. SemVer communicates compatibility to humans; a content
digest binds exact bytes. Dependency ranges resolve only at install/apply.
Receipts pin exact resolved versions and digests. Opening a project never
resolves “latest,” recompiles automatically because a catalog changed, or
silently upgrades a recipe.

Project-schema migration, receipt migration and contribution-state migration
are separate contracts. Upgrade is an explicit planned operation with preview,
diff and a new revision.

The federated catalog describes owners, units, ranges, animation domains,
effects, paints, geometry, curves, generators, recipes, asset requirements,
bounds/cost rules, UI controls, Agent guidance and generated CLI/MCP schemas.
It contains no arbitrary executable C++, platform code or user-authored SkSL.

Qualification truth is not copied into every descriptor. Descriptors declare
requirements. One qualification ledger records evidence; effective recipe
eligibility is derived from the minimum state of all resolved dependencies.

### PresetDescriptor candidate

~~~text
PresetDescriptor
  preset_id
  semantic_version
  immutable_digest
  category
  searchable_tags and aliases
  compatible owner/content kinds
  required capability IDs
  required qualification profile
  typed ParameterDescriptor list
  declared input/output slots
  topology declaration
  exact-time and duration policy
  layout/safe-area policy
  conflict policy
  normalized recipe operations
  bounds and estimated cost class
  preview/export support
  required capability and profile classes
  migration policy
~~~

### ParameterDescriptor candidate

~~~text
ParameterDescriptor
  id
  type = number | ratio | angle | time | color | paint |
         enum | bool | seed | asset_ref | curve_ref | list
  canonical unit
  default
  hard minimum and maximum
  suggested UI minimum and maximum
  step and display precision
  animatable
  interpolation domain
  constraints and dependencies
  affects layout/bounds/render passes
  compatible owners
  cost class
  UI control hint
  Agent guidance
~~~

For materialized Recipe v1, animatable means the Recipe compiler may create or
update a normal AnimationTrack for this parameter. It does not mean the Receipt
is evaluated per frame.

A convenience Strength slider is allowed only as a deterministic projection
onto the real parameters. Professional controls remain addressable. For
example, Drop Shadow must not expose only “amount”; offset, angle projection,
opacity, softness, spread, color and blend remain available.

## Preset application, sliders and ownership

The preferred near-term representation is:

~~~text
Recipe Descriptor
        |
        | compile with exact inputs and catalog digest
        v
Atomic typed ChangeSet
        |
        v
ordinary Layers / Groups / Paints / FX / Animation
        +
PresetApplicationReceipt (non-rendering metadata)
~~~

The receipt records:

- receipt ID;
- package, publisher, preset ID, version and digest;
- compiler ID/version;
- parameter-schema and resolved-dependency-lock digests;
- canonical parameter values;
- explicit random seed;
- application context: Composition, exact time/range, Canvas, rate and profile;
- stable Slot-to-Entity mappings;
- channels/properties/effects/paths managed by the preset, each marked attached,
  detached or stale;
- per-target output digests;
- managed-materialization digest;
- topology postconditions;
- source capability/catalog digests;
- user overrides;
- created and last-applied revisions;
- migration provenance.

Materialized entities are the only visual/evaluation truth. The renderer,
Runtime and RenderPlan compiler never read a Receipt. A Receipt is authoring
provenance and ownership management only.

Updating a slider recompiles only the attached receipt-owned portion into one
atomic ChangeSet. It verifies project revision, receipt revision and managed
materialization digest. If a user manually edits a managed curve, the same
ChangeSet first detaches that channel and then applies the edit. Reapplying a
preset never touches detached work. A mismatch marks ownership stale; no silent
synchronization is permitted.

The project remains renderable from its materialized ordinary entities even if
the preset catalog is unavailable. Missing catalog data blocks preset updates,
not opening the Last-Known-Good materialized result.

Whether a fully live ParameterizedRecipeInstance should later become canonical
project truth is an open ADR decision. It must not create a second evaluator.

### Receipt lifecycle commands

- ApplyRecipe;
- UpdateRecipeParameters;
- InspectRecipeOwnership;
- DetachRecipeChannel;
- ReattachRecipeChannel with explicit confirmation;
- ResetManagedPortion;
- BakeRecipe, which removes the Receipt and preserves current visual state;
- RemoveRecipeResult, which removes only still-owned output after confirmation;
- UpgradeRecipe through plan/diff/commit;
- ResolveMissingRecipe without changing the visual state implicitly.

### Pure bounded Recipe compiler

~~~text
compile(
  immutable recipe descriptor,
  exact dependency lock,
  canonical parameters,
  stable slots,
  accepted measurement snapshot,
  exact project context
) -> Normalized Atomic ChangeSet
~~~

The compiler has no network, wall clock, OS font discovery, arbitrary
filesystem path or platform API. Resource, recursion, operation and generated
node limits are explicit. Stable entity IDs derive deterministically from the
receipt/application identity and stable recipe slot IDs.

## Animation mathematical contract

### One curve truth

The Value Graph and Speed Graph are two views of the same AnimationTrack.
Persisting them independently would permit contradictory truth.

~~~text
AnimationTrack<T>
  track_id
  owner_id + property_id
  exact half-open time range
  ordered Keyframes<T>
  SegmentCurve after each key
  blend/conflict policy
  pre/post behavior
~~~

Candidate SegmentCurve variants:

- Hold;
- Linear;
- Steps with explicit jump policy;
- Cubic Bezier;
- Piecewise Linear;
- deterministic Spring;
- later, a qualified sampled curve for imported/baked results.

For professional Speed Graph editing, a cubic segment may need temporal/value
control points, not only normalized CSS-style progress controls:

~~~text
P0 = (time0, value0)
P1 = (time0 + outgoing influence, outgoing control value)
P2 = (time1 - incoming influence, incoming control value)
P3 = (time1, value1)
~~~

Incoming/outgoing velocity and influence are derived from the same handles.
Auto Bezier, Continuous, Broken and Roving are authoring operations that resolve
to canonical handles and exact times before commit.

### Spatial versus temporal motion

Position needs separate meanings:

- SpatialPath describes the geometric path and tangents;
- TemporalProgressCurve describes progress along that path.

Changing an easing curve must not alter path geometry. Changing path geometry
must not silently replace its timing.

### Curve preset families

- Hold;
- Linear;
- Steps;
- Ease In;
- Ease Out;
- Ease In Out;
- Easy Ease;
- Smooth Continuous;
- Sharp/Snap;
- Anticipation;
- Back/Overshoot;
- Bounce;
- Elastic;
- Spring soft, snappy, bouncy, critical and heavy;
- Decay;
- Custom Cubic;
- Custom Speed/Influence;
- Custom Piecewise.

Names are search aliases; projects persist a versioned Curve ID or actual
canonical parameters.

### Spring contract

SwiftUI, Core Animation, Android or Qt springs may not become project truth.
ReFusion needs one portable evaluator with at least:

- mass;
- stiffness;
- damping or damping ratio;
- initial velocity;
- rest-value threshold;
- rest-speed threshold;
- maximum settle duration;
- interruption/retarget policy.

A spring that cannot satisfy its settle contract within the allowed duration
must fail or require an explicit loop/non-settling policy.

## Style profile taxonomy

A professional Style Profile is multi-axis rather than a single enum.

### Narrative and product context

- cinematic narrative;
- documentary/editorial explainer;
- news, broadcast and sports;
- product commercial and demo;
- corporate and brand;
- educational, tutorial and course;
- social vertical, reels and shorts;
- UGC, POV and creator;
- interview and podcast;
- music video and lyric;
- trailer and title sequence;
- data visualization and UI walkthrough;
- event, fashion, travel, food and real estate.

### Art direction

- clean/minimal/Swiss;
- editorial/magazine;
- paper collage, zine, scrapbook and cutout;
- kinetic typography;
- flat vector and isometric;
- photoreal;
- 3D CGI and product stage;
- clay, voxel, low-poly and toon;
- surreal/dreamlike;
- maximalist;
- brutalist;
- luxury/fashion;
- glass, liquid and tactile;
- tech, HUD and futuristic;
- neon and cyberpunk;
- retro, vintage, film and VHS;
- Y2K, vaporwave and synthwave;
- hand-drawn, sketch and doodle;
- comic, manga and anime;
- stop-motion;
- organic/nature;
- archive/documentary;
- children/playful;
- culturally specific/local.

### Finish

- clean digital;
- tactile/grainy;
- matte paper;
- glossy/glass;
- metallic/chrome;
- monochrome/duotone;
- high-key or low-key;
- chromatic/glitch;
- soft pastel;
- saturated/high-contrast.

### Motion language

- restrained/minimal;
- smooth cinematic;
- editorial snap;
- rhythmic or beat-synchronized;
- elastic/bouncy;
- mechanical/stepped;
- handcrafted jitter/stop-motion;
- chaotic/glitch.

Profiles select compatible defaults and constraints. They do not draw, create a
new Layer kind, clone a brand, or bypass qualification.

## Editorial Paper-Collage Explainer Style Pack

The visual language informally requested as “Vox style” must be represented by
the neutral canonical profile:

~~~text
style.video.editorial_explainer.paper_collage.v1
~~~

The informal phrase may exist as a search alias only. It must not be a Layer
kind, renderer switch, claim of affiliation, reproduction of proprietary brand
assets, or promise to clone one publisher’s identity. The engine meaning is a
general editorial explainer system built from paper, cutout, typography,
annotation, data-visualization and choreography recipes.

This is not one Effect. It is a versioned Style Pack that selects compatible
recipes and defaults while leaving every important value editable.

### Target ownership and hierarchy

The intended scene structure is:

~~~text
EditorialExplainerScene
  SceneBackgroundOwner
    PaperBase
    AmbientPrintField
    PaperTexture
  PrimaryContentOwner
    PrimaryCutout
    OptionalSecondaryCutouts
  InformationOwner
    HeadlineText
    CaptionOrLabelText
    ChartMapOrDataGraphic
  AnnotationOwner
    Arrows
    MarkerHighlights
    Underlines
    TapePinsAndStamps
  ForegroundFinishOwner
    Grain
    Halftone
    RegistrationOffset
~~~

The Main Timeline exposes one collapsed Scene or Composition row when the
accepted hierarchy capability exists. Drilling down exposes the semantic owners
and their internal content. Effects, masks, Text selectors, path operators and
animation channels remain inside their owners and never masquerade as
independent root Layers.

Until nested Composition semantics are accepted, a bounded LayerGroup may prove
the ownership model, but the project must not claim full precomposition
behavior.

### Style Pack catalog

The pack may reference these independently versioned descriptors:

#### Scene and background

- scene.editorial_explainer.paper_collage.v1;
- background.editorial.paper_clean.v1;
- background.editorial.paper_recycled.v1;
- background.editorial.newspaper.v1;
- background.editorial.cardboard.v1;
- background.editorial.modular_collage.v1.

#### Image and cutout

- media.cutout.subject.v1;
- media.cutout.rough_edge.v1;
- media.cutout.white_border.v1;
- media.cutout.duotone.v1;
- media.cutout.halftone.v1;
- media.cutout.photocopy.v1;
- media.cutout.taped_photo.v1;
- media.cutout.layered_parallax.v1.

#### Typography

- text.editorial.headline_bold.v1;
- text.editorial.cutout_letters.v1;
- text.editorial.label.v1;
- text.editorial.caption.v1;
- text.editorial.marker_highlight.v1;
- text.editorial.underline_draw.v1;
- text.editorial.typewriter.v1;
- text.editorial.word_build.v1.

#### Annotation and information

- annotation.arrow_hand_drawn.v1;
- annotation.callout.v1;
- annotation.marker.v1;
- annotation.tape.v1;
- annotation.pin.v1;
- annotation.stamp.v1;
- data.editorial.bar_chart.v1;
- data.editorial.line_chart.v1;
- data.editorial.map_callout.v1;
- data.editorial.timeline.v1.

#### Motion and transition

- motion.editorial.cutout_pop.v1;
- motion.editorial.paper_slide.v1;
- motion.editorial.image_stamp.v1;
- motion.editorial.collage_build.v1;
- motion.editorial.stop_motion_jitter.v1;
- motion.editorial.camera_pan_zoom.v1;
- motion.editorial.layered_parallax.v1;
- motion.editorial.annotation_draw.v1;
- transition.editorial.paper_wipe.v1;
- transition.editorial.paper_tear.v1;
- transition.editorial.newspaper_slide.v1;
- transition.editorial.halftone_dissolve.v1.

These IDs are research candidates, not current registered capabilities.

### Parameter contract

The Style Pack must not reduce to a fixed look. Its top-level parameters should
project deterministically into the real recipe parameters:

~~~text
paper_style
paper_color
paper_texture_asset optional
paper_fiber_amount
paper_grain_amount
paper_grain_scale
paper_age
edge_roughness
cutout_border_width
cutout_shadow_offset
cutout_shadow_sigma
halftone_cell_size
halftone_angle
registration_offset
palette
accent_color
typography_profile
headline_scale
caption_scale
annotation_density
motion_language
motion_intensity
scene_duration
beat_overlap
camera_depth
parallax_amount
transition_recipe
explicit_seed
~~~

Every macro parameter has a declared mapping. Advanced Inspector sections still
expose the underlying values. For example, the user may adjust shadow offset,
sigma, opacity and color individually even when the Style Pack exposes a
convenience cutout-depth control.

### Paper and print construction

A paper surface is either:

1. a native registered procedural material with algorithm version and seed; or
2. an immutable scanned texture Asset with digest and provenance.

Candidate controls include base color, fiber amount and scale, flecks, grain,
roughness, stains, folds, edge roughness and print contrast. The Style Pack must
not fetch a texture during rendering or depend on a temporary URL.

Print finishing may combine registered Grain, Halftone, Posterize, Duotone,
slight channel/registration offset and bounded Roughen Edges. These remain
owner-local effects or material operations with explicit color, bounds and
cost contracts.

### Cutout construction

A cutout is one media owner containing:

- an immutable Image or SVG Asset reference;
- an explicit mask or extracted subject path;
- optional rough/torn-edge operation;
- optional white or colored border;
- optional paper texture;
- owner-local Drop Shadow;
- color treatment such as duotone, photocopy or halftone;
- owner-local transform and animation.

Automatic background removal is an external preparation operation. Its result
is ingested as an immutable mask/asset candidate and accepted through a typed
ChangeSet. It is not rerun on every frame.

### Text and annotation construction

Headlines, labels and captions use qualified packaged font bytes and the common
Text layout path. Arabic and mixed RTL/LTR content follows cluster-safe
selection and explicit reading order.

Hand-drawn arrows, markers and underlines are vector paths or Text decorations
with Trim-Path-style reveal. They remain internal to the Annotation or Text
owner. A marker highlight behind a headline is not a new unrelated root Layer.

### Motion language

The default editorial motion language is beat-oriented:

~~~text
establish background
  -> introduce primary cutout
  -> settle
  -> reveal headline
  -> draw annotation
  -> introduce supporting media or data
  -> hold for reading
  -> emphasize the current idea
  -> transition to the next scene
~~~

Preferred curves by role:

- Editorial Snap or strong Ease Out for cutout and headline entrances;
- short, bounded overshoot for a Pop or Stamp;
- Hold/Steps for intentional stop-motion cadence;
- smooth seeded noise for subtle handmade jitter;
- Linear for uninterrupted camera drift or parallax;
- Cubic Ease InOut for bounded camera moves;
- Trim progress with Ease Out for arrows and marker strokes;
- no large perpetual Spring on the complete background;
- exact half-open loop ranges and authored seeds.

The motion remains parameterized by duration, delay, overlap, amplitude,
direction, curve, overshoot, jitter rate, camera scale, parallax depth and
transition choice.

### Choreography contract

The Scene Recipe compiles semantic relations rather than guessing absolute
placement:

~~~text
background starts at scene frame 0
primary cutout enters after establish cue
headline enters after primary settles plus an authored frame offset
annotation begins after headline readability starts
secondary media enters only after the primary beat is established
exit begins after minimum reading hold
transition owns the final bounded overlap
~~~

Before commit, the AuthoringService resolves these relations to exact frames,
checks the title-safe and action-safe regions, measures cutout focal bounds,
tests text readability, rejects unwanted collisions and publishes one explicit
sibling order.

### Agent intent example

~~~json
{
  "operation": "ApplyStylePack",
  "owner_id": "cmp_scene_02",
  "style_id": "style.video.editorial_explainer.paper_collage.v1",
  "parameters": {
    "paper_style": "newspaper",
    "palette": ["#F1E6CE", "#111111", "#E43A32"],
    "motion_language": "editorial_snap",
    "paper_grain_amount": 0.32,
    "edge_roughness": 0.18,
    "parallax_amount": 0.12,
    "transition_recipe": "transition.editorial.paper_wipe.v1",
    "explicit_seed": 9137
  },
  "slots": {
    "primary_media": "asset_primary_subject",
    "headline_text": "Why cities keep growing",
    "supporting_media": ["asset_map", "asset_chart"]
  }
}
~~~

The Agent may later request:

- reduce paper grain;
- change the accent palette;
- slow the primary entrance;
- reveal the Arabic headline word by word;
- reduce torn-edge roughness;
- make camera motion calmer;
- replace one cutout Asset.

Each request updates the receipt-owned parameters or channels in one atomic
ChangeSet. It does not rebuild the scene from prose or rewrite the entire
project.

### Required capability spine

The complete Style Pack depends on:

- AssetId/digest-based Image and SVG ingestion;
- masks and arbitrary vector paths;
- Paint, Stroke and owner-local Layer Styles;
- paper/grain/halftone registered materials;
- typed curves and exact-time animation;
- Text Animator/Selector with Arabic-safe clusters;
- Trim Paths or an equivalent registered path-reveal operation;
- LayerGroup and later accepted Composition ownership;
- measurement, safe-area and choreography constraints;
- common RenderPlan execution and preview/export parity.

Therefore the full pack is not a current capability. A first bounded experiment
may use only already qualified primitives, but must identify itself as a reduced
profile and fail on unsupported options.

### Performance rules

- paper and cutout Assets are decoded and cached by digest;
- static masks, paths, Paints and filter stacks are compiled at revision
  acceptance;
- no image generation, background removal, font discovery or network access
  occurs per frame;
- static internal children may be cached as one local surface when semantics
  permit;
- transform-only motion reuses owner-local content;
- effect isolation has deterministic bounds;
- texture and pass budgets are admitted before publication;
- preview and export consume the same semantic graph.

### Cross-platform qualification

No part of the pack may be implemented as a macOS-only visual shortcut.
Descriptors, recipe compilation, layout, curve evaluation, masks, color and
effect order are shared. Platform code only binds Metal, D3D12 or Vulkan
resources and presents the already-defined result.

Qualification requires the same project and Style Pack receipt to produce:

- identical normalized ChangeSet and semantic digest;
- identical owner hierarchy and exact key times;
- qualified packaged-font shaping;
- matching RenderPlan digest;
- pixel results within the calibrated backend tolerance;
- matching preview/export ownership and color policy;
- declared performance evidence on required physical devices.

### Acceptance demonstration

The pack is not accepted until a reference editorial scene proves:

1. one collapsed Scene/Composition row in the Main Timeline;
2. drill-down reveals named semantic owners, not fake FX Layers;
3. UI and Agent apply the same intent and produce the same revision;
4. paper, cutout, text, annotation and transition parameters remain editable;
5. Arabic and Latin headlines shape and animate correctly;
6. reopening preserves IDs, receipt, parameters and semantic digest;
7. unsupported capabilities fail without damaging Last-Known-Good;
8. macOS and Windows render within the accepted tolerance;
9. preview and export show the same scene meaning;
10. the qualified scene stays inside declared CPU, GPU and memory budgets.

## Background system taxonomy

A Background is normally one semantic owner row. Its internal base, ambient,
texture, light and accent slots remain children/content of that owner or group.
They do not become unrelated root Timeline layers.

### Native paint backgrounds

- transparent;
- solid;
- duotone base;
- linear gradient;
- radial/focal radial gradient;
- sweep/conic gradient;
- two-point conical gradient;
- repeating gradient;
- hard-stop/striped gradient;
- later, qualified multi-point/freeform/mesh gradient.

Gradient parameters require stable stop IDs, offset, color, alpha, optional
midpoint, geometry, tile mode, interpolation space, hue path, alpha policy and
banding/dither policy. Morphing gradients with incompatible types or stop sets
requires an explicit rule.

### Pattern backgrounds

- grid and checkerboard;
- stripes, crosshatch and scanlines;
- dots, stipple, halftone and dithering;
- chevron, zigzag, brick and honeycomb;
- circles, rings, rays, radial bars and sunburst;
- waves, ribbons and contour lines;
- topographic maps and guilloche;
- truchet and tessellation tiles;
- polygon and low-poly fields;
- circuit and HUD fields;
- isometric grids;
- confetti and starfields;
- repeating icon, logo or image tile.

Parameters include cell size, spacing, line width, angle, phase, palette, shape,
density, duty cycle, local transform, tile mode, bounds, antialias policy and
explicit seed where variance exists.

### Composited geometric backgrounds

- abstract shapes and blobs;
- metaballs;
- waves and ribbons;
- arcs and rings;
- polygon facets;
- cut-paper stacks;
- collage, zine and scrapbook;
- modular editorial layout;
- UI/data panels and HUD.

A recipe may expose slots:

~~~text
background.base
background.ambient
background.texture
background.light
background.foreground_accent
~~~

Slot order and ownership are explicit postconditions.

### Procedural fields and textures

- value/white noise;
- Perlin/fBm/turbulent noise;
- Voronoi/Worley/cellular;
- bands, waves and rings;
- curl and flow fields;
- domain-warped noise;
- marble and wood;
- clouds, plasma, aurora, smoke and fog;
- reaction-diffusion;
- signed-distance fields;
- grain and paper fibers;
- animated organic liquid fields.

Every generator requires algorithm ID/version, seed, frequency/scale, octaves,
lacunarity, gain/persistence, contrast, threshold, warp, phase/evolution, tile
period, loop period and resource budget where applicable.

Project-authored arbitrary SkSL is prohibited. Registered engine-owned,
versioned runtime-effect descriptors may lower through the shared compositor.

### Particles and instances

- dust, snow and rain;
- bubbles;
- confetti, sparks and embers;
- stars and bokeh;
- floating glyphs/icons;
- flock and flow-field particles.

The contract needs emitter, count/rate, lifetime, pre-roll, motion fields,
distributions, sprite/shape, blend, explicit seed, spawn phase, depth/parallax,
simulation step/version and conservative bounds.

### Optical and lighting backgrounds

- vignette;
- bokeh;
- light leaks;
- flare and glints;
- light/god rays;
- spotlight;
- caustics and ripples;
- chromatic glow and bloom;
- volumetric-looking beams.

These are typed recipes with explicit source positions, colors, intensity,
falloff, threshold, blur, scatter, phase, blend and cost. A one-slider “water”
or “light” effect is not a sufficient semantic contract.

### Imported or generated asset backgrounds

- raster image;
- SVG/vector asset;
- layered import when supported;
- image sequence;
- animated raster;
- Lottie/vector-animation source when supported;
- video loop or alpha video;
- panorama/HDRI;
- stock or Agent-generated image/video.

The project stores AssetId, immutable digest and qualified metadata, never a URL
or absolute OS path as project truth.

### Composition and generative scene backgrounds

- parallax matte painting;
- layered 2.5D scene;
- pre-rendered 3D scene;
- procedural 3D/volume only after an accepted capability;
- data-driven visuals;
- audio-reactive visuals driven by a deterministic baked Control Track.

Live microphone/network callbacks are not project truth.

### Background motion presets

- pan, drift, rotate and orbit;
- zoom and Ken Burns;
- parallax drift;
- tile scroll in any direction;
- gradient stop travel;
- gradient point orbit;
- angle rotation;
- palette/stop morph;
- aurora flow;
- noise evolution and turbulence advection;
- domain warp and ripple propagation;
- particles drift/rise/fall/swirl;
- starfield travel and bokeh float;
- light sweep and rotating rays;
- vignette breathe and glow pulse;
- seamless media loop, ping-pong, reverse and freeze;
- camera orbit/dolly/truck;
- hue cycle and exposure breathe;
- wipe, iris, luma, liquid, paper and glitch reveals;
- beat/amplitude/frequency-band response from baked controls.

Professional defaults depend on purpose:

- continuous scroll, rotation and evolution normally use Linear phase to avoid
  visible slowing at a loop seam;
- bounded entrance uses Ease Out;
- bounded exit uses Ease In;
- morph/crossfade/camera move normally uses smooth Cubic;
- strobe/glitch uses Hold or Steps;
- Spring/Bounce is normally for accents, not large perpetual ambient motion;
- random drift uses smooth seeded noise, never a new random sample each frame;
- loops use exact cycle closure and a half-open range without a duplicate
  terminal frame.

## Text system taxonomy

### Legal ownership

~~~text
TextLayer
  TextDocument
    logical UTF-8 text
    paragraphs and styled spans
    font/fallback identities
    shaping configuration
  TextAnimatorGroups
  TextDecorations
  LayerEffectStack
  LayerTransformAnimations
~~~

Text effects, selectors and animation remain inside the Text Layer. Per-character
or per-word animation does not create top-level Layers. Underline, marker and
highlight remain text decorations or internal children under one owner.

### Typography controls

- packaged font and fallback AssetIds/digests;
- family, face, style, weight, width and slant;
- variable-font axes and named instances;
- script and language;
- font size, scaling and baseline shift;
- OpenType features;
- fill/stroke paint and ordering;
- tracking, kerning and leading;
- point text or paragraph text;
- TextBox dimensions and padding;
- horizontal and vertical alignment;
- base direction and writing mode;
- indents, paragraph spacing and tabs;
- wrap, overflow, ellipsis, fit/shrink and maximum lines;
- hyphenation and kashida policy;
- text on path.

### International and Arabic correctness

- store logical UTF-8, never visually reordered text;
- apply the Unicode Bidirectional Algorithm;
- default character unit is an extended grapheme or shaping cluster, not a code
  point;
- do not separate Arabic marks, ZWJ sequences or ligatures;
- word and line boundaries use ICU/Unicode rules, not splitting on ASCII space;
- fallback fonts must be byte/digest qualified;
- mixed RTL/LTR reveal order must be explicit;
- variable axes must exist in the selected font and remain in its declared
  range.

Arabic Typewriter requires two explicit modes:

1. final-layout mask reveal, which shapes the full text then reveals clusters;
2. live-prefix shaping, which reshapes each logical prefix for natural joining.

### Text Animator and selectors

Selector basis:

- grapheme/shaping cluster;
- characters excluding spaces;
- word;
- line;
- all.

Order:

- logical;
- visual/reading;
- reverse;
- center-out;
- edges-in;
- seeded random.

Selector shapes and controls:

- square, ramp up, ramp down, triangle, round and smooth;
- start, end, offset, amount and smoothness;
- ease high/low;
- include whitespace;
- explicit seed;
- Wiggly selector frequency, correlation, phase and amplitude.

Animator properties:

- anchor and position;
- scale and rotation;
- skew and skew axis;
- opacity;
- fill/stroke color, paint, opacity and width;
- tracking and baseline;
- blur;
- line spacing;
- qualified variable-font axes.

### Text motion families

- fade in/out by layer, cluster, word or line;
- directional slide;
- scale/grow/shrink;
- blur-to-sharp;
- clip/mask wipe;
- typewriter;
- word and line build;
- tracking expand/contract;
- scramble/decode;
- marker/highlight sweep;
- underline write-on;
- pop, bounce and elastic;
- swing and 2D flip;
- wave, wiggle and jitter;
- breathe, pulse and float;
- color cycle and Glow pulse;
- strobe/flicker;
- kinetic stack and slam;
- karaoke fill;
- path follow;
- glitch slice and RGB split;
- numeric counter;
- exit reversals and emphasis loops.

Common parameters include phase, exact start/duration/delay, stagger, overlap,
basis, order, curve, strength, seed and Motion Blur profile when that capability
is qualified.

### Text visual styles

- solid and gradient;
- outline and double outline;
- Drop/Inner Shadow;
- Outer/Inner Glow;
- neon;
- bevel/emboss/letterpress 2D;
- sticker and paper cut;
- long shadow 2D;
- chrome/gold/holographic 2D;
- qualified glass;
- comic halftone;
- grunge;
- retro offset;
- glitch RGB.

These use the same Paint and Layer-Style descriptors as Shape; Text does not get
a second effect engine.

## Shape system taxonomy

### Geometry primitives

- rectangle and per-corner rounded rectangle;
- ellipse and circle;
- line and polyline;
- polygon and star;
- arc, pie and ring;
- capsule and arrow;
- speech bubble/callout;
- cross and heart;
- Bezier path;
- imported SVG path.

Each descriptor declares stable points, dimensions, corner smoothing, radii,
angles, closure, fill rule and complexity limits as applicable.

### Path and geometry operators

- group;
- boolean union, subtract, intersect and xor;
- offset path;
- round corners;
- Trim Paths;
- dash;
- repeater;
- pucker/bloat;
- twist;
- zigzag;
- wiggle path;
- wiggle transform.

Order matters and remains explicit in the Shape content graph. Operators are
internal content operations, not Timeline Layers.

### Paint and Stroke

Paint kinds:

- solid;
- linear, radial, sweep and two-point conical gradients;
- image pattern;
- registered procedural paint.

Stroke parameters:

- paint and opacity;
- width;
- center/inside/outside alignment;
- cap and join;
- miter limit;
- dash array/offset;
- trim and taper.

Inside/outside stroke behavior needs a shared geometry/clipping contract; it
must not be reinterpreted separately by Metal, D3D12 or Vulkan.

### Shape materials and visual recipes

- transparent tint;
- solid;
- gradient;
- outline;
- duotone;
- neon;
- clay 2D;
- glossy/plastic 2D;
- metal/chrome/gold 2D;
- bevel/emboss 2D;
- sticker;
- paper cut and letterpress;
- paper/cardboard;
- grain/noise;
- halftone/comic;
- pattern/pixel/grunge;
- iridescent/holographic 2D;
- texture assets such as fabric, wood and stone;
- qualified clear/frosted/acrylic/liquid glass.

Glass is not Gaussian Blur on the owner. It needs a declared backdrop input,
group isolation, render-pass dependency, deterministic bounds/crop, color
policy, cache policy and backend qualification.

True extrusion, depth, PBR materials, environment lighting and mesh deformation
are a separate 3D capability. A 2D approximation must be named with a 2D suffix
and never silently substitute for real 3D.

## Shared effect catalog and parameters

Candidate shared effects:

- Gaussian Blur;
- Drop Shadow;
- Inner Shadow;
- Outer Glow;
- Inner Glow;
- Bevel/Emboss 2D;
- Satin;
- Color/Gradient/Pattern Overlay;
- Stroke;
- Color Matrix;
- Morphology dilate/erode;
- Displacement;
- Posterize;
- Pixelate;
- Roughen Edges.

### Drop Shadow

Canonical parameters should include:

- offset X/Y as authored truth;
- angle/distance as an Inspector projection, not duplicate persisted truth;
- color;
- opacity;
- sigma X/Y or declared softness unit;
- spread/choke;
- contour;
- noise;
- blend mode;
- shadow-only;
- composite/knockout placement;
- edge, tile and crop policy.

### Glow

- basis/threshold;
- radius/sigma;
- intensity;
- color or gradient;
- opacity;
- spread;
- blend/composite placement;
- inner/outer selection;
- dimensions and quality tier;
- edge and crop policy.

### Blur

- radius/sigma X/Y;
- blur kind;
- direction where applicable;
- quality/iterations;
- edge/tile/crop policy;
- explicit isolation bounds.

All bounds expansion and pass cost must be declared before admission.

## Asset acquisition and generation

Agent search, download and generative-image/video actions are external,
permissioned tools. They do not execute inside frame evaluation.

The safe flow is:

~~~text
authorized provider/search/generation request
        |
        v
candidate bytes + source/provenance/license metadata
        |
        v
asset.ingest validation
        |
        v
project-local immutable AssetId + digest
        |
        v
typed project ChangeSet references the AssetId
~~~

Ingest validates MIME/codec/profile, size, dimensions, alpha, color profile,
malware/resource limits, license/provenance and project permissions.

For generated media, retain provider, model/version where available, prompt or
a policy-approved redacted prompt, generation time, safety/privacy decision and
content credentials such as C2PA when available.

URLs, temporary provider handles and absolute paths are not project truth.
Network access, generation and licensing are separate permissions from editing
a project.

## Scene choreography and the Agent’s digital eye

The Agent does not infer placement from Timeline pixels or layer names. It reads
stable IDs and engine measurements in project coordinates.

### Semantic roles

- background;
- primary;
- secondary;
- headline;
- caption;
- annotation;
- accent;
- overlay.

### Timing relations

- start_with;
- start_after;
- finish_with;
- overlap_by;
- align_to_cue;
- stagger;
- wait_until_settled.

Relations form an acyclic dependency graph and resolve to exact frames and
half-open ranges before commit.

### Scene phases

- establish;
- enter;
- hold/read;
- emphasize;
- exit.

### Layout and readability contract

Before commit:

- read Canvas width, height, aspect, exact frame rate and safe areas;
- inspect only required owners and properties;
- measure geometry, logical, ink, effect and world bounds;
- use explicit anchors and alignment constraints;
- enforce minimum readable size and text hold time;
- honor asset focal/subject bounds when available;
- solve allowed overlap and non-overlap constraints;
- reject clipping or occlusion that violates the recipe;
- assign one explicit sibling order;
- sample critical key times and conservative swept bounds;
- run bounded render probes;
- verify topology and resource budgets.

Semantic z-order bands may guide planning, but the accepted project has one
legal child order, not a parallel z-index truth.

“Make it slower” updates duration, speed multiplier or the selected curve and
regenerates receipt-owned channels atomically. “Move it here” resolves a real
project-coordinate target and updates the owner’s canonical transform. It never
edits Timeline display pixels.

## Choreography example

~~~text
roles:
  background: establish at frame 0
  primary: enter after background
  headline: enter after primary has settled + 3 frames
  callout: enter after headline read-start
  accent: emphasize with headline

constraints:
  headline inside title-safe area
  callout does not overlap primary focal bounds
  minimum headline hold = 42 frames
  background stays below all content
  all random operations use seed 9137
~~~

The compiler resolves this description to exact ranges, owners, channels and
postconditions in one ChangeSet.

## MCP surface

Do not create one MCP Tool per preset; that makes discovery, context and version
management unbounded. Use a compact generic surface:

- refusion_project_context;
- refusion_catalog_search;
- refusion_inspect;
- refusion_measure with instant or swept mode;
- refusion_plan_changeset;
- refusion_commit_plan;
- refusion_operation_status;
- refusion_diagnostics;

Separately authorized extensions:

- refusion_asset_ingest;
- refusion_render_probe;
- refusion_asset_generate.

Preset catalogs, schemas and examples are paginated MCP Resources:

~~~text
refusion://projects/{id}/context
refusion://projects/{id}/outline
refusion://projects/{id}/nodes/{stable-id}
refusion://catalog/{digest}/search
refusion://schemas/authoring/{version}
refusion://examples/{intent-id}
refusion://diagnostics/{project-id}
~~~

Optional MCP Prompts provide user-invoked workflow templates and have no write
authority. Tools use strict input/output JSON Schemas and structured content.
List/read results have deterministic ordering, opaque cursors and cache hints.

MCP 2026 has a stateless protocol core. ReFusion therefore returns explicit
Plan and operation handles and requires the model to pass them into later
calls. It never relies on a hidden transport session for project state.

Every write is server-planned. The client submits intent; it does not submit a
trusted normalized diff. Commit includes project/revision, Plan handle/digest,
idempotency key and approval when required.

MCP is an adapter over AuthoringService, not another authority. Transport and
authorization must follow the negotiated protocol version and project-scoped
permissions.

Candidate scopes:

- project.read;
- project.measure;
- project.plan;
- project.write.properties;
- project.write.animation;
- project.write.topology;
- asset.read;
- asset.ingest;
- render.probe;
- asset.generate.

Whole-project replacement, topology changes and network generation require
stronger scope and may require explicit user confirmation.

Project text, Layer names, imported metadata, Style-Pack descriptions and
third-party catalog content are untrusted data. They never override Skill,
policy or Tool instructions.

### Diagnostics, security and recovery

One diagnostic schema serves Studio, CLI, MCP, files and project diagnostics:

~~~text
code
severity and blocking
phase
message
retryable
remediation
JSON pointer or source location
operation ID
related stable IDs
base and current revision
required capability and scope
trace ID
~~~

Remote authorization needs issuer/audience validation, short-lived tokens,
credential isolation, least privilege and no token passthrough. Local and
remote adapters enforce request size, schema depth, operation count, timeouts,
cancellation, rate limits and output pagination.

Paths are canonicalized and confined to the project/approved asset portals;
symlink escape and traversal fail. Asset ingest verifies magic/MIME, digest,
dimensions, decompression limits, color/alpha metadata, license and provenance.
Diagnostics and audit records do not include tokens, unrestricted absolute
paths, private source content or the complete Agent prompt by default.

Audit binds actor, origin, scopes, Plan digest, accepted/rejected revision and
result. Operation status plus durable idempotency lets an Agent recover after a
timeout without repeating or guessing whether a write succeeded.

## Mobile and cloud authoring

On iOS and Android:

- the app sandbox owns project files;
- a remote Agent receives a compact project snapshot/diff, not unrestricted raw
  filesystem access;
- intents are project-scoped and permission-scoped;
- assets are uploaded/ingested separately and referenced by AssetId/digest;
- an offline Agent may produce a signed, nonce-bound intent envelope for
  re-planning and approval on import;
- arbitrary native C++, dynamic libraries and unsigned plugins are prohibited;
- the same schema, digests, CAS, diagnostics and Revision authority apply;
- MCP or a connector is only a gateway to the same AuthoringService.

This design can be prepared before G9, but product implementation belongs to
G9.

An offline envelope binds issuer, key ID, subject, project, base revision and
snapshot digest, Registry/Catalog/profile digests, operation and Asset-manifest
digests, nonce, issue/expiry times, scopes and signature. Import verifies trust,
expiry and replay, resolves capabilities/assets again, replans against the
active revision, displays the diff where required and commits through the same
CAS. A signature never bypasses validation or permissions.

## Project-local generated Agent package

The project should receive a compact, generated instruction package bound to
the engine/catalog digests. It must not copy the entire global preset catalog
into every project or prompt.

~~~text
AGENTS.md
.agents/
  skills/
    refusion-project-authoring/
      SKILL.md
      references/
        contracts.json
        capabilities.json
        registry-digests.json
        schemas/
        examples/
.refusion/                         host-local and gitignored
  agent-context.json
  cache/                           revision + digest + TTL bound
  diagnostics/
~~~

Generated guidance contains:

- the short safe workflow;
- supported capability/profile discovery contract;
- Registry and Catalog binding rules;
- compact command schemas;
- how to outline, inspect, measure, plan and commit;
- examples generated and executed in CI;
- forbidden paths and fail-closed rules;
- links or MCP Resource identifiers for detailed searchable catalogs.

Dynamic project ID, revision, outline, selection and measurement are never
portable static instruction files. They are read live or cached only under
.refusion with revision, digest and TTL binding. A stale cache is ignored.

The Agent should outline first, inspect selected IDs, search the catalog by
intent, measure only relevant owners/times, plan a compact ChangeSet and commit
the server-issued Plan once. It should not reread or rewrite the whole project
for a local change.

## Performance and token discipline

Authoring responsiveness requires:

- server-side catalog search and compatibility filtering;
- compact project outline followed by selective inspection;
- digest caching of Skill, Registry and Catalog;
- one preset/recipe intent instead of hand-written keyframes;
- atomic batching for a complete scene;
- compiled recipe cache keyed by recipe/compiler/parameter-schema digests,
  exact dependency lock, canonical parameters, Slot bindings, Asset/Font/
  Plugin digests, seed, Canvas/rate/profile and engine evaluator contract;
- affected-ID diffs rather than whole-project echo;
- paginated diagnostics/resources;
- examples split by content kind;
- revision-time validation and compilation, not repeated string dispatch per
  frame;
- static geometry/paint/FX caching where valid;
- instrumented authoring, evaluation, GPU and present timings.

Initial screening budgets to validate later:

| Operation | Candidate budget |
|---|---:|
| Project context response | at most 8 KiB or approximately 1,500 tokens |
| Outline page | at most 100 nodes and 16 KiB |
| Inspect response | at most 25 requested properties and 12 KiB |
| Catalog search | at most 20 hits and 12 KiB |
| Diagnostics page | at most 50 records and 12 KiB |
| Core MCP Tool surface | approximately 8 and at most 12 stable Tools |
| Engine-only simple property ChangeSet p95 | under 100 ms |
| Engine-only bounded preset/scene batch p95 | under 250 ms |
| Catalog search p95 | under 100 ms on local indexed catalog |
| 60 fps total frame budget | 16.67 ms |
| CPU evaluate/lower p95 at 60 fps | at most 2 ms for the qualified scene tier |
| GPU p95 at 60 fps | at most 12 ms for the qualified scene tier |

LLM thinking, network asset generation and provider latency are measured
separately from engine apply latency. Telemetry records response bytes/tokens,
cache hit rate, generated operations, plan/commit latency, conflict/retry rate
and trace ID.

## Cross-platform completion rule

Adding a preset is not complete when it merely appears in one Inspector or
renders on macOS. A qualifying preset contribution must include:

1. versioned descriptor and typed parameters;
2. owner compatibility and conflict policy;
3. Command/ChangeSet support;
4. validation, limits and fail-closed diagnostics;
5. canonical persistence and migration;
6. Registry/catalog generation and digests;
7. deterministic evaluator or recipe compiler;
8. RenderPlan lowering;
9. common Skia execution with no platform semantic branches;
10. preview/export parity;
11. bounds, color, alpha, edge, tile and crop policy;
12. UI, CLI and MCP intent parity, plus file intent parity only for lossless
    normalization and admission parity otherwise;
13. generated Skill guidance and executable examples;
14. semantic and pixel goldens;
15. macOS, Windows and required mobile-canary evidence;
16. performance/resource-budget qualification;
17. device-loss, missing-asset and unsupported-capability behavior;
18. documentation and capability matrix update.

The qualification status is derived from one ledger and all dependency
receipts. It is not copied into every PresetDescriptor. For a target profile:

~~~text
recipe status =
  minimum state of compiler, primitives, effects, fills/generators,
  Text/Font/Assets, output/color contract and backend evidence
~~~

Application derives the effective profile; a client cannot select a profile
that bypasses missing qualification. Candidate preparation uses
capability-declared critical times, swept bounds and resource probes, not only
the current playhead frame.

Metal, D3D12 and Vulkan files may bind devices, targets, synchronization and
presentation only. They may not redefine recipe, curve, color, text, effect or
compositing meaning.

Public third-party plugins add further requirements: ABI/API versioning,
manifest, capability declaration, isolation/sandbox policy, signing,
deterministic behavior, preview/export support and per-platform binaries or a
portable declarative graph. Public plugin/SDK implementation remains G10.

## Fallback ladder

The Agent must use this ladder:

1. an existing typed intent;
2. a parameterized preset;
3. a recipe composed from registered descriptors;
4. a custom declarative graph only when that graph capability is accepted;
5. a proposal to add a new engine or plugin capability.

At level 5 the Agent stops editing the user project and opens a separately
reviewed development path. It does not inject C++, dynamic libraries, arbitrary
shaders or executable scripts into the project.

An imported/generated immutable image or video may be an explicit user-approved
alternative when editability is not required. It must never be a silent
approximation of a requested native effect.

## Rejected paths

| Rejected path | Required path |
|---|---|
| Claim every creative style is enumerated | Extensible, versioned taxonomy with measured coverage |
| One Layer kind for every style | Small primitives plus recipes and profiles |
| One MCP Tool for every preset | Compact tools plus searchable resources |
| Put all presets in every prompt | Digest-bound search and progressive disclosure |
| Rewrite Project.rfx for each edit | Typed atomic ChangeSet |
| Store Value Graph and Speed Graph separately | One curve, two views |
| Use native platform animation as truth | One exact-time portable evaluator |
| Represent FX/animations/letters as top-level Layers | Owner-local stacks/channels/selectors |
| Generate a new random value every frame | Authored seed and registered deterministic algorithm |
| Use floating seconds as authored truth | Exact frames/time and named rounding |
| Use QML/Timeline pixels as geometry | Project-space Canvas and measurement contracts |
| Blur a translucent Shape and call it Glass | Backdrop/isolation capability |
| Store URL or absolute path as source | AssetId, digest and project-local bytes |
| Run AI generation during evaluation | Generate externally, validate and ingest once |
| Allow Agent-written C++/SkSL in a project | Registered capability or reviewed plugin development |
| Silently approximate unsupported work | Structured rejection, explicit alternative and LKG |
| Reimplement an effect in Metal/D3D/Vulkan | Shared descriptor, evaluator, RenderPlan and common Skia |
| Treat a style brand name as a renderer contract | Neutral searchable style profile |

## Fail-closed conditions

Reject before publication when:

- preset ID/version/digest or compiler is unknown;
- registry/catalog digest does not match;
- owner kind or topology is incompatible;
- parameter type, unit, range or relationship is invalid;
- a property is not animatable;
- a required capability is not qualified for the target profile;
- an asset, font, texture, SVG or plugin is missing or digest-invalid;
- license/provenance policy fails;
- random behavior has no explicit seed;
- a Text selector splits a grapheme or shaping cluster;
- RTL/mixed-direction order is undefined;
- natural Arabic Typewriter is requested without prefix reshaping support;
- Glass is requested without backdrop/isolation;
- a procedural shader is not engine-owned and versioned;
- real 3D is requested when only a 2D approximation exists;
- bounds, glyph, path, pass, memory or simulation limits are exceeded;
- gradient/path/boolean topology is invalid;
- a spring does not settle under its declared policy;
- a seamless loop does not close;
- two presets conflict on one channel without an allowed resolution policy;
- UI/file/MCP normalization produces different topology for the same intent.

The response is a structured diagnostic. The accepted revision, Canvas,
Timeline and Inspector remain on Last-Known-Good.

## Historical screening questions and UCAS-WP01 disposition

The questions below are preserved as the pre-plan review record. They no longer
describe an unplanned next step: [`UCAS-WP01`](../plans/unified-creative-authoring/work-packages/UCAS-WP01-decision-package.md)
owns their formal ADR disposition, and the dependent UCAS work package remains
blocked until its answer is accepted or explicitly deferred.

1. Accept that materialized entities are the only visual truth and the
   PresetApplicationReceipt is authoring-management truth only.
2. Accept the full Apply/Update/Detach/Reattach/Bake/Remove/Upgrade lifecycle.
3. Accept PlanRequest, server PlanReceipt, CommitRequest and durable
   idempotency/result lookup.
4. Define the canonical multi-operation ChangeSet and temporary/stable ID
   derivation.
5. Accept the Federated Capability Catalog, Common DescriptorHeader,
   dependency-lock and separate migration contracts.
6. Decide the bounded Recipe IR and package/signature/resource-limit contract.
7. What is the canonical cubic tangent representation for exact cross-toolchain
   behavior?
8. Which curve subset enters the first qualified release: Hold, Linear,
   Cubic, Steps and Spring, or a smaller staged set?
9. What is the canonical Text selector unit: grapheme cluster, shaping cluster,
   or a mode selected by the preset?
10. Which first generator catalog is small enough to qualify completely?
11. Does Glass/backdrop enter the first G3 slice or a later bounded work package?
12. Which color interpolation spaces and gradient policies are accepted first?
13. How are recipe conflict, ownership, detach and migration represented in the
   canonical project format?
14. What asset-generation providers, licensing and privacy metadata are allowed?
15. What MCP transport, protocol-version policy and authorization model are
    accepted?
16. What is the mobile gateway and signed offline intent-import model?
17. Which style/workflow corpus defines the future 60–70% coverage claim?
18. How are public downloadable graphs separated from built-in G3 recipes and
    routed to G10?
19. Which effects require export-time high-quality modes distinct from preview
    without changing semantics?
20. What plugin portability contract is declarative, native-per-platform, or
    hybrid?
21. Which qualification stages/profiles block merge, shipping and mobile
    distribution respectively?

## Historical screening-to-plan sequence adopted by PLAN-UCAS-001

This sequence was the research recommendation used to derive the official
execution order. The canonical current order is now
[`PLAN-UCAS-001`](../plans/UNIFIED_CREATIVE_AUTHORING_SYSTEM_PLAN.md):

1. accept definitions, authority boundaries and measured coverage corpus;
2. accept materialized-state versus Receipt truth and the Receipt lifecycle;
3. accept Plan/Receipt/Commit, durable idempotency and atomic multi-operation
   ChangeSet;
4. accept the Federated Catalog, Common DescriptorHeader, exact dependency lock,
   migrations, stable IDs and bounded Recipe IR;
5. accept Curve/Speed/Spatial contracts;
6. generate C++/UI/CLI/MCP/Skill schemas and executable examples from the same
   accepted contracts;
7. convert UI and CLI to the same AuthoringService and remove UI Shape-Fill
   semantics and whole-RFX fallback for admitted operations;
8. add filtered/paginated introspection, structured diagnostics and the local
   MCP adapter;
9. qualify a small baseline catalog:
   - solid/linear/radial/sweep backgrounds;
   - basic pattern;
   - Image asset background when the asset spine exists;
   - Text fade/slide/scale/type-on by safe cluster;
   - Shape rectangle/ellipse/path with solid/gradient/stroke;
   - Blur, Drop Shadow and Glow with full parameters;
10. add deterministic procedural noise/grain;
11. add rich Text Animator/Selector and Shape operators;
12. add shared Layer Styles and material recipes;
13. add backdrop isolation and real Glass;
14. add particle/control-track recipes;
15. route remote/mobile gateway, 3D, public declarative graphs and plugins to
    their accepted stages.

## Primary references

### Motion, presets and text

- [Adobe Effects and Animation Presets](https://helpx.adobe.com/in/after-effects/desktop/apply-effects-and-animation-presets/effects-and-animation-presets/effects-animation-presets-overview.html)
- [Adobe Animation Basics](https://helpx.adobe.com/after-effects/desktop/animate-in-after-effects/animation-basics/animation-basics.html)
- [Adobe Speed Graph](https://helpx.adobe.com/ca/after-effects/using/speed.html)
- [Adobe Text Animation](https://helpx.adobe.com/uk/after-effects/desktop/animating-text/text-animation/animating-text.html)
- [Apple Motion Generators](https://support.apple.com/en-gb/guide/motion/motn8ae88750/mac)
- [Apple Motion Animated Text](https://support.apple.com/en-ie/guide/motion/motn17692a95/mac)
- [Apple Motion Keyframe Editor](https://support.apple.com/en-gb/guide/motion/motn147486cf/mac)
- [W3C Easing Functions Level 2](https://www.w3.org/TR/css-easing-2/)

### Shape, paint, effects and color

- [Adobe Shape Layer Overview](https://helpx.adobe.com/ca/after-effects/using/overview-shape-layers-paths-vector.html)
- [Adobe Shape Paint Operations](https://helpx.adobe.com/ca/after-effects/using/shape-attributes-paint-operations-path.html)
- [Adobe Blending Modes and Layer Styles](https://helpx.adobe.com/after-effects/using/blending-modes-layer-styles.html)
- [Adobe Blur and Sharpen Effects](https://helpx.adobe.com/uk/after-effects/using/blur-sharpen-effects.html)
- [Adobe Noise and Grain Effects](https://helpx.adobe.com/after-effects/desktop/apply-effects-and-animation-presets/list-of-effects/noise-grain-effects.html)
- [W3C CSS Images Level 4](https://www.w3.org/TR/css-images-4/)
- [W3C CSS Color Level 4](https://www.w3.org/TR/css-color-4/)
- [W3C SVG Paint Servers](https://www.w3.org/TR/SVG2/pservers.html)
- [W3C Filter Effects Level 1](https://www.w3.org/TR/filter-effects-1/)
- [Skia Gradient API](https://api.skia.org/classSkGradientShader.html)
- [Skia Image Filters](https://api.skia.org/classSkImageFilters.html)
- [Skia Shading Language](https://docs.skia.org/docs/user/sksl/)

### International text

- [Unicode Bidirectional Algorithm](https://www.unicode.org/reports/tr9/)
- [Unicode Text Segmentation](https://unicode.org/reports/tr29/)
- [Unicode Arabic Mark Rendering](https://www.unicode.org/reports/tr53/)
- [ICU Boundary Analysis](https://unicode-org.github.io/icu/userguide/boundaryanalysis/)
- [HarfBuzz Clusters](https://harfbuzz.github.io/clusters.html)

### Assets, MCP and provenance

- [MCP 2026-07-28 release](https://blog.modelcontextprotocol.io/posts/2026-07-28/)
- [MCP Tools 2026-07-28](https://modelcontextprotocol.io/specification/2026-07-28/server/tools)
- [MCP Authorization 2026-07-28](https://modelcontextprotocol.io/specification/2026-07-28/basic/authorization)
- [C2PA 2.0 Specification](https://spec.c2pa.org/specifications/specifications/2.0/specs/C2PA_Specification.html)

## Final screening conclusion

The central architecture should be a Federated-Registry-driven,
parameterized Recipe system. One AuthoringService plans a server-owned atomic
ChangeSet, returns a bound PlanReceipt and commits it once through CAS and
Runtime preparation.

Materialized project entities are the only visual truth. Preset receipts remain
editable authoring provenance and ownership management; they do not create a
new renderer, a second evaluator, platform-specific animation or hidden
Timeline layers.

Unlimited extensibility is layered: materialized recipes handle most fast
authoring; an accepted declarative graph tier later handles truly live
parameterized graphs; new primitives enter the existing built-in/SkSL/worker/
out-of-process plugin tiers. All tiers retain one semantic path.

The draft's former next activity has been dispositioned: UCAS-WP00 freezes the
coverage corpus, UCAS-WP01 accepts or defers the Recipe/Receipt, Curve, Text
Selector, Asset and MCP contracts, and the remaining UCAS packages stay gated by
their Master Plan stages. Bulk preset coding remains forbidden before the
accepted spine and admission barrier pass.
