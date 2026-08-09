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
| Report state | `Play crash fixed; full Windows build, tests, live playback, looping and pause passed` |
| UTC timestamp | `2026-08-09T17:34:24Z` |
| Evidence branch | `feature/shared-machine-dependency-cache` |
| Tested source commit | `1d2af7db8bce522f03a87f53266e53a992d95a17` |
| Windows edition/build | `Windows 11 Pro 23H2, build 22631.6199 (NT kernel string 10.0.22631.0)` |
| CPU/architecture | `13th Gen Intel Core i5-1335U / x64` |
| GPU | `Intel(R) UHD Graphics` |
| GPU driver | `32.0.101.5542, dated 2024-06-07` |
| Visual Studio/MSVC | `Visual Studio 2022 Build Tools 17.14.33 / MSVC 19.44.35227.0` |
| Windows SDK | `10.0.26100.0` |
| CMake / Ninja / Python | `4.3.3 / 1.12.1 / 3.12.13` |
| Direct3D runtime | `inbox D3D12.dll, D3D12Core.dll and dxgi.dll 10.0.22621.5415; SDK D3DCompiler_47.dll 10.0.26100.7705` |
| Machine-cache root | `%USERPROFILE%\.rfx\dc1` |
| Qt cache identity | `Qt 6.11.1 MSVC 2022 x64; f9a2ae394a1062252ee612ba8274f0660d1027241416e38c58ade080c90162db` |
| Skia cache identity | `revision 294d31e0b1aa295d585836ab41bd2fba170e0c5d; artifact SHA-256 878b859e11df5240fdff806e53057a60a49c0e36fe8e60c1f1d47e436` |
| Windows Skia lock | `tracked and unchanged; deps_sha256 cf5e0309bfb2deb9d8ab030937bc2ed98c11d5adfc5531ee6b9721146ff450dd; file SHA-256 0d30f1a9b619ad2087366563fe5263337d065796a8175beb66ed2dd205bb24ac` |
| Skia materialization | `passed; official roots plus 46 clean official transitive Git dependencies` |
| Clean GitHub clone/cache proof | `passed at 8704e2a; no checkout-local dependency source/build/toolchain directories were created` |
| Clean-clone dependency isolation | `passed; no out/deps-src, out/deps-build or out/toolchains after the Visual build` |
| Clean-clone Visual compile | `passed in 327.1 seconds; refusion-studio.exe linked successfully` |
| D3D12 presenter stress | `passed; 240 rendered/presented frames, live 640x360 to 800x450 resize, zero native wait timeouts and zero CPU pixel transfers` |
| Windows Visual tests | `passed; 45/45 CTest tests in 38.79 seconds` |
| Studio transport | `passed with C:\Users\hp\Desktop\TEST\Project.rfx; Play crossed the complete 30-second timeline and looped, Pause held a stable timecode for five seconds, process remained responsive, no new WER event, stdout/stderr empty` |
| Final result | `the reproducible Windows Play crash is fixed at its resource-lifetime cause; no fallback renderer, legacy Windows 8.1 runtime or driver-specific approximation was introduced` |

#### First causal failure

- User-visible failure: `invoking Play terminated refusion-studio.exe with APPCRASH exception 0xc0000005`.
- CDB first causal frame: `GrD3DTextureResource::Resource::freeGPUData()+0x5c attempted IUnknown::Release through an invalid COM object while Skia recycled a completed direct command list`.
- Root cause 1: `SkiaGpuContextsD3D12 constructed GrD3DTextureResourceInfo from a swapchain-owned raw ID3D12Resource pointer. That constructor adopts a reference; it does not AddRef. Skia therefore released ownership that it never acquired and later retained a dangling resource identity`.
- Root cause 2: `after correcting COM ownership, the resize stress test exposed DXGI_ERROR_INVALID_CALL (0x887A0001): completed Skia command lists still held swapchain-buffer references when ResizeBuffers invalidated those targets`.
- Classification: `shared renderer/native-presenter resource-lifetime contract defect, exposed by the Windows D3D12/DXGI backend; not a QML transport defect, Intel-only rendering path, obsolete DLL, or evidence that project semantics are non-portable`.

#### Evidence and artifacts

- Crash debugger log: `out/evidence/studio-crash-cdb.log`; generated evidence remains ignored and is summarized here rather than committed.
- Fixed launch logs: `out/evidence/studio-play-fixed.stdout.log` and `out/evidence/studio-play-fixed.stderr.log`, both empty.
- Automated transport observation: `Play advanced through sampled timecodes at 1, 5, 10, 15, 20, 25 and 30 seconds, looped to the next cycle, and remained responsive. Pause then held 00:00:14:58 for five seconds`.
- Presenter regression: `d3d12_viewport_presenter_test rendered 120 frames, retired/resized the target, then rendered another 120 frames successfully`.
- Repository checks: `full Windows build passed; CTest 45/45; docs-doctor 111 documents/0 problems; architecture-check 116 files/0 problems/0 boundary debt`.
- Dependency reuse proof: `out/evidence/clean-clone-machine-cache.json passed all nine steps with cache resolve in 9.087 seconds and a Visual compile in 327.1 seconds`.

#### Changes made on Windows

- `ViewportFrameRenderer` now exposes a portable `retire_frame_targets()` lifecycle operation. The contract contains no D3D12, Metal, Qt or project-semantic behavior.
- `SkiaGpuContextsD3D12.cpp` explicitly retains the swapchain resource for Skia and synchronously retires completed Ganesh target references when a presenter invalidates them.
- `SkiaGpuContextsMetal.mm` implements the same renderer contract for cross-platform source completeness; existing Metal presentation behavior is otherwise unchanged. The Vulkan canary accepts the contract without inventing a backend path.
- `DxgiViewportPresenter.cpp` waits for native GPU idle and retires renderer target references before buffer resize or detach. Per-frame rendering remains asynchronous; synchronization is limited to target retirement.
- `d3d12_viewport_presenter_test.cpp` now pumps Win32 messages and verifies sustained presentation across a real swapchain resize with actionable failure diagnostics.
- The verified machine-cache implementation remains in commit `8704e2a82d8d4cd4ac21bceb29d0c7cd72fe238d`; this fix does not alter dependency identities or the reviewed Windows Skia lock.
- No Qt/Skia binaries, `.exe`, CMake cache, debugger log or `out/` content is committed.

#### Diagnosis and next action

- Runtime assessment: `the loaded Direct3D and compiler DLLs are modern Windows 11/SDK components. No Windows 8.1-era application runtime was loaded. Updating dependencies blindly would not have repaired the COM lifetime violation`.
- Driver assessment: `Intel 32.0.101.5542 is older than the current Intel 11th-14th Gen package 32.0.101.7088. Updating through HP OEM support, or the Intel generic package after checking OEM compatibility, is recommended independently; the complete fixed run passed on 5542, so the driver is not the causal fix`.
- Cross-platform boundary: `the shared lifecycle contract and both Skia implementations were reviewed, but this Windows host cannot physically compile or execute Metal. macOS must build and run this exact commit before integration to main`.
- Visual qualification boundary: `the prior Metal/D3D12 comparison remains unqualified until a same-commit physical macOS reference is produced; no pixel threshold was weakened in this repair`.
- Next action: `fetch this branch on the official macOS host, run the full Metal build/tests and transport/resize checks at 1d2af7db8bce522f03a87f53266e53a992d95a17, regenerate the physical Metal comparison, then review through the documented integration workflow. Do not merge this branch directly into main`.

<!-- WINDOWS_AGENT_REPORT:END -->
