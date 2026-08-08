---
id: EV-VA-0001
kind: owner-review-evidence
status: recorded-revision-required
date: 2026-08-08
subject: reels-agent-authoring
master_plan: MP-001
stage_plan: G2
---

# Reels visual-authoring owner review

## Purpose

Record the product owner's real Agent-authoring evaluation without converting
an external workspace, a screenshot, or a macOS observation into a shipping or
cross-platform claim. This record routes every finding to one canonical work
package; it is not a second plan or architecture authority.

## Evaluated state

The external owner workspace compiled as valid experimental RFX3 and opened as
one accepted project revision. Inspection of its canonical project source found
15 Layers, one LayerGroup and nine root visual nodes.

The review confirmed that these implemented mechanisms work:

- real Composition dimensions, exact frame ranges and stable IDs survive into
  Canvas, Timeline and the canonical project source;
- a seven-child Subscribe component is represented by one `LayerGroup` and one
  collapsed root Timeline row with drill-down;
- the title's Drop Shadow is an ordered local effect owned by the Text Layer;
- modern solid/linear/radial Shape paint, border, blend, local masks and the
  bounded static FX stack render through the macOS Metal/Skia path;
- accepted external edits update Canvas, Timeline and project source through
  the existing revision authority.

These observations do not prove Windows runtime behavior, a shipping RFX
format, complete typography, Precomposition, Group isolation or G2/G3 exit.

## Revision-required findings

1. The composed Background was authored as four independent Shape Layers in
   the Composition root. Timeline correctly projected four rows because the
   source did not place those Layers in one Background LayerGroup.
2. The title Glow was implemented by three duplicate Text Layers with different
   local Glow effects and animated Layer opacity. The original Text Layer's
   Drop Shadow remained local, but the duplicated Glow topology violated the
   requested effect ownership and cluttered the Timeline.
3. RFX3 can animate Transform and Layer opacity but cannot animate Glow color,
   sigma, intensity or enable state. The Agent silently approximated the
   unsupported request instead of failing closed.
4. Text state has `layout_width` but no authored TextBox height, paragraph
   horizontal/vertical alignment, line metrics, logical/ink bounds, packaged
   Font Asset identity or measurable baseline contract.
5. The renderer places the shaped line from a fixed layout origin. The Agent
   compensated with hand-authored anchor offsets, so Text-to-Shape centering is
   dependent on one string, font and platform rather than a measured command.
6. The CLI/Skill do not yet provide complete `outline`, `inspect`, `measure`,
   `capabilities`, semantic `lint`, `diff` and typed `commit` operations. A
   syntactically valid candidate can therefore violate the user's topology or
   effect-ownership intent.
7. Group creation/reparenting and topology-preserving `AddEffect`/`AlignNodes`
   are not yet first-class typed ChangeSets shared by UI, CLI and MCP.

## Required semantic decisions

- A composite Background is one root LayerGroup whose internal Layers remain
  addressable through drill-down.
- An effect request edits the target Layer's ordered FX stack and may not create
  a Layer unless the user explicitly requests a new visual Layer.
- Unsupported animated FX fail with one typed capability diagnostic while
  Last-Known-Good remains active; duplicate-Layer approximation is forbidden.
- Canvas dimensions are authored truth, while local/logical/ink/effect/world
  bounds are derived measurements from the accepted revision at an exact time.
- `position` is expressed in parent pixels and `anchor` in local pixels. Anchor
  is not a typography-compensation field.
- Text paragraph alignment and node-to-node alignment are separate operations.
  The latter is a measured Core command, not an Agent coordinate guess.
- Effects, masks and animation-property lanes are children of their owning
  semantic node in Studio projections; they are not Visual Layers or NLE
  Tracks.

## Plan routing

| Finding | Canonical owner |
|---|---|
| TextBox, alignment, typed bounds and effect ownership | G2-WP02 |
| Atomic GroupNodes, AddEffect and AlignNodes intents | G2-WP03 |
| exact-time Text layout and local/logical/ink/effect/world measurement | G2-WP04 |
| one root Group row and owner-local FX/animation projections | G2-WP05 |
| inspect/measure/capabilities/lint/diff/commit and generated Agent rules | G2-WP06 |
| Reels regression, save/reopen, parity and desktop evidence | G2-WP07 |
| animatable arbitrary FX properties and broad motion/easing | G3 planning |

Immediate bounded remediation is planned by `EXP-006`; G2 remains proposed and
RFC-0002 remains undecided.

## Owner disposition of preceding experiments

- `EXP-002`: implementation passed; owner evaluation is partial. LayerGroup,
  collapse and drill-down work, while general Agent topology authoring remains
  incomplete.
- `EXP-004`: implementation passed; owner evaluation requires revision for
  composite Background authoring and measurable layout.
- `EXP-005`: implementation passed; owner evaluation requires revision for FX
  ownership, unsupported animated-FX behavior and Agent topology guardrails.

## Acceptance target for the regression fixture

- root Timeline projection contains exactly Background Group, Title Layer and
  Subscribe Group for the bounded reference scene;
- opening Background reveals its ordered internal visual Layers;
- the title exists once and owns Drop Shadow and Glow in its FX stack;
- adding a static Glow changes neither Layer count nor root count;
- unsupported animated Glow rejects without changing the accepted revision;
- Text and Shape centers differ by at most `0.25` Composition pixel before
  rasterization for the declared alignment basis;
- save/reopen preserves IDs, hierarchy, order, alignment and FX ownership;
- equivalent UI and Agent intents yield equal normalized semantic meaning and
  accepted digest.
