# Semantic authoring and topology guardrails — EXP-006A

Project syntax validity is necessary but does not prove that the authored
topology matches the user's intent. Apply these rules before publishing a
candidate.

## Ownership rules

- A request to add Blur, Drop Shadow or Glow edits the target Layer's ordered
  `effect` declarations. It must not create a new Layer.
- A composite Background with multiple visual components is one root
  `LayerGroup`; its Shape Layers are ordered children and remain accessible by
  drill-down.
- A multi-part visual component such as a button is one LayerGroup. Internal
  Shape/Text primitives are children, not root Timeline rows.
- Masks, FX and Transform animations belong to their owning Layer or Group.
  They are not Layers, Groups or NLE Tracks.
- Preserve the existing sibling order when grouping selected nodes unless the
  user explicitly requests a visual reorder.

## Capability guardrails

Static Layer-local Gaussian Blur, Drop Shadow and Glow are supported. Animation
of Glow color, sigma, intensity or enable state is not supported in RFX5. Report
`RFX-CAP-FX-ANIMATION-001` and leave the project unchanged; never approximate
the request with duplicate Text/Shape Layers.

Authored TextBox paragraph alignment and measured node-to-node alignment are
different operations. Never use Text `anchor` to compensate for glyph width,
ascent or baseline. The engine command must name an exact Composition time,
horizontal/vertical relation and geometry/logical/ink basis. Logical/ink Text
alignment requires the admitted measurement port and the candidate must pass
the 0.25 px postcondition. Use the project-local typed Align commit. If its
requested basis is unavailable, do not imitate it with hand-guessed offsets.

## Composite Background recipe

Declare every Background component once, then make one root Group:

```rfx
group id("grp_background") name("Background") {
  range frames(0, 1800);
  transform {
    position canvas_px(0, 0);
    anchor canvas_px(0, 0);
    scale ratio(1, 1);
    rotation degrees(0);
    opacity ratio(1);
  }
  children {
    layer("lyr_background_base");
    layer("lyr_background_violet");
    layer("lyr_background_cyan");
  }
}

root {
  group("grp_background");
}
```

The Group is pass-through in the current contract. Its opacity must remain one;
its identity Transform preserves child Canvas positions.

## Local Glow recipe

Add the effect inside the existing target Layer:

```rfx
effect glow id("fx_title_glow") enabled(true) {
  sigma px(18);
  color rgba8("#7C5CFFFF");
}
```

After editing, verify that Layer, Group and root counts did not change. A new
Layer is correct only when the user explicitly requested another visual object.

## Candidate review

Before atomic replacement, compare the candidate to the active source:

1. stable IDs for unchanged objects remain identical;
2. effect-only work changes no visual topology;
3. composite Background/components have one intentional root Group;
4. unsupported animation or unavailable client command produced no approximation;
5. the candidate validates and advances the revision exactly once.

Use `outline` to verify the resulting parent path and Timeline row, `inspect`
to verify semantic property ownership, and `diff` to confirm the intended node
set. Use `measure --json` at the exact Composition time for any spatial claim.
