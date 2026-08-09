---
id: UCAS-WP07
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G2-bounded,G3-complete
owning_gate: G2
depends_on: UCAS-WP02,UCAS-WP03
decision_dependencies: render-operation-vocabulary-and-resource-policy
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,G2-WP04
evidence_owner: G2-WP04-and-PLAN-XPLAT-FIX-001
owner_role: visual-render-architecture
evidence: docs/evidence/G2/G2-WP04.md
---

# Outcome

Ensure every admitted creative primitive follows one descriptor-to-evaluator-to-
RenderPlan-to-common-Skia path and never acquires a native backend meaning.

# Dependencies

UCAS-WP02–03 and PLAN-XPLAT-FIX-001 shared renderer contracts.

# Deliverables

- versioned backend-neutral RenderPlan operation/pass vocabulary;
- resolved values, order, isolation, bounds, color, edge, crop and tile policies;
- backend-neutral resource references and declared capability/resource budgets;
- one lowering route for Preview and future Offline Export;
- common Skia compositor/resources/text execution with zero platform branches;
- thin target/context/sync/present/video-import bindings for Metal/D3D12/Vulkan;
- architecture rules forbidding Project, Recipe, Style and Receipt semantics in
  platform/native backend sources.

# Verification and exit

- one accepted revision/time yields one RenderPlan digest across toolchains;
- native files contain zero visual-authoring semantics;
- common Skia contains zero Qt or platform conditional behavior;
- unsupported operations reject before accepted publication;
- Metal and D3D execute the same plan for the admitted desktop profile;
- device-loss and resource-budget failures preserve Last-Known-Good.

# Failure and rollback

Disable admission of the new descriptor. A backend-specific implementation or
silent raster/CPU fallback is not an allowed rollback.
