---
id: G2-WP06
kind: work-package
status: proposed
gate: G2
owner_role: agent-authoring
evidence: docs/evidence/G2/G2-WP06.md
---

# Outcome

Give external Agents a precise, versioned and measurable authoring contract that
uses the same registry, validation, ChangeSets and diagnostics as the Studio.

# Dependencies

G2-WP03 transactional command/reconciliation path and G2-WP04 introspection and
measurement contracts.

# Read first

- accepted project-format and hierarchy decisions
- generated G2-WP02 registry contract
- G2-WP03/G2-WP04 APIs and diagnostics

# Allowed paths

CLI/MCP adapters, generated project-local Skill/examples, capability manifests,
diagnostic projections, conformance fixtures and evidence.

# Forbidden paths

Agent-only project authority; undocumented file rewriting; guessed coordinates,
times or capability names; direct renderer/backend control; examples that are
not compiled in CI; claims that prose can eliminate all Agent errors.

# Deliverables

- `describe`, `outline/tree`, `inspect`, `measure`, `time`, `capabilities`,
  `validate`, semantic `lint`, `diff`, typed `commit` and bounded
  `render-probe` operations;
- typed `GroupNodes`, `AddEffect`, `AlignNodes` and bounded recipe intents with
  machine-checkable topology postconditions;
- stable machine-readable diagnostics and remediation hints;
- project-local Agent Skill generated from registry version/digest;
- compiled recipes for composite Background, owner-local Glow, grouped
  Subscribe and bounded motion, plus examples for create/edit/group/animate/
  align/reorder/recover that validate in CI;
- one command envelope and semantic ChangeSet mapping shared by CLI/MCP;
- conflict-safe base revision and atomic commit workflow;
- source-location reporting and Last-Known-Good status discovery.

# Verification

- UI and Agent implementations of the same edit produce identical normalized
  ChangeSet meaning and accepted semantic digest;
- every shipped example validates against the exact registry digest;
- an Agent can locate a layer/group by stable ID, measure exact pixel/time data,
  commit, observe a deliberate error, read diagnostics and repair it;
- `measure` reports parent path, resulting Timeline row, Font digest, exact
  ranges and local/logical/ink/effect/world bounds from the accepted revision;
- static duplicate-Text/FX and ungrouped full-duration Background patterns are
  linted, while intent-aware commits reject topology violations;
- unsupported capabilities fail explicitly with no guessed fallback;
- CLI and MCP diagnostic codes match Studio Console.

# Evidence path

`docs/evidence/G2/G2-WP06.md`.

# Failure and rollback

Disable write operations whose validation/atomicity cannot be proven while
retaining read-only introspection. Preserve the accepted project and diagnostics.

# Exact handoff condition

The reference Agent authoring session completes without private APIs, direct UI
control or backend-specific knowledge and matches the UI semantic result.
