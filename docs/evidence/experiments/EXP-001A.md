---
id: EV-EXP-001A
kind: evidence-record
gate: pre-G2-experiment
work_package: EXP-001A
status: implementation-passed-owner-evaluation-pending
source_commit: working-tree-pending-owner-evaluation
date: 2026-08-07
---

# Project Launcher and workspace evidence

## Delivered vertical slice

- With no project argument Studio opens a native Qt Project Launcher, not a
  bundled example or HTML surface.
- The UI displays four composition presets, three exact resolutions per preset,
  six admitted integer frame rates, a project name and a positive duration.
  These values come from the portable Core registry; QML does not derive canvas
  dimensions or construct project source.
- Core creates engine-owned Project and Composition IDs and validates an empty
  Revision 1. Empty means zero Layers, not an absent or invalid Composition.
- The user selects or creates the exact project folder through Qt's native
  folder dialog. The adapter rejects non-local, missing and non-empty targets.
- Workspace creation uses a hidden staging directory and `QSaveFile`; promoted
  entries are rolled back on failure and `Project.rfx` is promoted last as the
  root commit marker.
- The real workspace contains `Project.rfx`, `refusion.lock`, `Media/Originals`,
  `AGENTS.md` and the versioned project-local `$refusion-project-authoring`
  Skill. Opening it through the normal Studio path creates `.refusion` runtime
  state and an absolute `agent-context.json` containing the active CLI,
  diagnostics and authoring paths.
- Creation success is not a parallel project model: generated RFX is compiled,
  reopened and activated through the existing Application/Core authority,
  Runtime, Timeline and Skia Canvas paths.

## Verification

- `cmake --workflow --preset macos-core`: 9/9 tests passed.
- `cmake --workflow --preset macos-core-sanitized`: 9/9 tests passed under
  ASan and UBSan.
- `cmake --workflow --preset macos-visual`: 23/23 tests passed on macOS,
  including Metal/Skia and all project workspace paths.
- `refusion.project_creation`: registry selection, exact geometry/time, empty
  Revision 1, stable engine-owned IDs and invalid-input rejection passed.
- `refusion.project_workspace_creator`: compile/reopen, required project files,
  empty Composition, collision rejection and preservation of pre-existing user
  data passed.
- `refusion.project_launcher`: Core registry exposure, 2K Reels at 90 fps,
  create/open signals and unsupported-preset rejection passed.
- `refusion.project_live_reload`: absolute runtime agent-context plus accepted
  and rejected external revisions passed.

## Cross-platform and claim boundary

Project creation semantics and RFX compilation are portable C++20. The native
folder dialog and filesystem transaction are desktop Qt adapters. Only macOS
has physical runtime evidence; Windows, iOS and Android remain `not-run`.
Selecting 4K or 90 fps establishes Composition geometry/time only and does not
qualify hardware decode, realtime playback, export or delivery at those modes.

## Owner evaluation pending

The implementation gate is green. The remaining EXP-001A evidence is the
owner's real launcher interaction and the first external-agent Revision 2 edit
inside the newly created workspace. That evaluation decides whether this
authoring experiment is accurate and efficient enough to advance into the G2
format decision; it does not adopt RFX as the shipping format by itself.
