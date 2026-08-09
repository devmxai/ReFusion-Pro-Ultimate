# ReFusion — Windows Build, Physical Test, and Diagnostic Handoff

This is the single entry point for an Agent working on the physical Windows
machine. It is an execution runbook and diagnostic handoff, not a second source
of project status or architecture truth.

Canonical authority remains:

- [`AGENTS.md`](AGENTS.md) for repository rules;
- [`docs/status/CURRENT.md`](docs/status/CURRENT.md) for the exact resume point;
- [`Fix Cross-Platform Architecture`](docs/plans/FIX_CROSS_PLATFORM_ARCHITECTURE.md)
  for qualification policy and exit criteria;
- [`docs/architecture/INVARIANTS.md`](docs/architecture/INVARIANTS.md) for
  non-negotiable boundaries.
- [`Cross-platform Git workflow`](docs/architecture/CROSS_PLATFORM_POLICY.md#canonical-two-host-git-workflow)
  for branch ownership, handoff, integration and evidence invalidation.

## Mission for the Windows Agent

Build and diagnose the exact checked-out ReFusion commit on a real Windows x64
machine. Prove what actually works under MSVC, pinned Windows Skia, hardware
D3D12/DXGI and Qt Studio. Never infer runtime success from source presence or a
compile-only result.

The current Windows scope is Canvas/Shape/Text/Group/current FX and the common
RenderPlan/Skia compositor. Windows Media Foundation video decode and complete
Video Layer import/playback are not implemented or qualified by this run.

## Mandatory Agent protocol

Before running or editing anything:

1. Read this file completely.
2. Read [`AGENTS.md`](AGENTS.md).
3. Run `python tools/rfdev.py context` and read the returned active documents.
4. Record the exact source commit with `git rev-parse HEAD`.
5. Confirm `git status --short` is empty.
6. Do not change branches or pull another commit during one test run.
7. Stop at the first failed lower lane. Do not launch Studio to hide a Core or
   Graphics failure.
8. Use an `evidence/windows-*` branch only for the bound run and its report.
   Start later product fixes from the latest `origin/main` on `fix/windows-*`
   or `fix/shared-*`; never continue product development on old evidence.

Do not:

- copy Skia, Qt, fonts, build outputs or caches from macOS or another checkout;
- use WARP, a software GPU, CPU project-pixel fallback or Qt rendering for the
  project Canvas;
- add Windows-only project, text, animation, color, mask, blend or FX meaning;
- disable tests, strict warnings, capability rejection or architecture checks;
- claim Video import/playback from a successful Canvas test;
- edit the static instructions in this file when reporting a result. Edit only
  the marked **Windows Agent Report** block at the end.

## Required physical host

- Windows 11 x64 on a physical machine, not a compatibility VM;
- a named hardware D3D12 GPU with the current vendor driver;
- Visual Studio 2022 or Build Tools with the x64 C++ workload and a Windows SDK;
- Git, PowerShell 7 (`pwsh`), CMake 3.30+, Ninja and Python 3 available in
  `PATH`;
- network access only for the repository-owned fresh dependency bootstrap;
- Qt 6.11.1 MSVC x64 available for the `Visual` lane. This is an engineering
  build requirement; the deferred Qt Commercial release-entitlement gate is
  not part of this diagnostic run.

Before starting, open PowerShell 7 and verify:

```powershell
git --version
pwsh --version
cmake --version
ninja --version
python --version
```

If Qt is not discovered automatically, point CMake to the Qt 6.11.1 MSVC kit
for the current PowerShell session:

```powershell
$env:CMAKE_PREFIX_PATH = "C:\Qt\6.11.1\msvc2022_64"
```

Use the actual installed kit path. Do not select a MinGW Qt kit for the MSVC
lane.

### Optional development cache

For ordinary UI/product development, a previously verified machine cache can
avoid downloading and rebuilding Skia/Qt/fonts for every clone. Its default
Windows location is `%USERPROFILE%\.rfx\dc1`; the short path is intentional
because Dawn contains filenames near the legacy Windows path limit.

Publish once from the already verified checkout and installed Qt kit:

```powershell
python tools/bootstrap.py machine-cache publish-skia `
  --profile windows-x64-d3d12 `
  --from-checkout C:\path\to\verified\ReFusion
python tools/bootstrap.py machine-cache publish-qt `
  --source C:\path\to\Qt\6.11.1\msvc2022_64
python tools/bootstrap.py machine-cache status
```

Every later clone can use the verified entries directly:

```powershell
pwsh -NoProfile -File .\tools\windows\Invoke-ReFusionWindowsBringup.ps1 `
  -Lane Visual `
  -UseMachineCache `
  -ReceiptPath out/evidence/windows-visual-machine-cache.json
```

This mode re-verifies the exact cache identities and remains non-qualifying by
design. Do not combine `-UseMachineCache` with `-FreshDependencies`. The
qualification phases below continue to use checkout-local official
materialization. Git, CMake, MSVC, Ninja and a working Python interpreter are
machine prerequisites installed once, not payloads copied into this cache. Use
`-PythonExe C:\path\to\python.exe` if the Windows Store alias shadows Python.

## Clone and isolate the evidence branch

```powershell
git clone https://github.com/devmxai/ReFusion-Pro-Ultimate.git
Set-Location .\ReFusion-Pro-Ultimate
git fetch --all --prune
git switch main
git pull --ff-only
$SourceCommit = (git rev-parse HEAD).Trim()
$Machine = $env:COMPUTERNAME.ToLowerInvariant()
$Date = Get-Date -Format yyyyMMdd
git switch -c "evidence/windows-desktop-v1-$Date-$Machine"
git status --short
python tools/rfdev.py context
```

The final `git status --short` must be empty before invoking the bring-up
script. Record `$SourceCommit` in the report block below.

## Phase 1 — Generate and review the Windows Skia lock

The Windows dependency closure cannot be fabricated on macOS. The first clean,
non-qualifying run generates it and proves the Core/Graphics compile entrance:

```powershell
pwsh -NoProfile -File .\tools\windows\Invoke-ReFusionWindowsBringup.ps1 `
  -Lane Graphics `
  -FreshDependencies `
  -CompileOnly `
  -ReceiptPath out/evidence/windows-graphics-compile-only.json
```

Expected generated file:

```text
deps/locks/skia-transitive-windows-x64.lock.json
```

After the command finishes:

```powershell
Get-Content .\out\evidence\windows-graphics-compile-only.json
Get-Content .\deps\locks\skia-transitive-windows-x64.lock.json
git status --short
```

If the lane fails, stop, complete the report block below, commit only the
diagnostic handoff, and push the evidence branch. Do not invent a lock or skip
to Studio.

If it passes, review that the lock contains repository-bootstrapped official
Skia materialization only, then commit it:

```powershell
git add .\deps\locks\skia-transitive-windows-x64.lock.json
git commit -m "deps: lock official Windows Skia materialization"
git push -u origin HEAD
git status --short
```

The repository must be clean again before Phase 2. The new lock commit becomes
the exact source commit for the physical qualification run; record it again
with `git rev-parse HEAD`.

## Phase 2 — Physical D3D12 and Visual qualification

Run the complete ordered Core -> Graphics -> Visual route from the clean lock
commit. Do not use `-CompileOnly`:

```powershell
pwsh -NoProfile -File .\tools\windows\Invoke-ReFusionWindowsBringup.ps1 `
  -Lane Visual `
  -ReceiptPath out/evidence/windows-visual-physical.json
```

This route must use a real non-WARP GPU. It builds and tests the common semantic
engine, the shared Skia compositor, the D3D12 binding, DXGI presentation and the
Windows Qt Studio. It also creates the declared D3D12 capture/comparison and
machine receipts when the qualifying steps pass.

Expected diagnostic artifacts include:

```text
out/evidence/windows-visual-physical.json
out/evidence/xplat-visual-v1-windows-d3d12-640x360.ppm
out/evidence/xplat-visual-v1-metal-vs-d3d12.json
out/evidence/windows-d3d12-visual-qualification.json
```

The main receipt is authoritative for the run result. A screenshot is useful
human evidence, but it is not a substitute for the receipt, test inventory,
semantic digests or calibrated comparison.

## Phase 3 — Launch the built Studio for manual observation

Only do this after the `Visual` lane builds successfully:

```powershell
$Studio = Get-ChildItem .\out\build\windows-visual `
  -Filter refusion-studio.exe -Recurse | Select-Object -First 1
if ($null -eq $Studio) { throw "refusion-studio.exe was not produced" }
& $Studio.FullName
```

Observe and report:

1. the Project Launcher opens without a bundled/mock project;
2. a new Reels project can be created and reopened;
3. the Canvas uses the D3D12/Skia route and presents continuously;
4. Timeline Play/Pause and playhead behavior match the accepted project time;
5. Shape, Text, Group, gradient background, Blur, Drop Shadow and Glow render;
6. Arabic/Latin packaged-font text renders without system-font substitution;
7. resizing, minimize/restore and occlusion recover without a silent fallback;
8. invalid project edits retain Last-Known-Good and produce diagnostics.

Do not report Video import or decoded-video playback as passed; that Windows
Media Foundation product path remains outside this test.

## Failure classification and repair boundary

When something fails, preserve the first causal failure and classify it before
editing code:

| Classification | Correct repair location |
|---|---|
| Host/toolchain missing | Windows setup or documented prerequisite |
| Official dependency/lock/profile | `deps/`, bootstrap/profile or CMake supply-chain code |
| Portable compile/semantic/digest failure | Core, Application, RuntimeRender, Registry, persistence or SkiaCommon; rerun macOS and Windows |
| D3D12/DXGI device, target, sync or present failure | thin Windows native adapter only |
| Qt shell/control failure | Studio/Qt adapter only; never project Canvas semantics |
| Unsupported capability | retain Last-Known-Good and report fail-closed; do not approximate it |
| Windows video decode/import | open Media Foundation/G4 work; do not treat as a Canvas regression |

For any code change:

1. make the smallest causal correction;
2. do not duplicate semantic rendering in Windows sources;
3. run `python tools/rfdev.py docs-doctor` and
   `python tools/rfdev.py architecture-check`;
4. rerun the failed lane and every affected lower lane;
5. update the report block with exact evidence;
6. commit and push the evidence branch—never push an unverified workaround to
   `main`.

## Returning the diagnosis through GitHub

After the run, edit only the report block below. Keep large logs and generated
build output under `out/`; record their paths, relevant hashes and the first
meaningful error here rather than pasting thousands of log lines.

Then publish the report:

```powershell
git add .\README_FOR_WINDOWS.md
git commit -m "evidence: report Windows physical bring-up"
git push -u origin HEAD
git status --short
```

If a reviewed Windows lock or an intentional source fix is also present, list
every changed file and why in the report. The macOS Agent will fetch this branch,
read this block and reconcile the receipts against the bound macOS evidence.
Shared UI or engine development is not appended here: after reconciliation it
uses a new feature/fix branch from the latest `origin/main` under the canonical
two-host Git workflow.

---

## Windows Agent Report

Replace only the content between the markers. Use factual values; write
`not-run`, `not-produced` or `unknown` instead of guessing.

<!-- WINDOWS_AGENT_REPORT:BEGIN -->

### Latest report

| Field | Value |
|---|---|
| Report state | `Windows passed; cross-platform physical pixel comparison requires a same-commit macOS reference` |
| UTC timestamp | `2026-08-09T05:26:40Z` |
| Evidence branch | `evidence/windows-desktop-v1-20260808-desktop-91mgbu8` |
| Tested source commit | `1e2dc898fdb50ce7871901159f4e51391350a337` |
| Windows edition/build | `Windows 11 Pro 23H2, build 22631.6199 (NT kernel string 10.0.22631.0)` |
| CPU/architecture | `13th Gen Intel Core i5-1335U / x64` |
| GPU | `Intel(R) UHD Graphics` |
| GPU driver | `32.0.101.5542` |
| Visual Studio/MSVC | `Visual Studio 2022 Build Tools 17.14.33 / MSVC 19.44.35227.0` |
| Windows SDK | `10.0.26100.0` |
| CMake / Ninja / Python | `4.3.3 / 1.12.1 / 3.12.13` |
| Qt kit/path | `Qt 6.11.1 MSVC 2022 x64 at out/toolchains/qt-engineering/6.11.1/msvc2022_64` |
| Phase 1 Graphics compile-only | `passed; superseded by the complete clean physical Visual route` |
| Windows Skia lock | `passed; 46 official transitive dependencies, SHA-256 312a68ce51a2f3c62bc4c8de90d8ec5aad683037cc1882d41e1f76a42110b7a0` |
| Phase 2 physical Graphics | `passed; 38/38 tests on hardware D3D12` |
| Phase 2 Visual build/tests | `passed; clean rebuild and 44/44 CTest tests` |
| Studio manual launch | `passed with C:\Users\hp\Desktop\TEST\Project.rfx` |
| WARP/software fallback observed | `no` |
| CPU project-pixel fallback observed | `no` |
| Final result | `Windows build, tests, D3D12 rendering and Studio playback passed; final Metal/D3D12 physical pixel qualification is blocked on a fresh macOS capture from this commit` |

#### First causal failure

- Failed step: `windows-visual-capture` in the physical bring-up orchestrator.
- Exact command: `C:\Users\hp\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe tools/qualification/compare_visual_captures.py docs/evidence/reviews/artifacts/xplat-visual-v1-macos-metal-640x360.ppm C:\Users\hp\Desktop\RFr\ReFusion-Pro-Ultimate\out\evidence\xplat-visual-v1-windows-d3d12-640x360.ppm --output C:\Users\hp\Desktop\RFr\ReFusion-Pro-Ultimate\out\evidence\xplat-visual-v1-metal-vs-d3d12.json`.
- Exit code: `1`.
- First meaningful diagnostic: `maximum_channel_delta=24 (limit 8) and pixels_over_delta_3_ratio=0.0065928819 (limit 0.005); mean_absolute_channel_delta=0.1082696759 and SSIM=0.9996202039 passed their limits`.
- Classification: `cross-platform reference/evidence mismatch after a shared raster-policy change; requires physical macOS confirmation`.
- Why this is the causal failure: `repository policy, Core 30/30, official fonts, official Skia materialization, Graphics 38/38, hardware D3D12 capture, a separate clean Visual build, and Visual 44/44 all passed. The checked-in Metal image predates this tested commit and cannot qualify the changed shared raster policy. A same-commit Metal capture is required to distinguish reference invalidation from a remaining backend pixel difference; thresholds were not weakened`.

#### Evidence and artifacts

- Main bring-up receipt: `out/evidence/windows-visual-physical-1e2dc89.json` (`qualifying_source=true`, clean initial/final source tree; stops at the comparison above).
- D3D12 qualification receipt: `not-produced because the orchestrator correctly stopped at the first failed physical comparison`.
- Visual comparison: `out/evidence/xplat-visual-v1-metal-vs-d3d12.json`.
- D3D12 capture: `out/evidence/xplat-visual-v1-windows-d3d12-640x360.ppm`, SHA-256 `515341da22c360a919a0ef715375a54a3cbfc08c9ed9b03bf191e4f4cd85ea3d`.
- Reference capture: `docs/evidence/reviews/artifacts/xplat-visual-v1-macos-metal-640x360.ppm`, SHA-256 `042200df6dee015c4065a1556049bf8a52798a8f78e9a617e47111bc38bc8d8b`.
- Relevant log paths: `out/build/windows-visual/Testing/Temporary/LastTest.log`; `out/evidence/studio-fit-final-20260808-221251.stdout.log`; `out/evidence/studio-fit-final-20260808-221251.stderr.log` (both Studio logs are empty).
- Screenshots: `out/evidence/studio-fit-final.png` (Intel hardware, Fit view, full portrait canvas, PLAYING/GPU READY, more than 1,200 presented frames).

#### Manual Studio observations

- Launcher/create/open: `opening the existing TEST/Project.rfx passed manually; create/reopen passed automated project workspace and launcher tests`.
- Canvas/D3D12 presentation: `passed on Intel UHD Graphics; Fit preserves the 1080x1920 project aspect ratio and uses full-resolution shared Skia composition followed by staged GPU reduction`.
- Timeline transport/playhead: `passed manually; Play remained responsive for more than 1,200 presented frames`.
- Shape/Text/Group/background/FX: `the black/blue gradient background and centered spring text passed manually; broader shape/group/FX behavior passed shared render-plan and fixture tests, not a complete manual authoring sweep`.
- Arabic/Latin packaged fonts: `official-font gate and Skia text-layout tests passed; the manual TEST project exercised Latin only`.
- Resize/minimize/restore/occlusion: `D3D12 presenter recovery tests passed; not repeated as a complete manual window-state matrix`.
- Last-Known-Good diagnostics: `live-reload and invalid-edit behavior passed automated tests; not repeated as a complete manual invalid-edit matrix`.

#### Changes made on Windows

- Build/test policy: `CMakePresets.json`, `apps/cli/CMakeLists.txt`, `apps/studio/CMakeLists.txt`, and `tests/integration/CMakeLists.txt` preserve strict MSVC builds, deploy required Skia runtime data, and prepend the resolved Qt runtime for Qt-backed CTest processes. This fixes the `Qt6Core.dll was not found` test-launch error without embedding a machine-specific Qt path.
- Live reload: `apps/studio/ProjectLiveReloadController.cpp` and `tests/integration/project_live_reload_test.cpp` make Windows directory/file replacement observation and Last-Known-Good diagnostics deterministic while retaining the portable controller contract.
- Official Skia supply chain: `cmake/deps/Skia.cmake`, `deps/locks/skia-transitive-windows-x64.lock.json`, `deps/patches/skia/windows-dynamic-crt.patch`, `deps/profiles/skia/profiles.json`, `deps/profiles/skia/windows-x64-d3d12.gn`, `tools/bootstrap.py`, and `tools/rfdev.py` add a reviewed Windows D3D12 lock, Release `/MD` build, temporary verified Dawn CRT patch, runtime-data checks, and clean materialization verification. No built Skia files are committed.
- Shared cross-platform raster path: `src/adapters/skia/CMakeLists.txt`, `SkiaGpuContextsD3D12.cpp`, `SkiaGpuContextsMetal.mm`, `SkiaSceneCompositor.cpp`, `SkiaSceneCompositor.hpp`, `SkiaSurfacePolicy.cpp`, `SkiaSurfacePolicy.hpp`, `SkiaVisualProgramExecutor.cpp`, and `SkiaVisualProgramExecutor.hpp` use common color/surface/font policy, full-resolution F16 composition, and explicit staged GPU downsampling. The backend-dependent mipmap path that crashed the Intel D3D12 Play route was removed.
- Native presentation: `src/platform/apple/metal/MetalViewportPresenter.mm` and `src/platform/windows/d3d12/DxgiViewportPresenter.cpp` declare equivalent sRGB output intent at the thin platform boundary.
- Portable viewport/digest behavior: `src/runtime/presentation/ViewportPresentation.cpp`, `src/runtime/presentation/include/refusion/runtime/presentation/ViewportPresentation.hpp`, `src/runtime/render/CMakeLists.txt`, `src/runtime/render/RenderPlanCompiler.cpp`, `src/runtime/render/ViewportMapping.cpp`, and `src/runtime/render/include/refusion/runtime/render/ViewportMapping.hpp` centralize Fit/zoom mapping, keep display state out of project persistence, and canonicalize render-plan numeric hashing across toolchains.
- Fixtures/tests: `tests/fixtures/render-plan/xplat-visual-v1/expected-render-plan.txt`, `tests/integration/d3d12_fixture_renderer_test.cpp`, `tests/integration/d3d12_viewport_presenter_test.cpp`, `tests/integration/skia_fixture_renderer_test.mm`, `tests/unit/viewport_presentation_test.cpp`, `tests/unit/visual_render_plan_test.cpp`, and `tests/unit/xplat_render_plan_conformance_test.cpp` cover the new shared mapping, staged reduction, native output contract, and digest.
- Architecture record: `docs/decisions/REGISTER.md` and `docs/decisions/adrs/ADR-0011-full-resolution-canvas-fit-preview.md` document the portable design as `proposed` until same-commit macOS evidence exists.
- Checks rerun after changes: `repository-policy passed; Core 30/30; Graphics 38/38; Visual 44/44; architecture-check 116 files/0 problems/0 boundary debt; docs-doctor 108 docs/0 problems; 46 Skia dependencies verified with clean source trees; manual Intel Studio playback passed`.
- Pushed evidence/fix commit: `1e2dc898fdb50ce7871901159f4e51391350a337; this report is in the containing follow-up commit on the same evidence branch`.

#### Diagnosis and next action

- Root-cause assessment: `three independent issues were confirmed. (1) Fit rendered directly at viewport dimensions with independent X/Y scaling and target-resolution text, producing soft/distorted canvas output; it now uses one portable aspect-preserving mapping and a full-resolution shared source surface. (2) Skia's backend mipmap generation crashed the Intel D3D12 Play path with 0xc0000409; explicit reusable GPU reduction passes remove that backend-specific failure. (3) the Qt6Core.dll dialog came from direct CTest executables lacking the Qt runtime directory in PATH, not from a non-cross-platform application architecture; CTest now receives the resolved kit path. Shared behavior remains in RuntimeRender/SkiaCommon, with only device creation, target wrapping and presentation in Metal/D3D12 adapters`.
- Remaining unqualified areas: `same-commit physical macOS Metal capture/comparison; full manual authoring/window-state matrix; performance profiling; Windows Media Foundation video import/decode remains outside this Canvas run`.
- Exact recommended next action: `on physical macOS, check out this evidence branch/report commit, build the same source and pinned Skia revision, regenerate xplat-visual-v1-macos-metal-640x360.ppm, and rerun compare_visual_captures.py with the existing thresholds. If it passes, produce the final qualification receipt and advance ADR-0011 through normal review; if it fails, investigate the measured shared/backend difference without weakening thresholds or adding a Windows-only visual approximation`.

<!-- WINDOWS_AGENT_REPORT:END -->
