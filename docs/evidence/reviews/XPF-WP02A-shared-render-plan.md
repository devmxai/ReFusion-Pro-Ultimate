---
id: EVID-XPF-WP02A-2026-08-08
kind: architecture-implementation-evidence
status: passed
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP02A
platform: macos-arm64-plus-common-source
date: 2026-08-08
---

# XPF-WP02A shared RenderPlan evidence

## Delivered boundary

- Application two-phase preview/Runtime preparation precedes authority commit.
- Rejected preparation retains the prior accepted Revision and LKG snapshot.
- Studio Runtime publishes a precompiled visual program and prebuilt Canvas/
  Timeline projections; file and UI observers have no Runtime activation power.
- `RuntimeRender` owns the immutable exact-time, revision-stamped
  `VisualRenderPlan` and deterministic receipt.
- `SkiaSceneCompositor.cpp` owns all implemented visual semantics once.
- `SkiaGpuContextsMetal.mm` owns Metal context/target/video binding and GPU
  submission without any Core Project/Layer/FX/Text semantic type.
- `SkiaCommon` is independent from `SkiaMetalBackend`; common CMake no longer
  links PlatformMedia or fails Windows configuration by design.
- Active visual-boundary debt shrank from 45 signatures/85 occurrences to 26
  signatures/53 occurrences. Frozen history remains digest-pinned.

## Verification

```text
cmake --preset macos-core                         PASS
cmake --build --preset macos-core                PASS
cmake --preset macos-graphics                     PASS
cmake --build --preset macos-graphics             PASS
ctest --preset macos-graphics                     PASS except stale allowances detected
ratchet active allowances 009-025 removed         PASS
cmake --preset macos-studio                       PASS
cmake --build --preset macos-studio               PASS
cmake --preset macos-visual                       PASS
cmake --build --preset macos-visual               PASS
ctest --preset macos-visual --output-on-failure   PASS; 41/41
python3 tools/rfdev.py architecture-check         PASS; 81 files, 0 problems
python3 tools/rfdev.py docs-doctor                 PASS before this receipt; 91 docs
```

## Claim boundary

This evidence proves the macOS/common relocation and fail-before-publication
contract. It does not prove MSVC compilation, Windows pixels/performance,
D3D12/DXGI presentation, qualified packaged-font parity, iOS/Android runtime or
preview/export equality. Those states remain open in XPF-WP01/02B/03/05/07/08.
