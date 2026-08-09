---
id: UCAS-WP02
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G2-foundation,G3-breadth,G10-exposure
owning_gate: G2
depends_on: UCAS-WP01
decision_dependencies: federated-catalog-version-migration-ADRs
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,G2-WP02
evidence_owner: G2-WP02
owner_role: descriptor-catalog
evidence: docs/evidence/G2/G2-WP02.md
---

# Outcome

Provide one deterministic searchable capability view over independently owned
domain registries, exact package locks and derived qualification state.

# Dependencies

Accepted UCAS-WP01 catalog/version decisions and the G2 schema/registry spine.

# Deliverables

- common descriptor identity/version/digest/compatibility header;
- Property, Effect/Mask, Paint/Geometry, Curve, Generator, Recipe/Style, Asset
  Type and Installed Extension registries;
- federated query/search/pagination API without a God Registry;
- deterministic combined catalog digest and exact dependency lock;
- explicit descriptor/package/contribution-state migration records;
- generated C++ validators, JSON schemas, UI metadata, CLI/MCP schemas and Skill
  references from the accepted source definitions;
- qualification projection with `defined`, `canonical`, `compiled`,
  `physically_run`, `semantically_matched`, `visual_tolerance_passed`,
  `performance_qualified` and `qualified` states.

# Verification and exit

- catalog bytes/digests match under AppleClang and MSVC;
- duplicate IDs, dependency cycles and incompatible ranges reject deterministically;
- adding a descriptor requires no UI, CLI or backend-specific switch;
- unknown packages/descriptors preserve unresolved state safely;
- opening a project never resolves `latest` or upgrades a package;
- generated artifacts fail CI on drift and queries meet accepted bounds.

# Failure and rollback

Keep compatibility façades over current registries. Remove an unqualified
package from the active lock without destructive project migration.
