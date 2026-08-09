---
id: UCAS-WP03
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G2
owning_gate: G2
depends_on: UCAS-WP01,UCAS-WP02
decision_dependencies: atomic-authoring-transaction-ADR
cross_plan_dependencies: MP-001,G2-WP03
evidence_owner: G2-WP03
owner_role: revision-authority
evidence: docs/evidence/G2/G2-WP03.md
---

# Outcome

Replace command-family overload growth with one versioned, atomic and
recoverable authoring transaction used by every client.

# Dependencies

UCAS-WP01 transaction decisions, UCAS-WP02 identity contracts and G2-WP03.

# Deliverables

- versioned IntentEnvelope and typed operation union;
- multi-operation ChangeSet with stable temporary references, dependencies,
  preconditions, postconditions, affected scope and resource limits;
- deterministic normalization and ChangeSet digest;
- `plan_changeset()` returning a server-issued PlanReceipt bound to project,
  base revision/snapshot, actor/scopes, catalog lock, profile and expiry;
- `commit_plan()` with revalidation, CAS, Runtime preparation and one atomic
  AcceptedRevisionBundle publication;
- durable idempotency ledger and `operation_status()` for ambiguous retries;
- journal/replay/Undo foundation and matching typed diagnostics;
- adapters from current typed commands during migration.

# Verification and exit

- one failed operation leaves the entire revision unchanged;
- equivalent UI/CLI/MCP intents produce equal normalized ChangeSet meaning;
- stale, expired, foreign-actor or modified plans fail without silent rebase;
- same idempotency key and digest returns the original result after restart;
- 10,000 mixed candidates cause zero mixed revision and zero LKG loss;
- fault injection before/after persistence and publication yields old or new
  complete state only.

# Failure and rollback

Disable external writes and retain read-only planning. Keep current typed
commands as adapters until dual-path evidence permits removal.
