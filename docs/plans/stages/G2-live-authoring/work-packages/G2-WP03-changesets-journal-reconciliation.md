---
id: G2-WP03
kind: work-package
status: proposed
gate: G2
owner_role: revision-authority
evidence: docs/evidence/G2/G2-WP03.md
---

# Outcome

Make UI and external file/CLI/MCP authoring transactional, replayable and
recoverable through the same RevisionAuthority without partial publication.

# Dependencies

G2-WP02 schema/registry and canonical digest contract.

# Read first

- `docs/architecture/INVARIANTS.md`
- accepted project-format decision
- G2-WP02 evidence and generated registry contract

# Allowed paths

Portable Core/Application command and authority services, project adapters,
journal/reconciliation services, CLI, diagnostics and focused tests.

# Forbidden paths

UI/file-watcher authority; mutable project ownership in Studio; direct document
replacement outside CAS; partial multi-file activation; silent conflict merge;
filesystem-event ordering as semantic ordering.

# Deliverables

- typed atomic ChangeSet with base revision and source/audit identity;
- typed `GroupNodes`, `ReparentNodes`, `AddEffect` and measured `AlignNodes`
  intents whose normalized postconditions are shared by UI, CLI and MCP;
- topology postconditions: an effect edit cannot create/reparent Layers and a
  one-shot alignment cannot introduce a hidden persistent dependency;
- CAS validate/apply/publish path and deterministic normalized semantic diff;
- atomic save plus journal/replay and crash recovery;
- watcher-as-hint reconciliation for partial/coalesced/duplicate events;
- Last-Known-Good and last compatible EvaluationStamp retention;
- stable diagnostics for syntax, schema, conflict, stale, partial and recovery;
- intent/capability diagnostics that reject unsupported animated FX without a
  duplicate-Layer approximation or accepted-revision change;
- Undo/Redo foundation expressed as accepted/replayed semantic changes.

# Verification

- 10,000 mixed UI/file candidates produce no mixed revision or LKG corruption;
- interrupted writes/restarts yield the previous or next complete transaction;
- duplicate/coalesced/out-of-order events are idempotent;
- UI and source edits with equal meaning produce equal digest;
- UI/Agent `AddEffect` preserves Layer/Group/root counts and equivalent
  `AlignNodes` produces equal normalized ChangeSet meaning;
- stale-base and malformed candidates never publish;
- fault corpus and sanitized desktop Core lanes pass.

# Evidence path

`docs/evidence/G2/G2-WP03.md`.

# Failure and rollback

Disable live reconciliation and retain explicit validated import if atomicity or
recovery cannot be proven. Never repair by accepting a partial project.

# Exact handoff condition

WP05/WP06 can observe immutable candidate/accepted/rejected records and submit
ChangeSets without owning project state.
