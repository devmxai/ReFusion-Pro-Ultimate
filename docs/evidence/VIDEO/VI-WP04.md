---
id: EVID-VIDEO-VI-WP04
kind: exact-video-playback-scheduler-evidence
plan: PLAN-VIDEO-VS-001
work_package: VI-WP04
status: macos-implementation-checkpoint-qualification-pending
source_branch: feature/shared-video-import-v1
last_verified: 2026-08-10
---

# VI-WP04 — Exact scheduler and RenderPlan Video checkpoint

## Honest claim

The macOS product path now maps Core ProjectTime to exact Stream PTS, lowers a
portable `DrawVideoFrame`, selects a bounded native surface and composites it
through SkiaCommon. It is not yet a cross-Desktop or completed VI-WP04 claim.

## Causal corrections

- one persistent hardware playback session preserves forward B-frame
  dependencies and never seeks back within the tested GOP sequence;
- distinct published-low and decoder-high watermarks prevent a short-window
  publish/refill loop;
- an anchored presentation deadline grid prevents permanent phase drift after
  scheduler jitter;
- project time remains owned by Core `ProjectClock`; media and UI only consume
  immutable snapshots.

## Verification

```text
refusion.viewport_presentation: passed
refusion.apple_imported_video_playback: passed
refusion.skia_decoded_video_surface: passed
macos-visual aggregate: 64/64 passed
architecture-check: 135 source files, 0 problems, 0 boundary debt
docs-doctor: 152 documents, 0 problems
```

Physical trace on `/Users/mx/Desktop/wv/Project.rfx` confirmed one project frame
per 60 Hz interval after anchoring. The 30 fps source selects one new source
frame every two project frames. Temporary low-level presentation traces were
removed from the normal product path after diagnosis.

## Remaining exit evidence

- formal drop/repeat/deadline counters and performance budgets;
- full product seek/pause/resume physical receipt;
- matching MSVC and Windows native-adapter execution;
- same-commit cross-Desktop qualification.
