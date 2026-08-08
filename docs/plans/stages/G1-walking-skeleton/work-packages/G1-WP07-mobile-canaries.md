---
id: G1-WP07
kind: work-package
status: active
gate: G1
owner_role: mobile-platform
evidence: docs/evidence/G1/G1-WP07.md
---

# Outcome

Compile the portable contracts plus admitted platform-port surfaces for iOS
arm64 and Android arm64-v8a without claiming a mobile product or runtime support.

# Required proof

Separate iOS framework closure without AppKit, Android NDK/SDK pins, Metal and
Vulkan device-port compile contracts, native media-surface type isolation, and
CI artifacts that contain no desktop-only conditional leakage.

# Non-claim

Simulator/emulator or compile success does not qualify runtime GPU/media,
thermal behavior, stores, adaptive UI, or downloadable native extensions.

# Admitted lanes

- `ios-core-canary`: iPhoneOS arm64, iOS 18 minimum, portable Core/RFX/
  Application/RuntimeRender/RuntimePresentation only.
- `ios-graphics-canary`: the same portable closure plus the official pinned
  `ios-arm64-metal-canary` Skia artifact and the fail-closed UIKit/Metal
  platform contract.
- `android-core-canary`: Android arm64-v8a/API 28 portable closure through the
  official pinned NDK.
- `android-graphics-canary`: the same portable closure plus the official
  pinned `android-arm64-vulkan-canary` Skia artifact and fail-closed
  ANativeWindow/Vulkan contracts.

Every lane is a configure+build workflow. Tests and product launch are disabled
because these are cross-compile contracts, not simulator or physical-runtime
qualification. Missing SDK/NDK or missing verified Skia artifacts hides/rejects
the relevant local lane; it never substitutes a host build.

# Architecture boundary

The mobile sources may own only native host/device/queue/target lifetime and
typed failure. They may not include AppKit on iOS, may not inspect project or
RenderPlan authoring types, and may not implement fills, text, masks, blending
or FX. The shared `SkiaCommon` target remains the sole visual-semantic executor.
Both presenters intentionally return `RFX-*-CANARY-NOT-PRODUCT` until G9
admits physical mobile runtime behavior.
