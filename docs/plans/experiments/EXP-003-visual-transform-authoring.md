---
id: EXP-003
kind: work-package
status: implementation-passed-owner-evaluation-pending
gate: pre-G2-experiment
owner_role: studio-authoring
evidence: docs/evidence/experiments/EXP-003.md
---

# Typed visual-transform authoring experiment

## Outcome

Prove the first bounded UI authoring loop on the accepted project authority:
select one stable Layer or LayerGroup from the Timeline, inspect its typed
Transform2D, submit one atomic command and publish the resulting accepted
revision coherently to Inspector, Timeline, Canvas runtime and Project.rfx.

## Authorization and decision boundary

The product owner instructed continued implementation on 2026-08-08 after
reviewing the exact EXP-002 versus G2/G3 capability boundary. This authorizes a
bounded pre-G2 experiment only. It does not activate G2, accept RFC-0001 or
RFC-0002, adopt a shipping schema, complete the registry/ChangeSet system, or
waive Windows evidence.

## Included

- one typed `SetVisualTransformCommand` targeting `LayerId` or `LayerGroupId`;
- Revision/CAS, idempotency, validation and Last-Known-Good behavior in Core;
- Position, Anchor, Scale, Rotation and Opacity as one atomic Transform2D;
- Timeline selection as Studio-only view state keyed by the stable node ID;
- read-only Inspector projection from the active ProjectSnapshot;
- runtime activation and canonical Project.rfx persistence after acceptance;
- rejection of unknown nodes, invalid numeric transforms and unsupported
  pass-through Group opacity;
- Core, Studio, live-reload, persistence and macOS Metal/Skia lane tests.

## Excluded

General descriptor/property registry, per-property ChangeSets, multi-selection,
drag transforms, visual gizmos, animated-property conflict UX, Undo/Redo,
Image/Video/Audio/SVG authoring, masks, FX, isolated Group compositing,
Precomposition, export and Windows runtime qualification.

## Kill criteria

Reject the experiment if QML owns a mutable project or transform cache, if a UI
edit bypasses RevisionAuthority, if rejected values alter Canvas or Project.rfx,
if runtime and persistence receive different accepted snapshots, or if the
command introduces Qt/Skia/platform types into Core.

## Verification

```bash
python3 tools/rfdev.py docs-doctor
python3 tools/rfdev.py architecture-check
cmake --workflow --preset macos-core
cmake --workflow --preset macos-studio
cmake --workflow --preset macos-visual
```

## Evidence path

`docs/evidence/experiments/EXP-003.md`.

## Failure and rollback

Removing the command and Studio projection restores the read-only EXP-002
hierarchy view. Existing RFX1/RFX2 projects remain readable; invalid commands do
not change the accepted revision or project file.

## Exact handoff

The owner opens an RFX2 grouped project, selects a Layer and then a Group,
changes a visible transform, and confirms the revision, Canvas, Inspector and
canonical Project.rfx update together. The next implementation package remains
the accepted registry/ChangeSet decision path, not broad FX switches in QML.
