---
id: RESEARCH-VA-001
kind: research-draft
status: draft
canonical_for: none
owner_role: visual-authoring-architecture
last_verified: 2026-08-07
---

# Visual Authoring, Hierarchy, and Extensibility Screening Draft

> **Non-authoritative research draft.** This document records the screening
> that informed RFC-0002 and the proposed G2 plan. It is not an implementation
> specification, an accepted architecture, a release promise, or a second
> Master Plan.

## Review question

How can ReFusion grow from the current live-authored Shape/Text experiment into
a professional, cross-platform visual authoring system with coherent groups,
nested compositions, typed animation, modern materials and effects, without
creating a second project truth, a second evaluator, platform-specific project
semantics, or one Timeline row per drawing primitive?

## Evidence reviewed

The review covered:

- the current `Project.rfx` experiment and Revision 7 authoring result;
- Core project/revision/time contracts and Studio Timeline projection;
- current macOS Metal/Skia rendering and the unqualified Windows lane;
- ReFusion architecture invariants, Product Contract and MP-001;
- official Adobe documentation for After Effects precomposition and shape
  groups, and Premiere nested sequences and adjustment layers;
- the Blackmagic Fusion manual's distinction between nodes, groups, macros and
  reusable templates;
- official Skia paint, image-filter and SkSL documentation.

Primary product references:

- [After Effects precomposing and nesting](https://helpx.adobe.com/after-effects/desktop/work-with-compositions/precomposing-and-nesting/precomposing-nesting-pre-rendering.html)
- [After Effects shape layers, paths and vector groups](https://helpx.adobe.com/uk/after-effects/using/overview-shape-layers-paths-vector.html)
- [Premiere nested sequences](https://helpx.adobe.com/premiere/desktop/edit-projects/edit-nested-sequences/about-nested-sequences.html)
- [Premiere adjustment layers](https://helpx.adobe.com/ca/premiere/desktop/add-video-effects/apply-video-effects/create-adjustment-layers.html)
- [Blackmagic Fusion manual](https://documents.blackmagicdesign.com/UserManuals/FusionManual.pdf)
- [Skia paint overview](https://skia.org/docs/user/api/skpaint_overview/)
- [Skia image filters](https://api.skia.org/SkImageFilters_8h_source.html)
- [Skia Shading Language](https://docs.skia.org/docs/user/sksl/)

These products are references for proven interaction patterns, not a parity
claim or permission to copy their internal implementations.

## Finding 1 — the current model is too flat

The experiment proved useful facts: a typed external edit can compile into Core,
be accepted atomically, retain Last-Known-Good on failure, and appear live in
Canvas and Timeline. It did not prove a scalable authoring model.

Revision 7 represented a relatively small Subscribe composition with fourteen
top-level layers, thirty-one animation blocks and 193 keyframes. Visual parts of
one button became independent Timeline rows and repeated pulse keyframes. This
is not primarily a renderer-quality problem. The project model currently makes
the Agent express internal drawing structure as top-level layers.

The correction is to separate four semantic domains:

```text
Asset / Source

Sequence -> Tracks -> Clips                 NLE domain

Composition -> Visual Layer Tree            DCC/motion domain
                 |-> Content Nodes
                 |-> Layer Groups
                 `-> Composition Instances
```

`Layer`, `Track`, `Clip`, `ContentNode`, and `TimelineRow` must not be aliases.

## Finding 2 — professional hierarchy has distinct meanings

| Concept | Persistent meaning | Timeline meaning | Own clock? |
|---|---|---|---:|
| Asset/Source | Immutable media or generated source identity | None by itself | No |
| Track | Ordered NLE lane for clips | One lane | No |
| Clip | Timed instance of a source in a sequence | One item in a track | No |
| Visual Layer | Compositable item in a composition | One row when exposed | No |
| Content Group | Internal paths/text/shapes inside one layer | Not a track | No |
| Layer Group | Parent for child visual layers | One collapsed parent row | No |
| Composition | Independent visual document with canvas and duration | Root or nested source | Uses ProjectClock |
| Composition Instance | Timed/reusable precomposition reference | One parent row/clip | No second clock |
| Folder | Organizational metadata only | Optional tree organization | No |

The word “Scene” may be a product-facing role or preset for a Composition; it
must not introduce another engine object with different timing semantics.

## Finding 3 — groups and precompositions solve different problems

A `LayerGroup` coordinates children within the same Composition. It carries a
parent transform, visibility and optional compositing properties. Collapsing it
changes only the Timeline projection; it does not destroy or merge children.

A `CompositionInstance` references another Composition. The child owns its own
canvas, duration and layer tree, while the instance owns an exact parent-to-child
time mapping. It appears as one parent item and opens by drill-down/breadcrumb.
It may not create an independent runtime clock or evaluator.

The bounded first hierarchy proof should use a LayerGroup. Full reusable
precomposition behavior follows after exact mapping and cycle rejection are
proven. Parameterized component definitions and a marketplace are post-v1
expansion candidates, not prerequisites for the first creator loop.

## Finding 4 — one typed property system must drive every surface

Every authorable value should be identified by a registry descriptor containing:

- stable capability/property ID and schema version;
- value type, unit, domain, default and validation constraints;
- compatible owner types and typed input/output ports;
- animatability, interpolation and extrapolation policy;
- Inspector metadata and diagnostic names;
- evaluation implementation and platform qualification state;
- serialization/migration, CLI/MCP and Agent Skill projections.

The initial animation contract should be deterministic and seekable:

- constant/step;
- linear;
- cubic Bézier with explicit temporal control points;
- bounded repeat and ping-pong behavior only after exact tests.

Expressions, arbitrary project shaders and a public node graph are not required
for v1. A feature that exists only in QML, only in the parser, or only in a Metal
file is not a ReFusion capability.

## Finding 5 — render order is a semantic decision

The following bounded order is the current candidate, not an accepted decision:

```text
Source / Paint
-> Crop and Layer Masks
-> Ordered Local FX
-> Anchor and Transform
-> Layer Opacity and Blend
-> Parent Composition
```

Candidate behavior:

- opacity is compositing, not geometry;
- a group is pass-through when it has no group compositing behavior;
- group opacity, mask or FX requires an isolated offscreen result;
- a precomposition is explicitly flattened at its instance boundary in v1;
- backdrop effects read only the already-accumulated lower layers of their
  parent, preventing dependency cycles;
- unsupported ports or ordering fail with typed diagnostics and never silently
  approximate the visual result.

This order requires golden tests before an ADR can accept it. Mask/shadow and
group isolation details are especially observable and cannot be left to backend
accident.

## Finding 6 — materials are recipes over primitives

The portable base should remain small:

- solid paint;
- linear, radial and sweep gradients;
- fill and stroke descriptors;
- bounded image texture input;
- a deliberately small procedural generator contract.

Video and Image are sources, not universal “materials.” A Background command is
a preset that creates the appropriate source/paint; it is not a special renderer
or platform layer. Paper, texture, frosted glass and similar looks should be
versioned registry recipes built from the portable primitives.

This permits vivid modern design without adding a new project node type for
every style name.

## Finding 7 — Glass and Motion Blur need prerequisites

Professional glass is not a translucent rectangle. Its candidate graph is:

```text
Accumulated lower composite
-> bounded backdrop capture
-> blur/color/tint
-> glass mask
-> highlight/stroke/shadow
-> parent composite
```

It requires explicit offscreen ownership, region-of-interest propagation,
linear compositing, bounds, transient GPU budgets and cache invalidation. It
must never read the platform drawable as an implicit project input.

Professional motion blur is not a Gaussian blur. Transform motion blur requires
exact subframe shutter sampling of the same evaluator. It should begin as a
bounded export-quality feature for Shape/Text/Group after the exact time domain
is proven. Video/deformation/optical-flow motion blur follows hardware media
scheduling and is outside the first visual-authoring gate.

## Finding 8 — typography and color are architecture, not polish

Professional cross-platform text requires project/bundled font assets, stable
font identity and provenance, deterministic resolver/fallback, cached shaping,
paragraph layout, alignment, tracking, leading, and Arabic/RTL fixtures. A
system font name alone cannot provide cross-platform semantic parity.

Desktop v1 remains SDR Rec.709. A linear F16 working surface is a strong
candidate for effects and compositing, but it must be measured on qualified
macOS and Windows devices before acceptance. The contract is exact semantic
agreement plus calibrated visual tolerances, not bit-identical floating-point
pixels across unrelated GPUs.

## Finding 9 — hierarchy must enter G2 before visual breadth

If G2 ships a flat `layers[]` authoring contract and hierarchy is postponed to
G3, every parser, ChangeSet, journal, Undo record, Timeline model, Inspector,
Agent API and migration will later require a structural rewrite.

Therefore G2 should include one bounded hierarchy slice:

- a stable LayerGroup with parent/child/order identity;
- one collapsed Timeline row with child drill-down;
- one parent animation that coherently moves the subtree;
- exact local/world measurement and save/reopen preservation;
- the same semantic digest from UI and Agent changes.

G3 then expands visual breadth on that accepted spine: paints, masks, typography,
blends, a small FX registry, Adjustment and richer animation tools.

## Candidate staged delivery

1. **G2 — Live Authoring spine:** decide format, type registry, ChangeSets,
   hierarchy seed, stamped evaluation, Timeline tree, Agent parity and reliable
   round-trip.
2. **G3 — Unified visual authoring:** complete Image/Text/Shape/Group/Adjustment,
   deterministic typography, paints, masks, blends and bounded FX.
3. **G4 — Hardware media and transport:** Asset/Source/Sequence/Track/Clip,
   linked independent video/audio, exact timing, waveform and hardware export.
4. **G5 — Creator loop:** installer-to-export reference projects, recovery,
   Undo/Redo, relink and production diagnostics on macOS and Windows.
5. **Post-loop updates:** reusable precompositions, advanced curve tools,
   glass recipes and transform motion blur after their prerequisites pass.

This is capability allocation, not a replacement delivery order; MP-001 remains
the sole delivery authority.

## Reference vertical slices

- **VS-01 Subscribe Group:** one collapsed Timeline row; body, label and bell
  remain addressable; one parent curve moves the whole group; a child curve may
  animate the bell locally.
- **VS-02 Material Background:** one Background layer can use solid or animated
  gradient without fake wash tracks.
- **VS-03 Precomp drill-down:** one parent row, three child rows, exact cross-rate
  mapping and cycle rejection.
- **VS-04 Mask/FX chain:** typed port compatibility and explicit render order.
- **VS-05 UI/Agent parity:** both paths normalize to the same ChangeSet and
  semantic digest.
- **VS-06 LKG fault corpus:** malformed, partial, stale and cyclic edits never
  replace the active revision.
- **VS-07 Preview/export equivalence:** same semantic evaluator and explicit
  quality/scheduling differences.
- **VS-08 Registry growth:** an internal descriptor can be added without parser
  or QML command-family switches.
- **VS-09 G4 media:** one imported asset produces linked, independently editable
  Video and Audio clips without CPU video pixel transfer.

## Candidate budgets and evidence

These are review targets, not passed measurements:

- 10,000 mixed UI/file candidates: zero mixed revisions and zero LKG corruption;
- canonical serialize/parse/serialize is byte-stable for the same schema;
- migration is idempotent and preserves stable IDs;
- 23.976/29.97/59.94 and subframe mapping show no cumulative drift;
- 500 layers and 5,000 keys parse/validate at p95 <= 100 ms on the declared
  reference Apple M1 tier;
- accepted revision becomes visible within one presented frame;
- ordinary authoring causes no UI stall over 16 ms on the reference profile;
- baseline 1080x1920@60 evaluation is p95 <= 2 ms CPU and p95 <= 12 ms GPU,
  subject to calibration on qualified macOS and Windows tiers;
- no unbounded memory growth across 10,000 frames or device-loss recovery;
- each effect reports pass count, transient bytes, cache result, GPU duration and
  affected bounds;
- every Agent Skill example compiles against the declared registry digest.

## Kill criteria

Stop, reject or redesign a candidate if it:

- makes UI or a project file watcher an authority;
- introduces another clock, project truth, semantic evaluator or export graph;
- aliases Layer, Track, Clip and Timeline row;
- loses IDs or semantic digest during group/precomp save/reopen;
- permits hierarchy/dependency cycles to reach rendering;
- requires C++ compilation for an ordinary property edit;
- places Qt, Skia, codec or native handles in portable project state;
- performs CPU video pixel decode, conversion, upload or readback;
- silently approximates an unsupported effect or platform capability;
- reads backdrop from a platform drawable rather than an explicit graph input;
- leaves caches valid across incompatible revision/device generations;
- changes project meaning between macOS and Windows.

## Screening conclusion

The professional path is not to copy an editor's UI breadth. It is to accept a
small semantic spine whose hierarchy, time, property registry, compositing order,
revision behavior and platform boundaries remain valid as capabilities expand.

The next decision artifact is RFC-0002. Until it is accepted, the hierarchy and
render-order model in this draft remains a reviewed proposal only.
