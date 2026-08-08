---
id: EVID-XPF-WP08A-LOCAL-2026-08-09
kind: implementation-evidence
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP08
scope: shared-preview-offline-export-visual-render-plan-contract
status: passed-local-contract-export-product-not-implemented
date: 2026-08-09
---

# XPF-WP08A local Preview/Export semantic-parity receipt

## Outcome

Runtime now owns one consumer-neutral visual-output preparation route. Both
interactive Preview and the future Offline Export consumer must call
`prepare_visual_output_frame(...)`, which evaluates the accepted
`VisualRenderProgram` at one exact `ProjectTime` and produces the same immutable
`VisualRenderPlan`. Consumer identity is metadata and cannot participate in
scene evaluation, operation lowering, effect ordering or semantic-digest
construction.

The production Skia Preview executor now enters through that shared contract.
`compare_visual_output_semantics(...)` fails closed unless it receives one
Preview frame and one Offline Export frame for the same project, Revision,
Composition, exact project time, canvas and RenderPlan digest. Independent
clock epochs remain legal because Preview and Export scheduling are separate;
they may not change project semantics.

## Verification

The repository-owned contract fixture proves:

1. Preview and Offline Export resolve the same semantic digest at the same
   exact project time, even with different scheduler epochs;
2. different project times fail parity;
3. two Preview frames cannot masquerade as Preview/Export evidence;
4. unknown consumers fail closed.

Verification results:

```text
ctest --preset macos-visual --output-on-failure
  49/49 passed

cmake --workflow --preset ios-graphics-canary
  BUILD SUCCEEDED

python3 tools/rfdev.py architecture-check
  checked_source_files: 110
  visual_boundary_debt.occurrences: 0
  problems: []

python3 tools/rfdev.py docs-doctor
  checked_documents: 103
  problems: []

python3 tests/tools/rfdev_policy_test.py
  passed
```

The iPhoneOS arm64 Graphics build compiled both the new Runtime contract and
the shared Skia Preview consumer, demonstrating that this semantic route is not
macOS-only source.

## Claim boundary

This is the locally executable WP08 semantic-consumer slice, not G4 Export and
not full WP08 qualification. ReFusion does not yet claim a production export
pipeline, reference-image equivalence between Preview and encoded output,
Windows physical execution, cross-backend calibrated pixel tolerance,
performance budgets, recovery or device-loss qualification. Those require the
remaining Windows/Android and later admitted G4/profile receipts.
