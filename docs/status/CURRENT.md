---
id: STATUS-001
kind: program-status
master_plan: MP-001
master_plan_status: active
current_gate: G0
gate_status: active
active_work_packages:
  - G0-WP03
  - G0-WP04B
  - G0-WP05
baseline_commit: 1a70f5a
last_green_commit: 24d5946e442de09ac9ccc798f9e7aeedeee04502
last_checkpoint: CP-G0-0006
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
Checkpoint `CP-G0-0005` passed `G0-WP04A`: one private Application Host now
owns mutable project authority, while Studio and CLI submit commands through the
same service. The local controls in `G0-WP04B` are code complete and fail closed;
Windows materialization/build/runtime still requires external evidence. By the
product owner's direction in `CP-G0-0006`, Qt Commercial SDK and entitlement
verification is explicitly deferred to the redistributable release-candidate
gate and does not block G0/G1 engineering. `G0-WP05` is active only to close the
technical readiness record; G1 remains gated by the Windows proofs.

## Read next

1. `docs/plans/stages/G0-foundation/PLAN.md`
2. `docs/architecture/INVARIANTS.md`
3. `docs/product/PRODUCT_CONTRACT.md`

## Next actions

1. Connect a CI-capable Git remote or Windows x64 runner; run portable Core and
   retain the run URL, image and compiler evidence.
2. Materialize the repository-local Windows Skia graph and lock on that clean
   runner; implement and prove the D3D/Dawn same-device context before accepting
   `windows-graphics`.
3. Do not run or accept `windows-graphics` as same-device proof until G0-WP05
   defines and G1 implements the missing D3D/Dawn context, link and runtime test.
4. Complete G0-WP05 and perform the criterion-by-criterion G0 exit review.


## Do not repeat

- Do not turn the research draft into an implementation plan.
- Do not reuse, search for, or copy any external Skia checkout or machine cache;
  only the ReFusion-local clean bootstrap path is admitted.
- Do not start renderer/media feature work before G1 entry criteria.
- Do not repeat Qt/Skia/media/font intake research; use ADR-0005 through ADR-0007.
- Do not reopen the Qt licensing lane without a superseding ADR.
- Do not request or claim Qt Commercial SDK/entitlement evidence during G0/G1;
  preserve the fail-closed release gate for the later redistributable RC.
- Do not re-implement the Application Host boundary or the macOS Skia lock; use
  source commit `24d5946e442de09ac9ccc798f9e7aeedeee04502`.
