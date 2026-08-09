---
id: EVID-XPF-WP05C-MACOS-RECONCILIATION-2026-08-09
kind: evidence-review
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP05,XPF-WP08
tested_source_commit: 47948b29525ce8bf42aaab5e021d2bf32e6e6338
windows_candidate_commit: 38536e54fb748d6fa57cc2c54086f5a3672bd0f6
integration_merge_commit: 78879f6
platform: macos-arm64-metal
state: source-compiled-physical-high-precision-pass-bounded
date: 2026-08-09
---

# Windows Canvas candidate — macOS reconciliation

## Outcome

The Windows-owned `fix/windows-canvas-quality` candidate was integrated on the
isolated `integration/windows-canvas-quality-macos` branch and qualified on the
official Apple-M1 macOS host before promotion. `main` was not used as an
experimentation branch.

The review confirmed that full-resolution Composition rasterization, staged
Fit reduction, Mitchell presentation sampling, final dither and presentation
profile selection remain shared contracts. Metal and D3D12 contain native
target/profile mechanics only; the Windows correction did not add a second
project, text, color, animation or FX meaning.

Two integration defects were corrected before qualification:

- the candidate reused ADR numbers already assigned on `main`; the machine
  cache and high-precision carrier decisions are now ADR-0016 and ADR-0017;
- the policy tamper test still replaced the retired local dependency-record
  constant instead of the cache-aware resolver. The test now injects its
  altered record through `skia_dependency_record_path()` and proves that a
  changed Skia `DEPS` digest is rejected.

## Dependency and build evidence

Finder metadata inside transitive dependency worktrees was quarantined without
changing a source revision. The repository verifier then accepted all 46
pinned transitive repositories. No dependency was downloaded again.

The local macOS Skia profile was rebuilt from the already materialized official
sources and verified as:

```text
profile:          macos-arm64-metal
Skia revision:    294d31e0b1aa295d585836ab41bd2fba170e0c5d
bundle SHA-256:   7cb0fd473095136edd49566e13e0f13031b7576a3dbdea2886b17b23f2c056ce
dependency count: 46
```

The development machine cache was not published on this host because doing so
would duplicate the 9.2 GiB source materialization while only 29 GiB remained.
Local verified materialization is still the first admitted CMake source. This
keeps ADR-0016 proposed and does not weaken a build or runtime result.

## Automated qualification

```text
macos-core:       31/31 passed
macos-graphics:   40/40 passed
macos-visual:     52/52 passed
architecture:     116 source files, 0 problems, 0 boundary debt
docs-doctor:      112 documents, 0 problems
git diff check:   passed
```

The Visual suite includes the common RenderPlan/compositor, Metal presenter,
full-resolution mapping, both presentation profiles, packaged text, visual
regression, decoded-video GPU-surface binding, live reload and dependency
tamper policy.

## Physical macOS run

The built Studio opened `/Users/mx/Desktop/mm1/Project.rfx`, a real 1080x1920
project containing animated gradients, blur and animated text. Application
telemetry reported:

```text
GPU LIFECYCLE: READY
Presentation: RGBA16F / LINEAR sRGB
Canvas: FIT
Device-loss rejections: 0
```

Space transport ran the project for ten seconds and paused it again. The native
window was resized from 1440x932 to 1200x820 and then 1500x950. The process
remained responsive and did not fall back or crash.

A separate 240-frame physical presenter stress matched the returned Windows
stress depth:

```json
{"requested_frames":240,"present_submissions":240,"cpu_pixel_maps":0,"cpu_pixel_uploads":0,"gpu_readbacks":0,"unattributed_gpu_copies":0}
```

## Decision and claim boundary

The candidate is eligible for reviewed promotion to `main`. This evidence
qualifies the affected macOS Metal source/runtime route and reconciles the
Windows correction; it does not close `PLAN-XPLAT-FIX-001`.

The same-commit calibrated Metal/D3D12 capture comparison, full performance
qualification, Android official-NDK receipt, Windows Media Foundation Video
import/decode and production packaging remain open. ADR-0017 remains proposed
until its complete decision-level visual qualification is reconciled; no
threshold or capability state was promoted by inference.
