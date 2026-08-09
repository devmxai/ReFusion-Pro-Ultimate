---
id: UCAS-WP12A
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G3
owning_gate: G3
depends_on: UCAS-WP07,UCAS-WP08B,UCAS-WP12
decision_dependencies: adjustment-scope-isolation-color-order-ADR
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,G3
evidence_owner: future-G3-stage-plan
owner_role: adjustment-and-color
evidence: docs/evidence/UCAS/UCAS-WP12A.md
---

# Outcome

Implement the bounded Adjustment capability required by G3 as an explicit
scoped visual owner, not a hidden global renderer state or backend feature.

# Deliverables

- typed Adjustment descriptor, target scope and deterministic order;
- color/exposure/contrast/saturation and separately admitted operations;
- exact isolation, bounds, blend, alpha, working-space and output-transfer rules;
- compatible animation through the canonical curve system;
- Inspector/Timeline/Agent ownership and measurement projections;
- common RenderPlan operations shared by Preview and Export.

# Verification and exit

- scope/order survives save/reopen/migration and is never inferred by UI order;
- unsupported ports or color profiles fail before accepted publication;
- no native backend contains Adjustment semantics;
- UI/Agent parity and macOS/Windows semantic/visual/performance evidence pass;
- one Adjustment owner does not materialize one root Layer per internal operation.

# Failure and rollback

Disable the unqualified operation/profile and preserve unresolved project state
and Last-Known-Good. No platform color patch is allowed.

