---
id: UCAS-WP16
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G10
owning_gate: G10
depends_on: UCAS-WP14
decision_dependencies: public-SDK-signing-conformance-host-ADRs
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,G10
evidence_owner: future-G10-stage-plan
owner_role: extension-platform
evidence: docs/evidence/UCAS/UCAS-WP16.md
---

# Outcome

Expose the already-qualified internal creative model as a versioned public SDK
without creating a second registry, command, renderer or plugin engine.

# Dependencies

Stable internal contracts after UCAS-WP14/G5 and explicit G10 activation.

# Deliverables

1. public read-only descriptor/catalog API;
2. signed declarative Recipe/Style packages;
3. conformance-qualified declarative graphs;
4. sandboxed non-realtime workers;
5. isolated out-of-process native host with stable IPC/C ABI only after v1;
6. certified GPU/audio extensions and marketplace policies only after signing,
   revocation, compatibility and crash/resource containment exist;
7. conformance kit, migrations, unresolved-state and mobile restrictions.

# Verification and exit

- third-party packages use the same descriptors, ChangeSet, admission and
  RenderPlan path as built-ins;
- a plugin cannot publish project state or access native renderer authority;
- crash, hang, resource abuse and revocation do not corrupt Studio or projects;
- unsupported/revoked contributions round-trip unresolved and fail closed;
- cross-platform extension qualification and security evidence pass for each
  advertised tier.

# Failure and rollback

Quarantine/revoke the package or disable the isolated host while retaining
pinned materialized project state and the last accepted package version.
