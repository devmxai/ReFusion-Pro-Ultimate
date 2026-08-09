---
id: UCAS-WP15
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: mobile-canaries,G9-productization
owning_gate: G9
depends_on: UCAS-WP14
decision_dependencies: mobile-gateway-offline-envelope-security-ADRs
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,G9
evidence_owner: future-G9-stage-plan
owner_role: mobile-authoring
evidence: docs/evidence/UCAS/UCAS-WP15.md
---

# Outcome

Carry the same project, ChangeSet, Recipe and RenderPlan semantics to mobile
contract canaries, then expose them through a sandboxed product-safe Agent flow.

# Dependencies

UCAS-WP14 contracts for productization; compile canaries may run earlier under
the cross-platform plan without making a mobile product claim.

# Deliverables

- iOS Metal and Android Vulkan Core/Recipe/Catalog/RenderPlan/common-Skia canaries;
- app-sandbox project/asset ownership and adaptive approval UI;
- compact project-scoped local/remote API using stable IDs and CAS;
- OAuth/scoped authorization where remote, signed nonce-bound offline ChangeSet
  envelope, replay ledger and explicit import/replan/diff/approve flow;
- AssetId/content-digest ingest without raw external filesystem authority;
- lifecycle, network, background, thermal, memory and store policy handling;
- no downloaded native plugins on mobile.

# Verification and exit

- canary compile/semantic evidence is distinct from product qualification;
- stale, expired, replayed or foreign-project bundles reject without mutation;
- app termination during import yields old or new complete revision only;
- physical iOS/Android semantic/visual/performance/lifecycle evidence passes at G9;
- unsupported desktop-only capability fails closed and round-trips safely.

# Failure and rollback

Disable remote/offline writes while retaining local project/read/export paths;
do not fork mobile project semantics.
