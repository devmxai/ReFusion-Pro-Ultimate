---
id: EVID-XPF-WP05B-WINDOWS-PHYSICAL-2026-08-09
kind: evidence-review
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP05
tested_source_commit: 1e2dc898fdb50ce7871901159f4e51391350a337
report_commit: c89c79213503e53366ff8642c3d85ab7639bc0d4
platform: windows-x64-d3d12
state: compiled-physical-semantic-pass-visual-performance-open
date: 2026-08-09
---

# Windows physical bring-up reconciliation

The physical Windows report on
`evidence/windows-desktop-v1-20260808-desktop-91mgbu8` is accepted as bounded
evidence for the tested source commit. It advances the Windows Desktop profile
through compile, physical run and semantic conformance; it does not pass visual
tolerance, performance qualification or the complete cross-platform plan.

## Accepted evidence

- Windows 11 Pro 23H2 build 22631.6199, MSVC 19.44.35227.0 and Windows SDK
  10.0.26100.0 built the clean source checkout.
- Official pinned Windows Skia materialization verified 46 transitive
  dependencies.
- Core passed 30/30, physical D3D12 Graphics passed 38/38 and the clean Qt
  Visual lane passed 44/44.
- Studio opened the real project on Intel UHD Graphics, rendered through
  hardware D3D12/DXGI and remained responsive for more than 1,200 presented
  frames.
- No WARP, software GPU or CPU project-pixel fallback was observed.
- The common canonical project, command, font and RenderPlan tests passed under
  MSVC; Windows did not introduce a second project or FX meaning.

The detailed host inventory, changed files, hashes and diagnostics remain in
the bound `README_FOR_WINDOWS.md` report at report commit `c89c792`.

## Open qualification boundary

The physical comparator correctly stopped because its checked-in macOS Metal
reference predates the tested shared full-resolution raster policy. The measured
maximum channel delta was 24 against a limit of 8 and the pixels-over-delta-3
ratio was 0.0065928819 against 0.005; mean absolute delta and SSIM passed. No
threshold was weakened and no D3D12 qualification receipt was fabricated.

Therefore the Windows capability matrix is truthfully:

```text
defined=true
compiled=true
physically_run=true
semantically_matched=true
visual_tolerance_passed=false
performance_qualified=false
qualified=false
```

The later portrait-workspace UI commits were physically tested on macOS only.
Windows Media Foundation Video import/decode also remains open G1/G4 work.

## Exact next action

Promote the reconciled Windows base plus reviewed macOS UI checkpoint to
`main`. The Windows host then pulls that exact commit, reruns the UI/Studio lane
on a new evidence branch and produces a fresh D3D12 capture. macOS produces the
same-commit Metal reference and the existing comparator runs without changing
the accepted bounds.
