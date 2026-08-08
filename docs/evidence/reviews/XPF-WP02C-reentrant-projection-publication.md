---
id: EVID-XPF-WP02C-REENTRANT-PROJECTION-2026-08-09
kind: implementation-evidence
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP02
scope: atomic-engine-commit-and-reentrant-ui-projection-publication
status: passed-local-regression-windows-not-run
date: 2026-08-09
---

# XPF-WP02C reentrant projection-publication receipt

## Defect

An Agent-authored valid RFX5 Revision containing one grouped animated gradient
background entered the filesystem watcher and passed parsing, validation and
Runtime preparation. Application then held its non-recursive admission mutex
while calling the prepared Runtime publication. Studio synchronously reset the
Timeline model; QML bindings queried `StudioBridge::selectedNodeId()`, which
called `ProjectCommandService::active_snapshot()` and attempted to acquire that
same mutex on the same main thread. Studio therefore self-deadlocked before the
accepted journal and diagnostics could advance.

The project was not corrupt and the visual workload was not excessive. The
5,141-byte candidate independently passed CLI `validate` and `lint` with three
Layers, one Group, two Blur instances and ten animation channels.

## Correction

`PreparedProjectRevision` now has two explicit no-fail phases:

1. `commit_engine_state()` runs under Application admission exclusion and may
   perform bounded Core-adjacent Runtime pointer/state swaps only;
2. `publish_observer_projections()` runs synchronously after Application
   releases the admission mutex and may publish immutable Canvas, Timeline,
   Inspector and diagnostic projections.

The Visual Runtime commits the shared RenderProgram and accepted Composition
before unlock. Only afterward does it emit viewport and Timeline model
notifications. No recursive mutex, deferred acceptance, clamp, retry loop or
second authority was introduced.

## Regression proof

The Application Host fixture now deliberately re-enters `active_snapshot()`
from synchronous projection publication, matching the QML failure pattern. It
requires the observed Revision to be the newly committed Revision and would
deadlock under the former ordering. The real filesystem live-reload fixture is
also bounded by a test timeout.

```text
refusion.application_host: passed
refusion.project_live_reload: passed
macos-visual: 49/49 passed
macos-core-sanitized: 28/28 passed
architecture/repository policy: passed
```

The rebuilt Studio reopened `/Users/mx/Desktop/mm1/Project.rfx` at Revision 2
and wrote `.refusion/Journal/accepted-r2.rfx` plus an accepted Revision-2 open
diagnostic. The previously frozen process had stopped before either receipt.
The replacement Studio process remains in its normal Qt event loop rather than
waiting on `admission_mutex_`.

## Claim boundary

This closes the reproduced macOS reentrancy defect and hardens the portable
Application publication contract. Windows UI execution remains `not-run`, so
this receipt does not replace the required MSVC/D3D12 physical evidence.
