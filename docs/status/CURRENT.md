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
last_green_commit: dba78d8
last_checkpoint: CP-G0-0003
blocking_risks:
  - RISK-002
last_updated: 2026-08-07
---

# Current program status

## Exact resume point

`G0-WP02` passed after the product owner selected Qt Commercial in ADR-0005.
`G0-WP03` code and CI definition are complete at checkpoint `CP-G0-0003`.
macOS Debug, Release, sanitizer, Core and Studio evidence is green. The exact
remaining item is a real Windows x64 execution of the committed CI workflow.

## Read next

1. `docs/plans/stages/G0-foundation/PLAN.md`
2. `docs/architecture/INVARIANTS.md`
3. `docs/product/PRODUCT_CONTRACT.md`

## Next actions

1. Connect a CI-capable Git remote or Windows x64 runner.
2. Run `.github/workflows/portable-core.yml` and retain the Windows run URL,
   runner image, MSVC/CMake versions, and test output.
3. If green, mark G0-WP03 passed and activate G0-WP04.
4. Do not confuse portable Windows compilation with G1 GPU/media qualification.


## Do not repeat

- Do not turn the research draft into an implementation plan.
- Do not duplicate the existing official Skia checkout until dependency intake
  decides whether to reuse a local cache or fetch the pinned revision.
- Do not start renderer/media feature work before G1 entry criteria.
- Do not repeat Qt/Skia/media/font intake research; use ADR-0005 through ADR-0007.
- Do not reopen the Qt licensing lane without a superseding ADR.
