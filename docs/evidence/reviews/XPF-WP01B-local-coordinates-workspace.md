---
id: EVID-XPF-WP01B-LOCAL-2026-08-08
kind: architecture-implementation-evidence
status: passed
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP01B-local
platform: portable-core-plus-macos-qt-adapter
date: 2026-08-08
---

# XPF-WP01B local coordinates/workspace evidence

## Canonical command coordinates

Core now commits UI/property and measured-alignment pixel values on a
binary-exact `1/1024 px` grid. Position and anchor fields in typed Transform
commands, numeric pixel properties, and base/keyframed positions translated by
`AlignNodes` pass through the same quantizer before Revision publication.
Scale, rotation, opacity and explicit existing RFX literals are not silently
treated as pixels or rewritten.

The checked-in `expected-commands.txt` receipt applies Transform, property and
rotated/nested measured Align commands to the Arabic/Latin cross-platform
fixture under a synthetic decimal-comma locale. It binds the exact final
Revision, canonical byte size, project digest and authored coordinates. The
canonical output recompiles to the identical snapshot.

## Relocated workspace regeneration

Studio classifies `.refusion` as host-local generated state by comparing its
recorded canonical project path with the opened project. A proven relocated or
copied workspace replaces only its copied local lock/context/cache/journal/
diagnostics, then creates a new local session. An unknown state cannot bypass
an active lock.

The integration test copies `.refusion` while the source Studio still owns its
lock, opens the copied Project at another path, and proves:

- the source active session remains protected;
- a second open of the same path fails closed;
- the copied lock does not falsely block the destination;
- foreign cache/journal/diagnostic sentinels are removed;
- context, diagnostics and journal are regenerated for the destination path;
- Agent context schema v2 stores the full unsigned Revision as a decimal
  string rather than a lossy signed/JSON number.

## Verification

```text
refusion.canonical_coordinates                  PASS
refusion.xplat_command_conformance              PASS
refusion.project_authority                      PASS
refusion.measured_alignment                     PASS
refusion.visual_property_registry               PASS
refusion.agent_authoring_guide                  PASS
refusion.cli_agent_surface                      PASS
refusion.project_workspace_creator              PASS
refusion.project_live_reload                    PASS
```

## Claim boundary

This closes WP01B locally, not the WP01 cross-platform exit. The command receipt
still requires execution under MSVC; a real Mac-create/Windows-open/canonical-
resave pass and iOS/Android compile canaries remain `not-run`.
