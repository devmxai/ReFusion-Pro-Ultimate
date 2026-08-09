---
id: UCAS-WP11
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G3
owning_gate: G3
depends_on: UCAS-WP02,UCAS-WP05,UCAS-WP06,UCAS-WP07,UCAS-WP08B
decision_dependencies: shape-geometry-operator-bounds-ADRs
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,future-G3-stage-plan
evidence_owner: future-G3-stage-plan
owner_role: vector-shape-authoring
evidence: docs/evidence/UCAS/UCAS-WP11.md
---

# Outcome

Deliver a professional editable vector/Shape system with stable internal content
hierarchy, typed paints and bounded geometry operators.

# Dependencies

UCAS-WP02, UCAS-WP05–08B and active G3.

# Deliverables

- rectangle, rounded rectangle, ellipse, polygon, star and Bezier path;
- solid/gradient/pattern Fill and typed Stroke controls;
- rounded corners, trim, repeater, offset, boolean and path-reveal operators as
  separately qualified descriptors;
- stable internal ContentNode ownership and deterministic geometry/effect bounds;
- Shape Recipes for solid, gradient, glass-ready input, paper, outline, sticker
  and editorial annotation styles using only admitted primitives;
- topology and complexity budgets plus explicit operator ordering.

# Verification and exit

- internal primitives/operators do not become root Timeline rows;
- invalid paths, cycles and over-budget geometry reject;
- save/reopen and migration preserve stable IDs and ordering;
- UI/Agent parameters produce the same normalized project meaning;
- semantic, pixel and performance fixtures pass on qualified desktop profiles.

# Failure and rollback

Preserve unknown operator state unresolved and disable its execution. Do not
rasterize destructively or create platform-specific approximations.
