---
id: EVID-XPF-WP06A-LOCAL-2026-08-08
kind: implementation-evidence
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP06
scope: portable-contribution-registry-rfx5-render-plan-inspector-agent-matrix
status: passed-local-not-cross-platform-qualified
date: 2026-08-08
---

# XPF-WP06A local contribution-admission receipt

## Outcome

One portable Core registry now owns the admitted Mask/FX contribution IDs,
capability IDs, schema version, owners, typed parameters, units, defaults,
ranges, validation, Inspector metadata and Agent documentation for Rounded
Rectangle Mask, Gaussian Blur, Drop Shadow and Glow.

Studio no longer contains per-FX defaults or type-specific Inspector/QML
branches. Its catalog, add controls, parameter editors and Timeline names are
descriptor projections. A newly created project receives a generated
`references/visual-contributions.md` from those same descriptors. Typed scale
and opacity keyframes now fail before publication when they leave their
admitted ranges.

RFX5 binds canonical bytes to the contribution Registry digest and persists
every current Mask/FX through one generic, ordered typed-parameter codec.
RFX1–RFX4 remain readable migration inputs. Unknown or missing RFX5
contribution parameters fail closed. The same descriptor identity is lowered
into each RenderPlan contribution as descriptor ID, capability ID and schema
version, and those fields participate in the semantic digest executed by the
common Skia compositor.

## Claim-state guard

`contracts/visual/cross-platform-capability-matrix.json` records every current
Registry capability separately for macOS Metal, Windows D3D12, iOS Metal
canary and Android Vulkan canary. The repository guard verifies:

- the Matrix and Registry capability sets are exact;
- every profile supplies every ordered boolean state;
- a later state cannot become true while an earlier evidence state is false;
- `qualified` is true only when every prerequisite is true;
- a source-defined profile has an evidence reference.

The truthful current state is deliberate. macOS has local compile/run and
semantic evidence but no calibrated cross-backend pixel/performance
qualification. Windows is source-defined but not compiled or physically run in
this lab. Mobile canaries remain undefined. None is marked qualified.

## Boundary reduction

The active visual-boundary debt decreased from 26 occurrences to zero.
Studio submits only descriptor-addressed contribution intent; Application
constructs the candidate stack and submits it through atomic admission. All
concrete Blur/Shadow/Glow/rounded-mask names, generic project-stack types, QML
kind dispatch and Timeline type-selection switches were removed from Studio.
The immutable frozen baseline remains historical; there are no active
allowances.

## Local verification

```text
cmake --build --preset macos-visual --parallel 8
  passed

ctest --preset macos-visual --output-on-failure
  48/48 passed

python3 tools/rfdev.py architecture-check
  checked_source_files: 105
  active visual-boundary occurrences: 0
  problems: []
```

## Formal WP06 work still open

The locally executable architecture route is complete. Formal closure still
requires the Windows/mobile matrix receipts and final evidence that each
shipping backend's advertised contribution set is rejected before publication
when unavailable. This macOS receipt does not promote any profile to
cross-platform qualified.
