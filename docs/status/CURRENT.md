---
id: STATUS-001
kind: program-status
master_plan: MP-001
master_plan_status: active
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

`G0-WP02` is active. MP-001 and the Product Contract are activated. Engineering
ADRs 0001–0004 are accepted; OS/media and font profiles are proposed. The only
owner-blocking choice is ADR-0005: Qt Commercial versus an explicit LGPLv3
compliance program.

## Read next

1. `docs/plans/stages/G0-foundation/PLAN.md`
2. `docs/architecture/INVARIANTS.md`
3. `docs/product/PRODUCT_CONTRACT.md`

## Next actions

1. Product owner selects the Qt distribution lane in ADR-0005.
2. Freeze the selected lane in `deps/policies/qt-modules.json`.
3. Review the deliberately narrow ADR-0006 Media/OS target and ADR-0007 fonts.
4. Close G0-WP02 evidence, then activate G0-WP03 and Windows x64 CI.


## Do not repeat

- Do not turn the research draft into an implementation plan.
- Do not duplicate the existing official Skia checkout until dependency intake
  decides whether to reuse a local cache or fetch the pinned revision.
- Do not start renderer/media feature work before G1 entry criteria.
