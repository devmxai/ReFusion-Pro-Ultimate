---
id: RFC-0002
kind: rfc
status: proposed
title: Unified visual authoring hierarchy and compositing semantics
owner_role: product-owner
decision_due: G2-entry
---

# Problem

The live `Project.rfx` experiment can render and atomically reload a flat list of
Shape/Text layers, but a professional visual composition cannot make every
drawing primitive a top-level layer, every layer a Timeline track, or every new
look a new parser/UI/backend special case.

ReFusion needs a durable meaning for Asset/Source, NLE Track/Clip, Visual Layer,
Content Group, Layer Group and nested Composition before it expands materials,
masks, effects, glass, motion blur or media editing.

# Required decision

Accept, revise or reject the following architecture for G2 and later gates:

1. distinguish `Track`, `Clip`, `VisualLayer`, `ContentNode`, `LayerGroup`,
   `CompositionInstance` and `TimelineRow` as separate typed concepts;
2. add a bounded LayerGroup hierarchy proof to G2;
3. drive project authoring, Inspector, CLI/MCP and Agent guidance from one typed
   capability/property registry;
4. recursively evaluate hierarchy into one Evaluation Graph and one accepted
   `EvaluationStamp` without adding a clock or revision authority;
5. adopt an explicit compositing-order contract and typed dependency ports;
6. allocate broad visual authoring to G3, hardware Track/Clip media semantics to
   G4, and advanced glass/motion-blur work only after prerequisites pass.

This RFC does not decide RFC-0001's shipping project syntax. The selected format
must be able to represent the accepted semantic model without becoming a second
truth.

# Options and trade-offs

## A — Keep the flat Layer/Timeline model through G2

Smallest immediate change, but it bakes structural debt into serialization,
ChangeSets, journal, Undo, Timeline, Inspector and Agent tools. Later Group and
Precomp work would require destructive migrations. Not recommended.

## B — Introduce a general node DAG now

Maximum theoretical freedom, but it expands cycle semantics, UI, scheduling,
caching, ports and diagnostics before the creator loop exists. It also risks a
Fusion-like node editor becoming a second authoring model. Not recommended for
v1.

## C — Typed hierarchical composition with bounded ports

Separate NLE and DCC concepts, add a tree-shaped visual hierarchy with explicit
Composition references and bounded dependency ports, and compile it into one
Evaluation Graph. This provides professional grouping and future extensibility
without exposing a general graph UI. Recommended.

## D — Compile project-specific C++ for hierarchy and effects

Allows unrestricted code, but ordinary edits become executable and compile-
bound, UI/source round-trip is unsafe, mobile distribution is incompatible, and
validation/LKG containment weaken. Native extension work remains isolated and
post-v1; it is not a project document model. Not recommended.

# Proposed semantic contract

```text
Project
  Assets/Sources
  Compositions
    VisualLayerTree
      VisualLayer
        ContentNode tree
      LayerGroup
      CompositionInstance
  Sequences                         G4 activation
    Tracks
      Clips -> Asset/Composition
```

- A ContentGroup never creates a Track.
- A collapsed LayerGroup projects as one Timeline row while preserving child
  identities and authorability.
- A CompositionInstance is a reusable/nested reference with exact parent-child
  time mapping and cycle rejection.
- Folder is organizational metadata only.
- “Scene” is a product label/preset for Composition, not another engine type.
- Imported Video and Audio become linked but independent clips in G4.

## Proposed transform and time behavior

Local transform is evaluated from explicit Composition-pixel units:

```text
T(position) * R(rotation) * Skew * S(scale) * T(-anchor)
```

Parent transforms multiply deterministically. Opacity is compositing state, not
geometry. Nested time maps use Core exact domains and the one `ProjectClock`;
there is no group/precomposition clock.

ADR-0009 remains the accepted clock-authority decision. The exact rational/
subframe representation required by non-integer rates and motion blur needs a
separate measured decision; the current nanosecond experiment must not be
silently promoted if it accumulates rounding error.

## Proposed compositing order

Pending golden tests:

```text
Source/Paint
-> Crop and Layer Masks
-> Ordered Local FX
-> Anchor and Transform
-> Layer Opacity and Blend
-> Parent Composition
```

A group is pass-through when it has no group compositing operation and isolated
when group opacity, mask or FX requires an offscreen result. A bounded v1
CompositionInstance is flattened explicitly. Backdrop inputs can read only the
accumulated lower result of the same parent.

# Experiments and evidence

Before acceptance:

1. classify the current `Project.rfx` Revision 7 into the proposed types and
   prove that its Subscribe button becomes one collapsed LayerGroup row;
2. prove one parent animation plus one child animation with correct pivot,
   local/world bounds and deterministic seeking;
3. serialize, reopen and migrate without changing IDs or semantic digest;
4. reject parent, precomposition and typed-port cycles before activation;
5. prove equivalent UI and Agent edits normalize to the same TypedChangeSet and
   accepted semantic digest;
6. evaluate the subtree through the existing revision/clock/presentation
   authorities and demonstrate that no second evaluator or graph was added;
7. measure parsing, evaluation, frame latency and memory against the declared G2
   reference tier;
8. run semantic fixtures on macOS and Windows before a cross-platform exit
   claim; visual comparison uses calibrated tolerances, not screenshots alone.
9. disposition the owner findings in
   [`EV-VA-0001`](../../evidence/reviews/EV-VA-0001-reels-authoring-review.md):
   composite Background topology, effect ownership, unsupported animated-FX
   behavior, measurable TextBox alignment and Agent intent guardrails.

Bounded implementation evidence is recorded in
[`EXP-002`](../../evidence/experiments/EXP-002.md). It proves the macOS/shared-
contract slice only and does not satisfy the required Windows or G2-entry
decision evidence by itself.

The revision-required findings are assigned to the bounded
[`EXP-006`](../../plans/experiments/EXP-006-semantic-authoring-measurement.md)
before this RFC can be dispositioned. EXP-006 may inform the decision but cannot
accept this RFC, activate G2 or pull general animated FX/typography into G2.

# Security, licensing and platform impact

The semantic model remains portable C++ data and contains no Qt, Skia, codec,
native GPU or operating-system objects. It adds no dependency or license by
itself. Built-in implementations may use platform adapters, but all platforms
consume the same descriptors and project meaning.

Arbitrary project C++ and SkSL remain excluded. Future native extensions stay
out of process under MP-001. Mobile may consume packaged/declarative descriptors
but may not download executable native plugins.

# Recommendation

Choose Option C and use the G2 reference slice to prove it before broad visual
features. Keep materials as registry recipes over a small paint/effect basis;
defer Glass until explicit backdrop/offscreen/color contracts exist and defer
Motion Blur until exact subframe evaluation exists.

# Final disposition

Proposed after the 2026-08-07 architecture and adversarial plan review, then
amended with the 2026-08-08 owner-authoring findings in EV-VA-0001. No
architecture is accepted and no G2 work package is activated by this RFC. The
product owner must accept, revise or reject it at G2 entry; an accepted outcome
must then be recorded in an ADR rather than rewriting this proposal as history.
