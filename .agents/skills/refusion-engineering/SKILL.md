---
name: refusion-engineering
description: Execute, verify, checkpoint, or hand off a bounded ReFusion engineering work package. Use when Codex changes ReFusion engine, Studio, platform adapters, contracts, plans, dependencies, tests, packaging, or evidence, or needs to resume the active Master Plan gate without rereading the repository.
---

# ReFusion engineering workflow

## Resolve authority and scope

1. Read repository `AGENTS.md` and `docs/status/CURRENT.md` completely.
2. Read the active stage plan and the assigned work-package section/file.
3. Read only linked ADRs, contracts, invariants, and module guidance.
4. Run `python3 tools/rfdev.py context` and verify the work belongs to the active
   gate and allowed paths.
5. Stop for a decision if the request creates a second truth, violates an
   invariant, changes product/legal authority, or expands beyond the active gate.

Do not load the foundation research draft unless a plan links to an unresolved
question in it.

## Execute a work package

1. Run the declared baseline checks.
2. Preserve unrelated user changes.
3. Implement the smallest complete vertical outcome, not disconnected types or
   inactive skeletons.
4. Route UI/agent changes through typed commands and accepted revisions.
5. Add structured diagnostics and explicit failure/rollback behavior.
6. Test every affected platform/profile the work package requires. Describe
   missing native evidence honestly.
7. Run docs and architecture checks plus the package-specific acceptance commands.

Read `references/work-package-contract.md` when creating a new work package,
checkpoint, evidence record, or handoff.

## Finish or hand off

- Update the evidence record with exact commands, artifacts, devices, results,
  failures, and dependency/toolchain fingerprints.
- Create a checkpoint containing the exact resume point and work not to repeat.
- Update `docs/status/CURRENT.md`; never report a vague completion percentage.
- Do not mark a gate passed unless every exit criterion and required native
  artifact/rollback proof exists and its owner accepts it.

