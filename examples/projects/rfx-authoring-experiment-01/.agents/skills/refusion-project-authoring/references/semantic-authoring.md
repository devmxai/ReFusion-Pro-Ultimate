# Semantic authoring guardrails

- FX and masks are owned declarations inside their target Layer; they never
  become Timeline Layers or Tracks.
- A composite Background or component is one root Group with ordered children.
- Preserve sibling stacking order and stable IDs unless the user explicitly
  requests reordering or replacement.
- Use typed measured alignment at an exact Composition time. Never move Text
  anchors to guess glyph width, ascent or baseline.
- Effect animation is unavailable with `RFX-CAP-FX-ANIMATION-001`; never fake
  it with duplicate Text/Shape Layers.

Before publication, use `outline` for parent path/Timeline row, `inspect` for
property ownership, `measure --json` for exact bounds and `diff` for the exact
changed node set. A valid effect-only edit changes no visual topology.
