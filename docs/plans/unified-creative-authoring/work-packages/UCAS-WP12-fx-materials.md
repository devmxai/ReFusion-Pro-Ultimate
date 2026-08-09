---
id: UCAS-WP12
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G3
owning_gate: G3
depends_on: UCAS-WP02,UCAS-WP06,UCAS-WP07,UCAS-WP08B,UCAS-WP09,UCAS-WP10,UCAS-WP11
decision_dependencies: FX-isolation-color-bounds-glass-ADRs
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,future-G3-stage-plan
evidence_owner: future-G3-stage-plan
owner_role: effects-and-materials
evidence: docs/evidence/UCAS/UCAS-WP12.md
---

# Outcome

Expand FX, Layer Styles and materials through descriptors and common render
operations so compatible owners share one implementation on all platforms.

# Dependencies

UCAS-WP02, UCAS-WP07–11 and active G3.

# Deliverables

- named-corpus Blur, Drop Shadow and Glow parameter contracts for each admitted
  descriptor/profile;
- inner/outer shadow/glow, color/gradient overlay, stroke, bevel and satin only
  when their isolation/color/bounds policies are accepted;
- animated FX properties over the one curve truth;
- reusable Layer Style/Material Recipes and parameter macros;
- explicit effect order, isolation bounds, expansion, crop/tile/edge, alpha,
  blend and color-space rules;
- Glass only after an accepted backdrop-capture/isolation contract;
- preview/export quality profiles with identical semantic meaning.

# Verification and exit

- FX stays owner-local and never becomes a duplicate visual Layer;
- native backend files contain zero FX/material meaning;
- UI/Agent/MCP parity, canonical persistence and migration pass;
- over-budget or unsupported effects fail before accepted publication;
- Metal/D3D visual and performance qualification is recorded honestly.

# Failure and rollback

Unregister or capability-gate the affected descriptor/profile, preserve
unresolved state and keep Last-Known-Good. No silent CPU/native fallback.
