---
id: EVID-XPF-WP07B-ANDROID-SOURCE-2026-08-08
kind: implementation-evidence
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP07
scope: android-arm64-vulkan-contract-canary-source-and-ci
status: source-defined-local-not-run
date: 2026-08-08
---

# XPF-WP07B Android source-canary receipt

## Implemented contract

The repository defines separate Android arm64-v8a/API 28 Core and Graphics
configure/build workflows. The Graphics lane imports only the pinned official
`android-arm64-vulkan-canary` Skia artifact, compiles the shared
Core/RFX/RenderPlan/SkiaCommon closure and adds thin ANativeWindow/Vulkan
contracts. The native presenter and Skia backend canary deliberately reject
runtime use with `RFX-ANDROID-CANARY-NOT-PRODUCT`; they contain no project,
Layer, Text, Mask, Blend or FX semantics.

The official NDK version `28.2.13676358`, API 28 and ABI `arm64-v8a` are pinned
in `deps/manifest.lock.json`. The CI workflow installs that exact NDK through
the official Android SDK manager, builds the pinned Skia profile, then executes
both CMake workflows. Repository policy rejects missing Android lanes, missing
fail-closed markers and native semantic leakage.

## Truthful local state

`ANDROID_HOME` points to `/Users/mx/Library/Android/sdk`, but that directory and
the NDK were absent on `MAC-LAB-001` during this receipt. Consequently no local
Android configure, compile, emulator or device claim is made. The capability
matrix advances Android only to `defined`; `compiled`, `physically_run`,
`semantically_matched`, `visual_tolerance_passed`, `performance_qualified` and
`qualified` remain false.

## Remaining evidence

The first admitted Android runner must execute `android-core-canary` and
`android-graphics-canary` with the pinned official NDK and Skia build record.
Physical Vulkan presentation, device lifecycle, thermal/performance and visual
comparison remain G9 work and cannot be inferred from compile success.
