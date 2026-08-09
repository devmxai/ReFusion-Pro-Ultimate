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
- [`Video Import and Hardware Playback Vertical Slice`](docs/plans/VIDEO_IMPORT_HARDWARE_PLAYBACK_VERTICAL_SLICE.md)
  when the requested branch is `feature/shared-video-import-v1` or
  `feature/windows-video-import-v1`.

## Mission for the Windows Agent

Build and diagnose the exact checked-out ReFusion commit on a real Windows x64
machine. Prove what actually works under MSVC, pinned Windows Skia, hardware
D3D12/DXGI and Qt Studio. Never infer runtime success from source presence or a
compile-only result.

The current Windows scope is Canvas/Shape/Text/Group/current FX and the common
RenderPlan/Skia compositor. Windows Media Foundation video decode and complete
Video Layer import/playback are not implemented or qualified by this run.

When the Video vertical-slice branch is published, do not improvise the media
workflow from this general Canvas runbook. Read `PLAN-VIDEO-VS-001` completely,
preserve the existing canonical checkout and its verified `out/` dependency
materialization, and execute VI-WP09 only after the macOS VI-WP08 receipt exists.
The current bring-up script does not itself qualify Media Foundation Video.

## Persistent checkout rule

Normal Windows branch updates must reuse one stable checkout. Do not create a
new clone merely to switch from the shared Video branch to the Windows native
branch, and do not delete `out/`. Without `-FreshDependencies`, the bootstrap
verifies and reuses the existing official Skia/font materialization. Before a
Video-branch build run:

```powershell
git status --short
git fetch origin --prune
python tools/bootstrap.py verify-skia-materialization
```

If verification fails, stop and report the exact failure. The current bootstrap
does not admit arbitrary external/shared caches or copied dependency trees.

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
| Report state | `Cross-platform high-precision Canvas presentation implemented; full Windows build, tests, pixel mapping, pan and live Play passed` |
| UTC timestamp | `2026-08-09T18:48:27Z` |
| Evidence branch | `fix/windows-canvas-quality` |
| Tested source commit | `38536e54fb748d6fa57cc2c54086f5a3672bd0f6` |
| Observed origin/main | `718346cd88e835696d258533660ac5e7f7f48fa0; unchanged and not merged` |
| Windows edition/build | `Windows 11 Pro 23H2, build 22631.6199 (NT kernel string 10.0.22631.0)` |
| CPU/architecture | `13th Gen Intel Core i5-1335U / x64` |
| GPU | `Intel(R) UHD Graphics` |
| GPU driver | `32.0.101.5542, dated 2024-06-07` |
| Windows display path | `SDR DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709; HDR supported but not enabled; internal AUO499F EDID reports 6 bits per color primary` |
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
| Selected presentation profile | `RGBA16F / linear sRGB on the physical Intel D3D12 adapter; BGRA8/sRGB remains the admitted capability fallback` |
| D3D12 presenter stress | `passed; 240 rendered/presented frames across live resize, a real R16G16B16A16_FLOAT Skia target, safe retirement, zero native wait timeouts and zero CPU pixel transfers` |
| Windows Visual tests | `passed; final full build and CTest 45/45 in 15.90 seconds; docs-doctor 112/0; architecture-check 116/0/0` |
| Studio transport | `passed with C:\Users\hp\Desktop\TEST\Project.rfx; UI Play ran beyond the complete 30-second timeline, returned naturally to Play, process stayed responsive, and no Crash/Hang/WER event was recorded` |
| Final result | `Windows now uses the same full-resolution common Skia Canvas policy as Metal source, prefers a high-precision linear carrier, exposes true Fit and 100% mapping with pan, and introduces no Intel-only renderer or project-semantic fork` |

#### Canvas and color diagnosis

- The test project is a 1080x1920 Composition with a valid straight-sRGB dark blue linear gradient. Its authored data was not the cause of the visible lines.
- The old Fit mapping preserved aspect ratio, but its small physical target necessarily discarded source samples. The missing user-facing Actual Pixels mode made 100% impossible to verify and encouraged interpreting Fit as a resolution defect.
- The previous display boundary quantized the final preview to BGRA8 before DWM and used Skia's fixed 8x8 ordered dither. On this dark gradient and 6-bit/FRC internal panel, that repeated screen-space pattern could be perceived as lines or a swirl.
- A first high-precision-only probe removed the fixed pattern but left sparse 8-bit capture levels. A first replacement dither was rejected because measured correlation at offset 8,8 was about 0.83 and could create diagonal structure. The accepted nonlinear float hash has no directional correlation peak.
- Classification: `presentation precision, quantization and viewport-control defects at the shared Canvas/native-carrier boundary; not a malformed project, X/Y coordinate distortion, obsolete Windows 8.1 DLL, Intel-only semantic path or proof that ReFusion is not cross-platform`.

#### Evidence and artifacts

- The physical Intel run selected `RGBA16F / LINEAR sRGB`; the UI telemetry did not infer this from the adapter name.
- The common D3D12 fixture proved Actual Pixels with a 640x360 source rendered into a 320x180 target: output matched the centered 1:1 source crop with maximum channel delta 2, allowing only the shared final dither.
- UI automation switched Fit/100%, paused on one project frame and captured both modes. Fit displayed the complete Composition; 100% displayed the expected centered 1:1 crop. A synthetic 80x48 drag produced the corresponding Canvas displacement and reset correctly.
- Clean dark-gradient analysis increased captured channel levels from `7/20/40` without final dither to `13/25/46` with the shared pass. The former directional 8,8 peak disappeared; correlations became directionally flat with no fixed 8x8 peak.
- The responsive Studio window used `1536x816` Qt workspace points and a `1440x786` client after native frame measurement. DWM's extra invisible resize border was excluded from the content assessment; no Studio content was behind the taskbar.
- Generated screenshots, pixel analysis and runtime logs remain under ignored `out/evidence/`; they are summarized here and are not committed.
- The first full CTest run had one timing-sensitive `project_live_reload` temporary-file observation failure. That unrelated test passed immediately in isolation and the final full run passed 45/45; no live-reload source was changed to conceal it.

#### Changes made on Windows

- Portable presentation leases now carry one validated storage/transfer profile: preferred RGBA16F/linear-sRGB or fallback BGRA8/sRGB. Unknown and mixed pairs fail closed and the selected profile is observable telemetry.
- DXGI probes the real format and color-space present support, selects `R16G16B16A16_FLOAT + G10/P709` on this Intel adapter and keeps a complete BGRA8/G22 fallback. CAMetalLayer source performs the equivalent RGBA16Float/extended-linear-sRGB probe and fallback.
- Both Skia backends wrap the admitted native profile with the matching color type and color space. The common executor retains full-resolution F16 raster, staged Fit reduction, Mitchell sampling and one nonlinear non-periodic encoded-domain final dither before the selected carrier transfer.
- Studio exposes a compact Fit/100% segmented control. Actual Pixels maps one Composition pixel to one physical target pixel and supports direct pan; Fit resets pan and preserves aspect ratio.
- Initial window sizing now uses Qt screen available geometry plus the settled native frame geometry instead of assuming a 1440x900 client fits every DPI/work-area combination.
- ADR-0017 explicitly separates the accepted ADR-0010 project/color semantics from the interactive high-precision display carrier. The Windows and Metal fixtures cover both carrier profiles and true 100% crop behavior.
- The reviewed Windows Skia lock and all dependency identities are unchanged. No Qt/Skia binaries, `.exe`, CMake cache, runtime log, screenshot or `out/` content is committed.

#### Diagnosis and next action

- Application guarantee: `the preferred path postpones quantization to the operating-system display boundary, removes the old fixed ordered pattern, keeps full-resolution project raster and makes Fit versus 1:1 mapping explicit`.
- Display limitation: `the AUO499F panel still reports 6 bits per primary and Windows HDR is disabled. Software cannot make that panel native 10-bit or guarantee identical appearance on an uncalibrated external monitor; fine panel/FRC texture may remain`.
- Cross-platform boundary: `the profile, dither, scaling and Actual Pixels logic are shared, and Metal source/fixtures were updated in the same commit. This Windows host cannot physically compile or execute Metal, so macOS qualification remains mandatory`.
- Git boundary: `origin/main remained at 718346cd88e835696d258533660ac5e7f7f48fa0. This branch must be reviewed on the official macOS host and must not be merged or pushed directly to main from Windows`.
- Next action: `fetch fix/windows-canvas-quality on the official macOS host, build and run the full Metal suite at 38536e54fb748d6fa57cc2c54086f5a3672bd0f6, verify RGBA16F selection or documented fallback, repeat Fit/100%/pan and physical gradient review, then reconcile through the documented integration workflow`.

<!-- WINDOWS_AGENT_REPORT:END -->
