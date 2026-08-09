---
id: UCAS-WP04
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G2-bounded,G5-product
owning_gate: G2
depends_on: UCAS-WP02,UCAS-WP03
decision_dependencies: agent-surface-MCP-security-ADR
cross_plan_dependencies: MP-001,G2-WP04,G2-WP06
evidence_owner: G2-WP06
owner_role: agent-authoring
evidence: docs/evidence/G2/G2-WP06.md
---

# Outcome

Make UI, file candidates, CLI and local MCP thin clients of the same
AuthoringService, while giving Agents a compact digital eye and executable
project-local guidance.

# Dependencies

UCAS-WP02, UCAS-WP03 and G2 measurement/evaluation contracts.

# Deliverables

- bounded `project_context`, paginated `outline`, filtered `inspect`, instant or
  swept `measure`, indexed `catalog_search`, `plan_changeset`, `commit_plan`,
  `operation_status`, `diagnostics` and bounded `render_probe`;
- stable-ID mutation and typed ambiguity diagnostics for display-name search;
- local MCP over authenticated stdio/loopback with explicit revision-bound
  handles and project-scoped permissions;
- file candidate path that uses lossless semantic normalization when possible,
  otherwise a privileged whole-project candidate through the same admission;
- descriptor-driven Studio controls with no UI construction/default semantics;
- generated project Skill and examples bound to schema/catalog digests;
- security limits, path confinement, redacted audit and prompt-injection rules.

# Verification and exit

- equal edits through UI/CLI/MCP produce equal ChangeSet, project and evaluation
  digests and equal affected stable IDs;
- file parity is claimed only when normalization is lossless;
- 500 Layers/5,000 keys do not force whole-project or whole-catalog echoes;
- retries do not duplicate mutation and MCP failure cannot corrupt a project;
- all generated examples execute in CI against their exact digests;
- project content is untrusted data and cannot change tool/Skill instructions.

# Failure and rollback

Withdraw write tools and keep read-only context/inspection. Never restore
UI-specific semantic constructors or Agent direct whole-file edits for supported
local operations.
