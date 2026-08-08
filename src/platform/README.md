# Platform implementations

This boundary owns native window, GPU, media, audio and lifecycle objects behind
Runtime contracts. Desktop GPU ownership begins with real Metal and D3D12
implementations. iOS and Android remain separate compile canaries until their
product gates; they may not redefine project or render semantics.

Native code is limited to device/queue/target/host lifetime, native media-surface
import, synchronization, submission, presentation and lifecycle telemetry. It
must not include project documents or switch on Composition/Layer/Mask/FX/Text/
Shape/Blend kinds. The relocation ledger is
`docs/plans/FIX_CROSS_PLATFORM_ARCHITECTURE.md`.

G1 mobile contracts are explicit compile canaries: `apple/metal_ios` owns a
UIKit/Metal lease boundary and `android/vulkan` owns an ANativeWindow/Vulkan
lease boundary. Both fail closed with `RFX-*-CANARY-NOT-PRODUCT`; neither is a
mobile application, presenter qualification or permission to duplicate visual
semantics. Their build authority is the `ios-*-canary`/`android-*-canary`
presets and `.github/workflows/mobile-contract-canaries.yml`.
