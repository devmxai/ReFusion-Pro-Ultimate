---
id: EXP-004
kind: work-package
status: implementation-passed-owner-evaluation-revision-required
gate: pre-G3-experiment
owner_role: visual-authoring
evidence: docs/evidence/experiments/EXP-004.md
---

# Modern Shape and Background authoring experiment

## Outcome

Prove a portable, project-authored Shape/Background slice that can be created
from an empty workspace and edited through the same typed authority used by an
external Agent. Solid, linear and radial paint, border and bounded blend modes
must render through the engine-owned Skia GPU path and round-trip in RFX3.

## Included

- one portable 22-descriptor Registry for Transform, Shape, Text and blend;
- typed `SetVisualPropertyCommand` and Core-owned BG/SHP/TXT creation presets;
- solid, ordered multi-stop linear and radial `ShapeFill` values;
- border width/color, rounded corners and Layer blend modes normal, multiply,
  screen and overlay;
- schema-projected Inspector and accepted-revision persistence to RFX3;
- RFX1/RFX2 read compatibility and exact RFX3 round-trip;
- macOS Metal/Skia execution using the same portable semantic state.

## Excluded

Sweep/conic gradients, image textures, noise/paper/procedural generators,
Glass/backdrop sampling, color-management qualification, Image/SVG content,
Group isolation, Precomposition, Windows runtime qualification and a shipping
format decision.

## Kill criteria

Reject the experiment if QML owns Shape state/defaults, paint becomes a Skia or
Qt object in Core, UI and Agent edits produce different snapshots, canonical
save loses IDs/stops/order, or an unsupported paint is silently approximated.

## Verification

```bash
python3 tools/rfdev.py docs-doctor
python3 tools/rfdev.py architecture-check
cmake --workflow --preset macos-core
cmake --workflow --preset macos-core-sanitized
cmake --workflow --preset macos-studio
cmake --workflow --preset macos-visual
```

## Exact handoff

From a newly created empty project, add BG, SHP and TXT; select the Background,
switch among Solid/Linear/Radial, edit border and blend, and confirm Revision,
Timeline, Canvas, Inspector and Project.rfx remain coherent. Owner acceptance
is required before the experiment can inform G3.

## Owner evaluation result

The 2026-08-08 Reels review accepted the visible progress of the modern
Background paint but requires revision before this experiment informs G3. The
Agent authored the Background's animated paint components as independent root
Layers instead of one Background LayerGroup, and the current Text model cannot
measure professional Text-to-Shape alignment. EXP-006 owns the bounded
topology/layout correction; broad materials and animated FX remain deferred.
