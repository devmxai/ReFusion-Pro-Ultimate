---
id: STATUS-001
kind: program-status
master_plan: MP-001
master_plan_status: active
current_gate: G0
gate_status: active
active_work_packages:
  - G0-WP03
  - G0-WP04A
  - G0-WP04B
baseline_commit: 1a70f5a
last_green_commit: 075be91
last_checkpoint: CP-G0-0004
blocking_risks:
  - RISK-002
  - RISK-003
last_updated: 2026-08-07
---

# Current program status

## Exact resume point

`G0-WP02` passed after the product owner selected Qt Commercial in ADR-0005.
`G0-WP03` Core code and CI definition remain complete but still need a real
Windows x64 run. By explicit owner direction, checkpoint `CP-G0-0004` also
materialized a fresh official Skia dependency graph inside ReFusion and proved
the engine-owned Metal plus same-device Ganesh/Graphite contract on macOS.
The 2026-08-07 independent audit activated `G0-WP04A` and `G0-WP04B` to correct
authority ownership and dependency/toolchain admission. G1 itself remains gated
and is not activated by this preflight or correction work.

## Read next

1. `docs/plans/stages/G0-foundation/PLAN.md`
2. `docs/architecture/INVARIANTS.md`
3. `docs/product/PRODUCT_CONTRACT.md`

## Next actions

1. Complete and prove `G0-WP04A` authority/boundary correction locally.
2. Complete the local `G0-WP04B` provenance and release-admission controls.
3. Connect a CI-capable Git remote or Windows x64 runner; run portable Core and
   retain the run URL, image and compiler evidence.
4. Do not run or accept `windows-graphics` as same-device proof until G0-WP05
   defines and G1 implements the missing D3D/Dawn context, link and runtime test.
5. Complete G0-WP05 and perform the criterion-by-criterion G0 exit review.


## Do not repeat

- Do not turn the research draft into an implementation plan.
- Do not reuse, search for, or copy any external Skia checkout or machine cache;
  only the ReFusion-local clean bootstrap path is admitted.
- Do not start renderer/media feature work before G1 entry criteria.
- Do not repeat Qt/Skia/media/font intake research; use ADR-0005 through ADR-0007.
- Do not reopen the Qt licensing lane without a superseding ADR.
