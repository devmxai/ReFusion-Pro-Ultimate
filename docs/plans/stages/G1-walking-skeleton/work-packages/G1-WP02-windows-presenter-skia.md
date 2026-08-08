---
id: G1-WP02
kind: work-package
status: source-ready-awaiting-windows-device
gate: G1
owner_role: windows-gpu
evidence: docs/evidence/G1/G1-WP02.md
---

# Outcome

Build, link and run the Windows Skia profile on a physical D3D12 adapter; select
Ganesh Direct3D or a proven Graphite/Dawn route and present the same Text/Shape
fixture through an engine-owned DXGI presenter.

# External entry evidence

Named Windows 11 x64 device, adapter/driver/LUID, exact MSVC/clang-cl and Windows
SDK, fresh tracked Windows transitive lock, and CI/device-lab access.

# Required proof

- Implement a real Windows `SkiaGpuContexts` candidate; identity flags alone fail.
- Generate/audit the complete Skia/Dawn system link closure and CRT/ABI policy.
- Assert engine device/queue/adapter LUID identity through draw/submit/present.
- Run 10,000-frame, resize, occlusion and device-removal tests.

# Current source state — 2026-08-08

The engine-owned D3D12/Ganesh context, DXGI swapchain/fence presenter,
Windows live Studio selection, `windows-visual` workflow and Windows presenter
test are implemented. The native binding calls the common visual-program
executor and contains no project/Layer/Mask/Blend/FX semantics. See
[`EVID-XPF-WP05A-SOURCE-2026-08-08`](../../../../evidence/reviews/XPF-WP05A-windows-source-wiring.md).

All Windows build/runtime requirements above remain `not-run`; source presence
does not satisfy this work package.

# Kill criteria

Reject WARP, competing device ownership, unresolved public GPU-context API,
unqualified link closure/CRT mix, or any CPU project-pixel transfer.
