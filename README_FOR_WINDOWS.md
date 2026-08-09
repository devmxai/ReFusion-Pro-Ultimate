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

---

## Windows Agent Report

Replace only the content between the markers. Use factual values; write
`not-run`, `not-produced` or `unknown` instead of guessing.

<!-- WINDOWS_AGENT_REPORT:BEGIN -->

### Latest report

| Field | Value |
|---|---|
| Report state | `not-run` |
| UTC timestamp | `not-run` |
| Evidence branch | `not-run` |
| Tested source commit | `not-run` |
| Windows edition/build | `not-run` |
| CPU/architecture | `not-run` |
| GPU | `not-run` |
| GPU driver | `not-run` |
| Visual Studio/MSVC | `not-run` |
| Windows SDK | `not-run` |
| CMake / Ninja / Python | `not-run` |
| Qt kit/path | `not-run` |
| Phase 1 Graphics compile-only | `not-run` |
| Windows Skia lock | `not-run` |
| Phase 2 physical Graphics | `not-run` |
| Phase 2 Visual build/tests | `not-run` |
| Studio manual launch | `not-run` |
| WARP/software fallback observed | `not-run` |
| CPU project-pixel fallback observed | `not-run` |
| Final result | `not-run` |

#### First causal failure

- Failed step: `not-run`
- Exact command: `not-run`
- Exit code: `not-run`
- First meaningful diagnostic: `not-run`
- Classification: `not-run`
- Why this is the causal failure: `not-run`

#### Evidence and artifacts

- Main bring-up receipt: `not-produced`
- D3D12 qualification receipt: `not-produced`
- Visual comparison: `not-produced`
- D3D12 capture: `not-produced`
- Relevant log paths: `not-produced`
- Screenshots: `not-produced`

#### Manual Studio observations

- Launcher/create/open: `not-run`
- Canvas/D3D12 presentation: `not-run`
- Timeline transport/playhead: `not-run`
- Shape/Text/Group/background/FX: `not-run`
- Arabic/Latin packaged fonts: `not-run`
- Resize/minimize/restore/occlusion: `not-run`
- Last-Known-Good diagnostics: `not-run`

#### Changes made on Windows

- Changed files: `none`
- Reason for each change: `none`
- Checks rerun after changes: `not-run`
- Pushed evidence/fix commit: `not-run`

#### Diagnosis and next action

- Root-cause assessment: `not-run`
- Remaining unqualified areas: `Windows physical evidence; Windows Media Foundation video remains outside this run`
- Exact recommended next action: `Run Phase 1 from a clean checkout`

<!-- WINDOWS_AGENT_REPORT:END -->
