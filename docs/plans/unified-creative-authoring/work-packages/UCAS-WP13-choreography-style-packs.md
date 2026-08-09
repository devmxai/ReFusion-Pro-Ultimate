---
id: UCAS-WP13
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: late-G3,G5-integration
owning_gate: G3
depends_on: UCAS-WP09A,UCAS-WP09,UCAS-WP10,UCAS-WP11,UCAS-WP12,UCAS-WP12A
decision_dependencies: choreography-relations-style-pack-ADR
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,future-G3-stage-plan
evidence_owner: future-G3-stage-plan
owner_role: scene-choreography
evidence: docs/evidence/UCAS/UCAS-WP13.md
---

# Outcome

Compose qualified primitives into deterministic professional scenes and Style
Packs without adding branded renderer types or hidden project semantics.

# Dependencies

UCAS-WP09A–12A, hierarchy, exact measurement and active G3.

# Deliverables

- semantic roles, timing relations, scene phases and one resolved sibling order;
- safe-area, focal-bound, readability, collision, hold, stagger and overlap rules;
- deterministic relation-DAG solver producing exact frame times;
- Style Profile/Pack descriptors and parameterized scene Recipes;
- first integration pack: neutral Editorial Paper-Collage Explainer with paper
  background, cutout border, bold headline, marker annotation, cutout-pop and
  annotation-draw Recipes;
- stable topology, asset/font licenses and detachable owned parameters;
- marketing names remain aliases/search metadata, not semantic types.

# Verification and exit

- cycles, unresolved constraints and collisions reject before commit;
- one atomic scene ChangeSet produces hierarchy, order and exact timing;
- every macro slider maps to real typed values;
- detach/reapply/bake/save/reopen and UI/Agent/MCP parity corpus passes;
- no effect/path/operator becomes an unintended root Timeline row;
- desktop visual/readability/performance receipts pass for the reference scene.

# Failure and rollback

Remove the pack from the default catalog without deleting pinned versions or
materialized projects. Apply/Update fails closed if a dependency is unavailable.
