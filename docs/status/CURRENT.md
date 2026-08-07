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
last_green_commit: b6cf27c
last_checkpoint: CP-G0-0002
blocking_risks:
  - RISK-002
last_updated: 2026-08-07
---

# Current program status

## Exact resume point

`G0-WP02` passed after the product owner selected Qt Commercial in ADR-0005.
`G0-WP03` is active: strengthen the typed command/revision baseline, prove
Last-Known-Good and concurrent expected-revision behavior, and add the portable
macOS/Windows Core CI definition.

## Read next

1. `docs/plans/stages/G0-foundation/PLAN.md`
2. `docs/architecture/INVARIANTS.md`
3. `docs/product/PRODUCT_CONTRACT.md`

## Next actions

1. Add typed command identity and idempotency envelopes to Core.
2. Expand deterministic rejection, replay, Last-Known-Good and concurrency tests.
3. Add pinned macOS/Windows portable-Core CI and record local macOS evidence.
4. Keep Windows runtime evidence pending until an actual Windows runner reports.


## Do not repeat

- Do not turn the research draft into an implementation plan.
- Do not duplicate the existing official Skia checkout until dependency intake
  decides whether to reuse a local cache or fetch the pinned revision.
- Do not start renderer/media feature work before G1 entry criteria.
- Do not repeat Qt/Skia/media/font intake research; use ADR-0005 through ADR-0007.
- Do not reopen the Qt licensing lane without a superseding ADR.
