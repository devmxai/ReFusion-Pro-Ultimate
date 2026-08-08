---
id: EXP-006
kind: work-package
status: implementation-in-progress-exp006e-passed
gate: pre-G2-experiment
owner_role: live-authoring-architecture
evidence: docs/evidence/experiments/EXP-006.md
review_basis: EV-VA-0001
---

# Semantic Authoring, Measurable Layout, and Topology Guardrails Experiment

## Outcome

Correct the bounded failures exposed by the Reels owner review before the RFX
and hierarchy candidates can inform G2 entry. Prove that a composite visual is
authored as one intentional hierarchy, local FX remain owned by their target,
Text/Shape alignment is measured from the accepted revision, and an external
Agent cannot silently replace an unsupported operation with topology-changing
Layers.

This is a bounded pre-G2 experiment. It does not activate G2, accept RFC-0001
or RFC-0002, adopt RFX as the shipping format, complete G3 typography/FX,
implement Precomposition, or waive Windows evidence.

## Review and plan dependencies

- `docs/evidence/reviews/EV-VA-0001-reels-authoring-review.md`;
- `docs/plans/stages/G2-live-authoring/PLAN.md` and WP02 through WP07;
- proposed `docs/architecture/VISUAL_AUTHORING_MODEL.md` and RFC-0002;
- EXP-002 hierarchy, EXP-003 typed Transform, EXP-004 paint and EXP-005 local
  mask/FX evidence;
- accepted project/revision/clock/GPU authority invariants.

## Already implemented and retained

- real Composition width/height, exact integer frame ranges and stable typed
  Layer/Group IDs;
- one revision authority and Last-Known-Good candidate containment;
- ordered LayerGroup children, explicit roots, parent Transform evaluation,
  collapsed row projection and drill-down;
- RFX4 round-trip with RFX1–RFX3 migration for the bounded
  Shape/Text/paint/mask/local-FX model;
- static Gaussian Blur, Drop Shadow and Glow in a Layer-local ordered stack;
- schema-projected bounded Inspector controls and macOS Metal/Skia execution.

These items are foundations, not completion claims. In particular, authored
Canvas dimensions are real while a general pixel-true measurement/layout
service is not yet implemented.

## Ordered implementation packages

### EXP-006A — Intent and topology contract

- define typed `GroupNodes`, `ReparentNodes`, `AddEffect`, `AlignNodes` and
  bounded `CreateRecipe` intents with base revision and atomic postconditions;
- require `AddEffect` to preserve Layer, Group and root counts;
- distinguish validation from semantic authoring lint;
- reject unsupported animated FX with stable capability diagnostics;
- add Background, local Glow and grouped Subscribe recipes whose examples are
  compiled in CI.

### EXP-006B — TextBox and coordinate schema

- add a centered local Text layout box with width, height and explicit padding;
- add direction, horizontal/vertical alignment, wrap/overflow, line-height and
  letter-spacing domains only to the bounded extent needed by fixtures;
- distinguish `position parent_px` from `anchor local_px` in contracts and
  migration while preserving stable IDs;
- represent packaged Font Asset identity and fail closed on missing/mismatched
  fonts; system-family names remain non-qualified convenience inputs;
- generate Registry/RFX/Inspector/Agent projections from one descriptor digest.

### EXP-006C — Shared layout and measurement

- introduce a backend-neutral `TextLayoutPort` and immutable paragraph result;
- provide layout box, logical bounds, ink bounds, clipped bounds, baselines,
  ascent/descent/leading, resolved Font digest and overflow state;
- expose geometry, mask, effect-expanded and world bounds at one exact project
  time through the common Evaluation/Render Plan;
- use one layout result for Canvas preview, offline probe and CLI measurement;
- keep derived metrics out of serialized project truth and cache them by full
  text/style/box/font/layout-engine digest.

### EXP-006D — Measured authoring commands

- implement one-shot atomic `AlignNodesCommand` with explicit
  logical/ink/geometry basis and horizontal/vertical relation;
- evaluate subject and target from the same accepted revision and exact frame;
- add optional later constraints only after dependency-cycle semantics are
  decided; this experiment introduces no hidden persistent layout dependency;
- keep Group/FX/Align edits on the same command, validation, CAS, persistence,
  journal and diagnostic path as UI edits.

### EXP-006E — Timeline and Inspector projection

- project only root VisualLayer/LayerGroup nodes as top-level visual rows;
- keep FX, masks and animation properties nested under their semantic owner;
- preserve Group collapse, drill-down, breadcrumb and stable selection;
- expose TextBox/alignment and measured read-only bounds from accepted snapshots;
- add no UI clock, project authority, layout cache or renderer state.

### EXP-006F — Agent digital eye and guardrails

- deliver machine-readable `outline`, `inspect`, `measure`, `capabilities`,
  `validate`, semantic `lint`, `diff` and typed `commit` operations;
- report parent path, resulting Timeline row, FX/mask/property ownership, exact
  ranges, local/logical/ink/effect/world bounds and Font resolution digest;
- generate the project-local Skill and recipes from the same Registry digest;
- forbid anchor compensation for glyph metrics and duplicate-Layer FX
  approximation in Agent instructions;
- make every shipped example compile and pass parity tests in CI.

### EXP-006G — Regression and owner evaluation

- add a repository-owned sanitized Reels fixture derived from the semantic
  findings, not the owner's external workspace contents;
- prove Background Group, one Title Layer with local FX and one grouped
  Subscribe component through save/reopen and deterministic seek;
- compare equivalent UI and Agent intents, semantic diffs and accepted digests;
- run the complete Core, sanitized, Studio and macOS Visual lanes;
- keep Windows runtime `not-run` until a physical device exists and make no
  cross-platform qualification claim from macOS alone.

## Implementation result — EXP-006A

The first package is implemented and passes its declared macOS/shared-contract
lanes:

- Core/Application expose atomic `GroupNodesCommand`,
  `ReparentNodesCommand` and topology-preserving `AddEffectCommand` through the
  same `ProjectCommandService` and revision authority;
- grouping preserves existing sibling stacking order, derives a containing
  Group range and rejects missing, multiply sourced or cyclic topology without
  changing Last-Known-Good;
- `AddEffectCommand` inserts one effect in the owner Layer's ordered stack and
  verifies that Layer IDs/order, Group IDs/children and root order are unchanged;
- typed `AlignNodesCommand` and `AnimateEffectPropertyCommand` were introduced
  fail-closed; alignment is admitted by EXP-006D while effect-property
  animation remains unavailable with `RFX-CAP-FX-ANIMATION-001`;
- a Core capability snapshot exposes supported grouping/reparent/effect-add;
  EXP-006D advances alignment to supported while effect animation retains its
  stable unavailable code;
- advisory semantic lint is separate from validation and detects ungrouped
  full-duration Background components plus suspicious duplicate Text/Glow
  Layers without rejecting intentional authored duplication;
- newly created workspaces receive EXP-006A semantic-authoring rules and
  Background/local-Glow recipes; the rules explicitly prohibit glyph-anchor
  guessing and duplicate-Layer FX approximations;
- tests prove idempotency, exact revision increments, topology preservation,
  sibling order, cycle rejection, RFX3 round-trip, LKG retention and Application
  Host routing.

## Implementation result — EXP-006B

The second package is implemented and passes its declared macOS/shared-contract
lanes:

- portable Core now owns a centered local `TextBox` with explicit width,
  height and four-edge padding plus bounded direction, paragraph horizontal/
  vertical alignment, wrap, overflow, line-height and letter-spacing domains;
- `FontIdentity` distinguishes non-qualified system-family convenience from a
  packaged asset identity requiring stable asset ID, family and lowercase
  sha256 digest; malformed or incomplete packaged identity fails closed;
- canonical experimental writing emits RFX4, records the exact Core visual
  property Registry digest and spells Transform position as `parent_px` and
  anchor as `local_px`; strict RFX1–RFX3 inputs migrate without changing stable
  project, composition, Layer, Group, mask or FX IDs;
- the RFX4 parser rejects a mismatched Registry digest before project parsing,
  preventing Agent/source vocabulary drift from the running engine;
- Inspector property records, canonical RFX4, `refusion.lock` and a generated
  project-local Agent property catalog all expose the same Registry digest;
- Font source/asset/digest projections are read-only until a future atomic Font
  asset command exists; paragraph and TextBox authored properties use normal
  typed property validation;
- CLI descriptions now report `position_parent_px`, `anchor_local_px`, TextBox
  geometry and Font identity rather than the ambiguous legacy Canvas labels;
- tests prove centered box/content geometry, RTL schema, padding rejection,
  qualified Font round-trip, malformed Font rejection, RFX migration,
  Registry-mismatch rejection and projection-digest equality.

## Implementation result — EXP-006C

The third package is implemented and passes its declared macOS/shared-contract
lanes:

- portable Core owns a backend-neutral `TextLayoutPort`, immutable paragraph
  result and deterministic cache key over the complete Text, Font, TextBox,
  paragraph-style and concrete layout-engine descriptor;
- the result contains layout/content boxes, logical/ink/clipped bounds, exact
  UTF-8 line ranges, origins, baselines, ascent/descent/leading, overflow state,
  resolved Font identity and layout-engine digest; none is serialized as
  authored project truth;
- Core evaluation attaches the same paragraph result plus geometry, mask,
  ordered-effect-expanded and transformed world bounds at one exact project
  time to the immutable evaluated Layer;
- the macOS Skia adapter implements the port with the admitted HarfBuzz/ICU
  shaping stack, exact installed-family matching and cached immutable TextBlobs;
  preview is forbidden from reshaping independently and rejects a missing
  accepted cache entry;
- `refusion-cli measure <Project.rfx> <project-time-ns>` uses the same Skia
  port and reports those bounds, line metrics, Font/layout digests and cache
  key for offline inspection;
- missing system Fonts reject with `RFX-FONT-SYSTEM-MISSING-001`; qualified
  packaged Fonts reject with `RFX-FONT-ASSET-RESOLUTION-001` until an admitted
  project Asset-byte resolver exists, with no silent family fallback;
- automated fixtures cover Latin, Arabic/RTL, mixed direction with diacritics,
  multiline/wrap/clip/overflow, negative Font resolution, cache stability,
  local effect/world bounds, CLI measurement and a real Metal/Skia frame using
  the shared layout path.

### Implementation result — EXP-006D

The fourth package is implemented and passes its declared portable command
contract:

- `AlignNodesCommand` is a one-shot atomic Core intent with explicit
  geometry/logical/ink basis, horizontal relation, vertical relation and exact
  Composition time; it introduces no serialized constraint or second layout
  authority;
- measurement consumes one immutable `EvaluatedVisualScene` whose hierarchy
  transforms and evaluated Layers come from one traversal of the accepted
  revision at the requested time;
- geometry uses authored Shape/TextBox bounds, while logical and ink bases are
  admitted only when the engine-owned `TextLayoutPort` supplied those metrics;
- world-space alignment is converted through the inverse parent transform, so
  nested scale/rotation remains correct; Group bounds are the aggregate of
  active descendants for the selected basis;
- a static Position is translated directly; an animated Position is translated
  by applying the same offset to every keyframe, preserving curve shape and
  producing a one-shot authoring edit rather than a hidden dependency;
- the candidate is validated, remeasured and rejected unless the requested
  anchors differ by at most `0.25` Composition pixel. Rejection, stale CAS,
  missing measurement, non-invertible parent, invalid basis/time and
  ancestor/descendant ambiguity all retain Last-Known-Good;
- Application Host routes the same typed command, Studio injects the admitted
  Skia measurement port, and canonical RFX save/reopen preserves the resulting
  authored Transform without persisting derived bounds.

### Implementation result — EXP-006E

The fifth package is implemented on the native Studio projection boundary:

- the Timeline continues to show only root Visual Layers/Groups as top-level
  rows and preserves Group drill-down/breadcrumb behavior;
- each visible Layer now projects Mask, owner-local FX and Transform-animation
  lanes as indented semantic property rows carrying stable owner ID, row kind
  and depth. Clicking a property row selects its owner; no property becomes a
  Visual Layer, Group or NLE Track;
- Inspector exposes accepted-snapshot geometry/logical/ink world bounds as
  read-only Composition-pixel values at the Runtime-owned current project time;
- alignment target projection excludes the subject and ancestor/descendant
  relationships through a shared Core hierarchy query, then Studio submits the
  typed `AlignNodesCommand`. QML contains display formatting and choices only;
  it computes no alignment offset and owns no timer, layout cache or authority;
- the visual application shares the admitted Skia `TextLayoutPort` with the
  Application command/Inspector measurement context. Runtime supplies the
  exact half-open Composition time and drives measurement refresh notifications;
- generic Registry-backed TextBox and paragraph controls remain the authored
  property path; unavailable logical/ink measurement fails closed while every
  rejected command keeps all panels on the accepted revision.

### Implementation result — EXP-006F

- portable `AgentIntrospection` derives stable node refs, parent paths, sibling
  indices, resulting Timeline rows, exact ns/frame ranges, Transform values,
  Mask/FX/animation ownership and Registry properties from one validated
  `ProjectSnapshot`;
- stable canonical snapshot digest and semantic diff distinguish metadata,
  topology, added/removed nodes and changed nodes without treating derived
  measurements as project truth;
- CLI JSON schemas now expose outline, inspect, Skia-backed measure,
  capabilities, validation, semantic lint and diff with structured failures;
- measured output joins the semantic address to local geometry/mask/effect,
  world geometry/logical/ink/effect bounds and resolved Font/layout digests at
  one exact Runtime-domain Composition time;
- typed Agent GroupNodes, AddEffect(Glow) and AlignNodes operations instantiate
  the normal Application command types. Rejected commands write no bytes;
  accepted candidates advance one revision and use a platform-selected atomic
  file adapter before live Studio revalidation;
- new project Skills receive an Application-generated command catalog bound to
  the same Registry digest/capability descriptors as RFX and Inspector, with
  explicit prohibitions on glyph-anchor guesses and duplicate-Layer FX;
- the shipped example is compiled by CI digital-eye/commit tests, including a
  deliberate duplicate-FX rejection/LKG check and Skia logical alignment with
  non-empty Font/layout digests.

### Implementation result — EXP-006G

- added a repository-owned synthetic 1080x1920, 60 fps, 30-second RFX4 fixture
  with exactly three semantic roots: one ordered Background Group, one Title
  Layer with local Shadow/Glow and one grouped Subscribe component;
- portable regression proves schema validity, stable roots/children/ownership,
  zero semantic-lint findings, canonical save/reopen equality and identical
  digest after arbitrary non-monotonic exact-frame seeks;
- Skia regression proves non-empty layout/Font digests, deterministic repeated
  seeks, Title and Subscribe-label measured centering within `0.25` Composition
  pixel, owner-local FX bounds expansion and unchanged paragraph metrics;
- Studio/Application parity regression applies equivalent UI and Agent Glow and
  Align intents from the same base. Both variants produce byte-equivalent
  snapshots, equal semantic digests, the same changed-node diff and no topology
  change;
- complete Core, sanitized, Studio and physical macOS Visual lanes pass. The
  owner's external Reels workspace was not read or modified.

Automated EXP-006G implementation is complete. On 2026-08-08, the product owner
opened the sanitized fixture in the physical macOS Studio and visually accepted
it. EXP-006 is therefore closed as bounded pre-G2 evidence. Formal
MCP/ChangeSet parity remains in proposed G2-WP06; this evidence does not
activate it.

## Deferred to formal G3 planning

- general registry-addressed `Property<T>` animation for arbitrary FX fields;
- animated Glow color/intensity/sigma, broad easing/spring/curve editing and
  reusable motion systems beyond the bounded Transform preset;
- full typography, optical layout breadth, text-on-path and advanced scripts;
- Group masks/FX/opacity isolation, Adjustment Layers, Backdrop Glass and Motion
  Blur;
- CompositionInstance/Precomposition and independent time mapping.

Until the corresponding G3 capability exists, requests for these operations
must fail closed rather than create hidden approximations.

## Acceptance criteria

- `AddEffect(title, glow)` changes no Layer, Group or root count;
- a composite Background creates one collapsed root Group with ordered children;
- unsupported animated Glow returns the same typed capability code through UI,
  CLI/MCP and diagnostics while Last-Known-Good remains active;
- Text/Shape center alignment differs by at most `0.25` Composition pixel before
  rasterization for both declared logical and ink bases;
- the alignment remains correct under parent scale/rotation and at multiple
  exact frames;
- Arabic/RTL, mixed direction, diacritics, multiline and missing-Font negative
  fixtures have explicit bounded outcomes;
- Shadow/Glow expand effect bounds without changing paragraph metrics;
- RFX migration/round-trip preserves IDs, hierarchy, order, TextBox,
  alignment and FX ownership;
- UI and Agent variants produce equal normalized intent and semantic digest;
- preview and offline probe consume the same layout/evaluation digest;
- all forbidden Qt/backend types and CPU media paths remain absent from Core.

## Verification

```bash
python3 tools/rfdev.py docs-doctor
python3 tools/rfdev.py architecture-check
cmake --workflow --preset macos-core
cmake --workflow --preset macos-core-sanitized
cmake --workflow --preset macos-studio
cmake --workflow --preset macos-visual
```

Matching portable Core/schema lanes remain mandatory from the first revision.
Windows physical runtime evidence is required by the formal G2 exit, not
fabricated by this experiment.

## Kill criteria

Reject the experiment if alignment is implemented as QML/Agent offsets, derived
metrics become editable project truth, an FX intent can create a Layer, a
missing Font silently falls back in a qualified fixture, layout semantics live
only in a Metal/Skia adapter, UI/file/CLI use different commands, or the change
introduces a second evaluator, clock, render graph or revision authority.

## Exact handoff

Completed on 2026-08-08: the product owner opened the sanitized regression
project in the physical macOS Studio and accepted one root Background Group,
one Title Layer with owner-local Shadow/Glow, one Subscribe Group and measured
Text centering. Automated evidence independently proves topology
postconditions, bounds tolerance, save/reopen, parity, rejection and
Last-Known-Good behavior. EXP-006 may inform G2 planning without activating G2.
