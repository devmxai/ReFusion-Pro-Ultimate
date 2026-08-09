---
id: UCAS-WP01
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G2-admission
owning_gate: G2
depends_on: UCAS-WP00
decision_dependencies: G2-admission-decisions
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,G2-WP01
evidence_owner: G2-WP01
owner_role: creative-authoring-architecture
evidence: docs/evidence/G2/G2-WP01.md
---

# Outcome

Accept or explicitly defer every architecture decision required before the
unified authoring spine can be implemented.

# Dependencies

UCAS-WP00 and formal G2 decision admission under MP-001.

# Deliverables

- ADR for materialized visual truth and non-rendering Receipt lifecycle;
- ADR for federated registries, common descriptor header and catalog digest;
- ADR for IntentEnvelope, atomic ChangeSet, PlanReceipt, Commit and durable
  idempotency;
- ADR for bounded Recipe IR, pure compiler and stable entity allocation;
- ADR for package versions, locks, migrations and explicit upgrade;
- ADR for exact-time animation curves and temporal/spatial separation;
- ADR for package trust, permissions, budgets, licensing and extension tiers;
- authority map, second-truth threat model and stage ownership matrix;
- explicit dispositions for Text selectors, Spring, Glass/backdrop, generated
  assets, public declarative graphs and native extensions.

# Verification and exit

- every blocking question is accepted or marked deferred with a blocking owner;
- no accepted decision contradicts MP-001, architecture invariants or the
  cross-platform plan;
- deferred decisions prevent activation of dependent packages;
- rejected alternatives and rollback consequences are recorded;
- architecture, product, security and cross-platform owners approve the set.

# Failure and rollback

Do not start code. Retain the research record and revise the decision package.
