---
id: EVID-XPF-WP05A-SOURCE-2026-08-08
kind: implementation-evidence
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP05
scope: windows-d3d12-dxgi-source-wiring
status: implemented-source-not-run
date: 2026-08-08
---

# XPF-WP05A Windows source-wiring receipt

## Outcome

The repository now contains a Windows visual route from the engine-owned
hardware D3D12 device and direct queue through one Skia D3D12 Ganesh context,
the common visual-program executor, a BGRA8 DXGI flip-model swapchain and an
HWND presenter. Windows Studio selects the same `VisualRuntimeComposition` as
macOS when `windows-visual` is configured.

This is source-readiness evidence only. No Windows compiler, Windows SDK,
physical adapter or Windows Skia artifact exists in the current macOS lab, so
build, runtime, pixel and performance state remain `not-run`.

## Native mechanics added

- Hardware adapter/device/direct queue identity remains owned by
  `D3D12GpuDeviceService`; software adapters and WARP are rejected.
- `SkiaGpuContextsD3D12.cpp` reconstructs and verifies the exact DXGI LUID,
  borrows the engine device/queue and creates one Ganesh D3D12 context.
- The D3D12 renderer validates the complete backend/adapter/generation target
  identity, wraps only BGRA8 resources from the same device, executes the
  shared visual program and flushes with `kPresent` surface access.
- `DxgiViewportPresenter.cpp` owns the three-buffer flip-discard swapchain,
  per-buffer fence values, buffer lifetime, resize quiescence, occlusion test,
  visibility and device-loss/stale-generation rejection.
- Production counters remain zero for CPU project-pixel maps, uploads and
  readbacks. No WARP or software renderer fallback was introduced.
- `windows-visual` is a configure/build/test workflow contract. A Windows-only
  presenter test covers HWND admission, real draw/present, resize, visibility
  skip and zero-CPU-pixel telemetry when executed later.

## Semantic isolation

Both Metal and D3D12 call `execute_visual_render_program`. Exact-time
evaluation and every Shape/Text/fill/gradient/stroke/mask/blend/FX draw
operation remain inside common C++ targets. Native context files no longer
include `RenderPlanCompiler.hpp` or any project document. Architecture checking
now rejects project/effect tokens in either native renderer binding.

## Local verification

```text
python3 tools/rfdev.py architecture-check
  checked_source_files: 101
  problems: []

python3 tools/rfdev.py docs-doctor
  passed

cmake --build --preset macos-visual --parallel 8
  passed

ctest --preset macos-visual --output-on-failure
  47/47 passed
```

This proves the new source did not regress the available Metal lane and that
the source boundary is enforced. It does not prove that MSVC accepts the new
files or that a D3D12 device renders correctly.

## Required formal Windows receipt

1. Materialize the pinned Windows Skia profile on Windows and record its exact
   build/transitive/CRT receipt.
2. Configure, compile and test `windows-visual` with MSVC and the declared
   Windows SDK; run canonical RFX, command, RenderPlan and font receipts.
3. On a named non-WARP Windows 11 adapter, run DXGI present/resize/occlusion,
   device-removal and 10,000-frame soak tests.
4. Run the same visual corpus capture and calibrated Metal-versus-D3D12
   comparison. Record driver/LUID, tolerances and zero CPU transfer counters.

Until these exist, XPF-WP05 and G1-WP02 remain open.
