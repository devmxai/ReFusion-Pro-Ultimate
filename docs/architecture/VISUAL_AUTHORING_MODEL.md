---
id: ARCH-VA-001
kind: architecture-candidate
status: proposed
owner_role: visual-authoring-architecture
decision: RFC-0002
master_plan: MP-001
last_verified: 2026-08-07
---

# Unified Visual Authoring Model — Proposed Contract

> This is the implementation-facing candidate attached to RFC-0002. It is not
> an accepted architecture until the RFC is decided and an ADR records that
> decision. MP-001 remains the only delivery-order authority.

## Objective

Provide one portable semantic model that can grow from a grouped Shape/Text
composition to Image, SVG, masks, effects, nested compositions and later NLE
video/audio editing without changing project truth or platform meaning.

## Authority flow

```text
UI Intent ─────────────┐
                      ├─> TypedChangeSet -> Validate/CAS -> RevisionAuthority
File/CLI/MCP Intent ──┘                                  |
                                                            v
                                                  Accepted Revision
                                                            |
                                          Evaluate(ProjectClock, epoch)
                                                            |
                                                 EvaluationStamp
                                     ┌──────────────┼──────────────┐
                                     v              v              v
                                  Timeline       Inspector       Canvas
                                                            |
                                                 Unified Render Plan
```

Files and filesystem notifications are inputs, not authorities. Timeline rows
and Inspector controls are projections, not project objects. A render failure
cannot partially publish a new revision; Last-Known-Good remains visible with a
structured diagnostic.

## Semantic domains

### Asset and Source

- `AssetId` identifies imported immutable media plus provenance and media facts.
- `SourceId` identifies an evaluatable image, video, audio, vector, text,
  procedural or composition source.
- Derived thumbnails, waveforms, proxy frames and GPU caches are artifacts and
  never project truth.

### Composition and visual hierarchy

- `CompositionId` owns canvas dimensions, duration/rate domain and an ordered
  root visual-layer list.
- `VisualLayerId` owns timing within the composition, compositing properties,
  transform, optional masks/FX and exactly one content source or child role.
- `ContentNodeId` addresses internal vector/text/shape structure without
  projecting each primitive to a Timeline track.
- `LayerGroupId` is a visual-layer parent with ordered children and optional
  group-level compositing.
- `CompositionInstanceId` references a child Composition with explicit time map.
- `FolderId` is organization-only and cannot affect evaluation.

### Sequence editing, admitted in G4

- `SequenceId` owns ordered NLE tracks.
- `TrackId` identifies a video, audio, graphic or bounded adjustment lane.
- `ClipId` is a timed instance referencing an Asset/Source/Composition with
  source range, trim, link identity and exact time mapping.
- Video and Audio clips may share an import/link group but remain independently
  selectable, editable and diagnosable.

These types may share utilities but may not be typedef aliases. In particular,
`TimelineRow` is a view record and never a stable substitute for a semantic ID.

## Hierarchy constraints

1. A visual node has at most one visual parent within a Composition.
2. Ordered sibling position is explicit and stable across save/reopen.
3. Parent and Composition reference graphs are acyclic before activation.
4. Reparent/reorder is one atomic ChangeSet with compare-and-swap base revision.
5. Collapse/expand and breadcrumb focus are per-user Studio state unless the
   product explicitly promotes a view state to project semantics.
6. Drill-down never changes the accepted project or ProjectClock.
7. Deleting a referenced object follows an explicit reject/relink/tombstone
   policy; it cannot leave an untyped dangling reference.

## Transform and coordinate contract

- `1 Composition Unit = 1 Composition Pixel`.
- Persisted transforms use explicit units and checked finite numeric domains.
- `position` is expressed in parent pixels; a root Layer's parent is its
  Composition. `anchor` is expressed in the node's local pixels and may not be
  used to compensate for glyph width, ascent or other derived layout metrics.
- Local matrix candidate:

```text
M_local = T(position) * R(rotation) * Skew * S(scale) * T(-anchor)
M_world = M_parent * M_local
```

- Anchor/pivot, position, scale, rotation and skew remain independent typed
  properties; no UI-local transform cache is authoritative.
- Geometry bounds, local visual bounds, effect-expanded bounds and world bounds
  are separate measurable results.
- Opacity and blend are compositing properties and do not mutate geometry.

## Exact time contract

- Core `ProjectClock` remains the only mutable project-time authority.
- Clips, layers, animations, groups and Composition instances own ranges/maps,
  never clocks.
- All ranges are half-open and all rounding modes are named.
- Non-integer rates, source PTS, audio samples and subframe shutter samples must
  map without cumulative drift.
- Every evaluation carries revision, transport epoch, device generation,
  composition, exact time and quality profile in one `EvaluationStamp`.
- Work from a stale epoch/generation may finish but may not publish or present.

The exact representation remains subject to a measured RFC/ADR. A convenient
nanosecond value is insufficient if it cannot exactly represent admitted rates.

## Property and capability registry

Each descriptor is the single authored definition for:

- stable ID, schema version and migration;
- owner kind and typed input/output ports;
- value type, unit, default, allowed range and validation;
- animation/interpolation eligibility;
- evaluation operation and qualification profile;
- Inspector label/editor metadata;
- RFX/schema vocabulary;
- command and ChangeSet builders;
- CLI/MCP introspection and Agent Skill examples;
- diagnostics and capability-state reporting.

Generated projections must replace hand-maintained switches. Adding a built-in
descriptor is not complete if parser, QML, Agent docs and evaluator can disagree.

Each property exposes an explicit animation capability. An unavailable
animation request fails with a typed diagnostic; an Agent or UI may not emulate
it by silently duplicating Layers or changing hierarchy.

Initial value domains include booleans, enums, strings, IDs, exact time/ranges,
scalar/vector/color, matrices/transforms, geometry/path, paint, port references
and ordered descriptor lists. Every numeric field declares its unit.

## Authoring intents and topology postconditions

High-level UI/CLI/MCP operations normalize to typed intents such as
`GroupNodes`, `ReparentNodes`, `AddEffect`, `AlignNodes` and registered recipe
creation. Intent postconditions are validated with the candidate ChangeSet:

- `AddEffect` edits the target Layer's ordered FX stack and cannot create,
  reparent or reorder Visual Layers;
- `AlignNodes` measures subject and target from one accepted revision and exact
  time, then emits one atomic transform change with an explicit alignment basis;
- unsupported operations retain Last-Known-Good rather than introducing an
  undocumented topology or visual approximation;
- semantic lint may warn about suspicious authored patterns, while strict
  rejection depends on typed intent and capability contracts so intentional
  creative duplication remains representable.

## Animation contract

```text
Property<T>
  = Static<T>
  | Animated<T> { ordered Keyframe<T>, interpolation, extrapolation }
```

- Duplicate key times, invalid handles and incompatible interpolation fail
  validation.
- Initial interpolation: Step, Linear, CubicBezier.
- Evaluation is pure for `(accepted revision, exact time, context)`.
- Parent and child animation combine through normal property/transform rules;
  group animation is not keyframe duplication.
- Expressions, scripting and nondeterministic callbacks are excluded from the
  initial contract.

## Candidate evaluation and compositing order

Pending RFC golden tests, each Visual Layer evaluates:

```text
Content Source / Paint
-> Crop and ordered Layer Masks
-> ordered Local FX
-> Anchor and Transform
-> Opacity and Blend
-> Parent Composition accumulation
```

Rules:

- an effect may consume only compatible typed ports;
- the evaluator rejects cycles before building the Render Plan;
- bounds propagate before transient allocation;
- a group with no group mask/FX/opacity can be pass-through;
- group compositing behavior creates one explicit isolated offscreen pass;
- a bounded v1 Composition instance is flattened at its declared boundary;
- a backdrop input sees only the accumulated lower layers of the same parent;
- unsupported capabilities fail closed; preview may use an explicit registered
  quality tier but cannot silently change semantic ordering;
- preview and offline export compile the same semantic evaluation graph.

## Paints, sources and reusable recipes

Portable paint primitives:

- solid;
- linear/radial/sweep gradient with stable stops and interpolation policy;
- fill and stroke;
- bounded image texture input;
- explicitly registered procedural generators.

Image and Video remain Sources. Background is a creation preset. Paper, glass,
frost, glow styles and branded looks are versioned Registry recipes, not new
platform-specific layer kinds.

## Text contract

Text descriptors use a centered local TextBox with explicit width, height and
padding; paragraph direction; start/center/end and absolute left/right
horizontal alignment; top/center/bottom vertical alignment; wrap/overflow,
tracking and leading. Paragraph alignment inside a TextBox is distinct from
node-to-node alignment.

Layout produces immutable derived metrics for the accepted revision and exact
time: layout box, logical bounds, ink bounds, clipped bounds, baselines,
ascent/descent/leading, line count, overflow and resolved Font digest. Renderer
planning further distinguishes geometry, mask, effect-expanded and world bounds.
Glow and Shadow may expand visual bounds but cannot change paragraph metrics.

Text descriptors must use packaged/project Font identity and deterministic
resolution. System font names are convenience inputs only and cannot satisfy
cross-platform evidence without resolution to an admitted asset; missing or
mismatched qualified Fonts fail closed instead of silently falling back.

Core owns the portable TextBox, units, alignment and validation types. A common
Text layout port produces the derived paragraph result consumed by preview,
offline probe and Agent measurement. Backend adapters may implement shaping and
rasterization but may not define a second alignment or baseline meaning.

## Renderer boundary

Core produces an immutable, backend-neutral Evaluation/Render Plan containing
semantic operations, resource dependencies, exact bounds and stable hashes.
Backend adapters translate it to Metal/D3D/other admitted GPU commands. No
semantic behavior may live only inside a `.mm`, D3D or QML file.

Skia is a GPU-backed content/effect implementation under the engine-owned GPU
device. Ganesh versus Graphite is a qualification decision, not two product
semantics. Windows requires a real Skia/GPU execution path before cross-platform
qualification.

## Color and advanced temporal effects

- Desktop v1 target remains SDR Rec.709.
- A linear RGBA16F working pipeline is proposed and must be measured before
  acceptance on macOS and Windows.
- Glass requires an explicit backdrop port, offscreen capture, ROI, mask,
  transient budget and cache dependency.
- Transform Motion Blur requires exact shutter/subframe samples of this same
  evaluator. Gaussian blur is not a motion-blur fallback.
- Video/deformation motion blur waits for G4 media scheduling.

## Diagnostics and observability

Every rejected candidate reports stable diagnostic code, source/property ID,
source location when available, revision/base revision, and remediation hint.
Each accepted evaluation exposes:

- semantic digest and EvaluationStamp;
- parse/validate/evaluate timing;
- render pass count and effect-expanded bounds;
- transient/resident GPU bytes and cache hit/miss reason;
- device generation, queue/fence identity and GPU duration;
- zero-CPU-video-pixel counters where media is involved;
- declared capability and quality state per platform/profile.

Console, CLI, MCP and diagnostics files are projections of the same records.

## Failure containment

- Invalid syntax, schema, ports, cycles, time maps or resource references never
  replace Last-Known-Good.
- Partial external writes are reconciled as candidates, never streamed as
  partial project state.
- Backend preparation failure rejects publication of that candidate for the
  required profile or retains the compatible previous stamp.
- Cache keys include semantic hash and all relevant revision/device/dependency
  generations.
- A crash or device loss cannot mutate the canonical project document.

## Deliberate exclusions

The initial architecture does not promise a public node editor, arbitrary SkSL,
public in-process native plugin ABI, HDR/RAW, 3D, particles, tracking, roto,
collapse transformations, advanced mattes, large procedural libraries, disk
frame cache, optical-flow motion blur or full mobile Studio.

## Acceptance boundary

This candidate becomes normative only after RFC-0002 evidence is reviewed and
an ADR accepts a specific version. Until then, implementations may build
experiments but must not claim this document as an accepted shipping contract.
