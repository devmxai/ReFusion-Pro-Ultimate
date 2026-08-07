---
id: G0-WP04A
kind: work-package
status: active
gate: G0
owner_role: foundation-architecture
evidence: docs/evidence/G0/G0-WP04A.md
last_verified: 2026-08-07
---

# Outcome

Remove concrete project/revision authority from Qt and CLI adapters, establish
one Application Host command boundary, correct result semantics, and enforce
the boundary with negative tests.

# Dependencies

G0 command baseline and the 2026-08-07 independent architecture audit.

# Read first

`AGENTS.md`, `docs/architecture/INVARIANTS.md`, `docs/architecture/BOUNDARIES.md`,
ADR-0001, ADR-0002, and the current checkpoint.

# Allowed paths

`src/core`, `src/application`, `apps/cli`, `apps/studio`, `tests/unit`,
`tests/tools`, CMake, Repo OS tools, G0 plan/status/evidence/checkpoints.

# Forbidden paths

Renderer/media feature work, project schemas, public plugin ABI, and any Qt,
Skia, OS, or native GPU type in Core project contracts.

# Deliverables

- Hidden concrete Application Host owning the mutable Core authority.
- UI/CLI clients depending on `ProjectCommandService`, never `ProjectAuthority`.
- Non-blocking success diagnostics and blocking rejection diagnostics.
- Source/link policy checks and negative policy tests.

# Verification

Core/Studio/graphics workflows, Repo OS policy test, Docs Doctor, Architecture
Check, and whitespace validation.

# Failure and rollback

Any client-owned concrete authority or direct Studio/CLI-to-Core link fails the
architecture gate. Last-Known-Good semantics must remain unchanged.

# Exact handoff condition

All local checks are green and immutable evidence identifies the remaining
external Windows gate.
