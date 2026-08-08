---
id: EVID-XPF-WP07A-IOS-COMPILE-2026-08-08
kind: implementation-evidence
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP07
scope: iphoneos-arm64-core-render-plan-skia-common-metal-compile-canary
status: passed-compile-canary-runtime-not-run
date: 2026-08-08
---

# XPF-WP07A iOS compile-canary receipt

## Outcome

Both prescribed iPhoneOS arm64 workflows compile successfully with an iOS 18
minimum and strict warnings:

```text
cmake --workflow --preset ios-core-canary
  BUILD SUCCEEDED

cmake --workflow --preset ios-graphics-canary
  BUILD SUCCEEDED
```

The Core lane compiled Core, canonical RFX, Application, RuntimeGpu,
RuntimeMedia, RuntimeRender and RuntimePresentation. The Graphics lane added
the same shared `SkiaCommon` implementation, CoreText font-provider boundary,
Metal backend binding and the UIKit/Metal fail-closed platform canary. No
AppKit or macOS Studio target entered the iPhoneOS closure.

## Verified official Skia input

`tools/bootstrap.py build-skia --profile ios-arm64-metal-canary` built revision
`294d31e0b1aa295d585836ab41bd2fba170e0c5d` from the verified official Skia
checkout inside ReFusion. The build record binds the GN profile, 46 hydrated
dependencies and component archive digests. Its final artifact is:

```text
path:   out/deps-build/skia/ios-arm64-metal-canary/librefusion_skia_bundle.a
size:   2388393760 bytes
sha256: f76819a8e7b372db562e6dca5f23f9aa8200305f0b9c333d5cb3bedd4d8ac790
```

CMake reverified source origin/revision, clean materialization, profile digest,
dependency inventory and artifact SHA-256 before importing it.

## Architecture result

iOS owns only Metal device/queue and UIView-host lifetimes plus typed failure.
The presenter deliberately rejects product presentation with
`RFX-IOS-CANARY-NOT-PRODUCT`; fills, gradients, Text, masks, blending and FX
remain in portable Core/RenderPlan/SkiaCommon. Product Metal creates one Ganesh
execution context; the unused Graphite context was removed and a repository
guard rejects its reintroduction into native product bindings.

## Claim boundary

This advances the iOS capability-matrix state through `compiled` only.
`physically_run`, `semantically_matched`, `visual_tolerance_passed`,
`performance_qualified` and `qualified` remain false. No simulator/device,
thermal, lifecycle, store, mobile UI or runtime-product claim follows from this
cross-compile receipt.
