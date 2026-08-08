---
id: EXP-001A
kind: work-package
status: implementation-passed-owner-evaluation-pending
gate: pre-G2-experiment
owner_role: application-workspace
---

# Outcome

Extend EXP-001 with a real desktop Project Launcher and transactional workspace
creation flow. A user selects an engine-owned composition preset, resolution,
frame rate and duration, chooses or creates an empty folder through the native
system dialog, and enters the normal Studio only after the generated
`Project.rfx` compiles and reopens as accepted Revision 1.

# Authority boundary

- QML presents registry snapshots and submits `CreateProjectRequest`; it never
  constructs RFX source, IDs, canvas dimensions or filesystem trees.
- portable Core owns preset meaning, validation, stable ID generation and the
  initial immutable project snapshot;
- Application exposes the creation use case;
- the Qt desktop adapter owns the selected filesystem grant and writes a
  fail-closed workspace transaction with `Project.rfx` as the final commit
  marker;
- the normal project-open, RevisionAuthority and Runtime paths activate the
  generated project. No launcher-only project/render authority is allowed.

# Required vertical slice

1. Launcher on startup when no project path is supplied.
2. Native folder selection for an existing empty folder or a folder created by
   the platform dialog.
3. Reels 9:16, Portrait 4:5, YouTube 16:9 and Cinematic 2.39:1 presets.
4. Explicit 1080p/2K/4K dimensions and 24/25/30/50/60/90 fps choices.
5. A positive duration, defaulting to 30 seconds.
6. Valid empty Composition and empty Timeline/Canvas state.
7. Project-local lock, Agent instructions, Skill and runtime agent-context.
8. Create, compile, reopen and activate in one process.
9. Collision, non-empty folder, invalid request and partial-write failure tests.

# Claim boundary

Preset selection defines composition geometry/time only. It does not qualify
4K/90 fps video decode, export or delivery quality. Physical runtime evidence
remains macOS-only; other declared platforms remain not-run.

# Verification

```bash
python3 tools/rfdev.py docs-doctor
python3 tools/rfdev.py architecture-check
cmake --workflow --preset macos-core
cmake --workflow --preset macos-core-sanitized
cmake --workflow --preset macos-visual
```

# Exact handoff

Stop with the launcher visible on a no-argument start and a generated project
that can be edited by an external agent through Revision 2 without any bundled
example/repository-relative assumption.

# Implementation result

The vertical slice is implemented. Core owns the four presets, exact canvas
dimensions, permitted frame rates, validated empty Revision 1 and engine-owned
IDs. The desktop workspace adapter creates the versioned project-local Agent
instructions through a rollback-safe staging transaction, commits
`Project.rfx` last, reopens it through the normal compiler/authority path and
then writes the runtime Agent context. No QML component generates source,
project IDs, time, folders or revisions.

Automated implementation evidence is recorded in
`docs/evidence/experiments/EXP-001A.md`. Owner evaluation remains required for
the native folder-dialog experience and the subsequent external-agent Revision
2 edit.
