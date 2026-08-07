---
id: STATUS-001
kind: program-status
master_plan: MP-001
master_plan_status: proposed
current_gate: G0
gate_status: active
active_work_packages:
  - G0-WP02
baseline_commit: 1a70f5a
last_green_commit: 1a70f5a
last_checkpoint: CP-G0-0001
blocking_risks:
  - RISK-001
  - RISK-002
last_updated: 2026-08-07
---

# Current program status

## Exact resume point

`G0-WP01` is technically complete at checkpoint `CP-G0-0001`. Resume with
`G0-WP02`: owner review of MP-001/Product Contract and decisions for Qt
licensing/modules, strict media policy, Skia intake, codecs, and fonts.

## Read next

1. `docs/plans/stages/G0-foundation/PLAN.md`
2. `docs/architecture/INVARIANTS.md`
3. `docs/product/PRODUCT_CONTRACT.md`

## Next actions

1. Review/accept or revise MP-001 and Product Contract.
2. Decide ADR-0001 through ADR-0004 or assign their blocking experiments.
3. Record Qt license/module, media/codec/font, and dependency intake decisions.
4. Start G0-WP03 and establish Windows x64 CI before claiming portability.


## Do not repeat

- Do not turn the research draft into an implementation plan.
- Do not duplicate the existing official Skia checkout until dependency intake
  decides whether to reuse a local cache or fetch the pinned revision.
- Do not start renderer/media feature work before G1 entry criteria.
