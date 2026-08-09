---
id: UCAS-WP06
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G2-bounded,G3-breadth
owning_gate: G2
depends_on: UCAS-WP03,UCAS-WP05
decision_dependencies: recipe-truth-receipt-lifecycle-ADR
cross_plan_dependencies: MP-001,G2-WP06
evidence_owner: G2-WP06
owner_role: recipe-ownership
evidence: docs/evidence/G2/G2-WP06.md
---

# Outcome

Preserve Recipe provenance and controlled updates without creating a second
visual truth or silently overwriting manual user edits.

# Dependencies

UCAS-WP03, UCAS-WP05 and accepted Recipe-truth ADR.

# Deliverables

- non-rendering PresetApplicationReceipt schema;
- recipe/version/dependency/profile/parameter and managed-materialization digests;
- stable slot-to-entity/channel ownership mapping;
- attached, detached, stale, unresolved and baked lifecycle states;
- atomic Apply, Update, Detach, Reattach, Reset, Bake, Remove and Upgrade intents;
- manual-edit interception with deterministic detachment;
- explicit upgrade preview/diff and receipt migration;
- missing/revoked package behavior preserving materialized Last-Known-Good.

# Verification and exit

- renderer and evaluator contain zero Receipt reads;
- deleting the catalog leaves current visuals unchanged;
- manual edits are never silently overwritten by reapply/update;
- state and Receipt commit atomically and crash recovery yields a complete pair;
- Bake removes management metadata only; Upgrade never occurs on open;
- save/reopen/migrate/Undo/Redo and missing-package fault corpus pass.

# Failure and rollback

Bake or freeze Receipts into normal materialized state and disable Recipe
updates. Never delete materialized output as rollback.
