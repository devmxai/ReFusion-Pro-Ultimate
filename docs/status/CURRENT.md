---
id: STATUS-001
kind: program-status
master_plan: MP-001
master_plan_status: active
current_gate: G0
gate_status: active
active_work_packages:
  - G0-WP03
baseline_commit: 1a70f5a
last_green_commit: 075be91
last_checkpoint: CP-G0-0004
blocking_risks:
  - RISK-002
last_updated: 2026-08-07
---

# Current program status

## Exact resume point

`G0-WP02` passed after the product owner selected Qt Commercial in ADR-0005.
`G0-WP03` Core code and CI definition remain complete but still need a real
Windows x64 run. By explicit owner direction, checkpoint `CP-G0-0004` also
materialized a fresh official Skia dependency graph inside ReFusion and proved
the engine-owned Metal plus same-device Ganesh/Graphite contract on macOS.
G1 itself remains gated and is not activated by this preflight.

## Read next

1. `docs/plans/stages/G0-foundation/PLAN.md`
2. `docs/architecture/INVARIANTS.md`
3. `docs/product/PRODUCT_CONTRACT.md`

## Next actions

1. Connect a CI-capable Git remote or Windows x64 runner.
2. Run `.github/workflows/portable-core.yml` and retain portable-Core evidence.
3. Materialize the locked official sources inside the Windows checkout, build
   profile `windows-x64-d3d12`, and run preset `windows-graphics`.
4. Only after Windows evidence, close G0-WP03 and continue the ordered gates.


## Do not repeat

- Do not turn the research draft into an implementation plan.
- Do not reuse, search for, or copy any external Skia checkout or machine cache;
  only the ReFusion-local clean bootstrap path is admitted.
- Do not start renderer/media feature work before G1 entry criteria.
- Do not repeat Qt/Skia/media/font intake research; use ADR-0005 through ADR-0007.
- Do not reopen the Qt licensing lane without a superseding ADR.
