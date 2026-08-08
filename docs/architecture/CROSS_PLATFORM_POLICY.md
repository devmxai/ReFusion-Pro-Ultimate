---
id: ARCH-XPLAT-001
kind: architecture-policy
status: accepted
owner_role: principal-architecture
canonical_for: cross-platform-build-and-evidence
last_verified: 2026-08-08
accepted_by: product-owner-user-instruction-2026-08-07
---

# Cross-platform build and evidence policy

## Product lanes

- Desktop v1: macOS Apple Silicon and Windows x64 are equal source and product
  lanes. Neither platform may redefine project, layer, timing, command, render,
  media, audio, or export semantics.
- Mobile: iOS and Android receive portable-contract compile canaries during G1;
  full product/runtime qualification remains G9.
- The current physical runtime lab is macOS only. Windows, iOS, and Android
  runtime state is `not-run`, never `passed`, `unsupported`, or silently waived.

## Merge contract

Every shared feature must:

1. place its versioned descriptor, state, migration, validation and evaluator
   semantics in portable C++20;
2. compile exact-time state into one immutable backend-neutral RenderPlan;
3. execute visual meaning through the common Skia compositor;
4. keep OS/GPU/media/window implementation in an explicit native adapter that
   owns mechanics only and does not switch on feature/effect kinds;
5. prepare semantic/runtime capability before atomic accepted-bundle publish;
6. keep Studio commands/snapshots descriptor-driven and platform-neutral;
7. define corresponding macOS and Windows build lanes before integration;
8. use the same fixture, IDs, time domains, semantic/plan digest, diagnostics,
   migrations and failure model;
9. serialize project/Agent truth with locale-free canonical numbers, portable
   case-sensitive ASCII IDs and validated byte-preserved UTF-8;
10. record separately whether each platform is defined, compiled, run and
   qualified.

The repository architecture check rejects platform conditionals in common
semantic code, protects declared macOS/Windows lanes and applies the
`PLAN-XPLAT-FIX-001` visual-boundary ratchet. Existing exact debt is frozen in a
digest-pinned, shrink-only manifest; new signatures, wildcard allowances and
count growth fail. This source check is not physical pixel/runtime qualification.

An FX/plugin is cross-platform only when one contribution identity and semantic
contract passes the same state/migration/plan/diagnostic/conformance corpus on
every required lane. Native C++ extensions are post-v1, out of process and
separately built/signed per target triple; mobile admits no downloaded native
executable plugin. Missing support fails closed without semantic substitution.

## Current execution rule

G1-WP01 may produce the first real visual experience and G1-WP03 may execute the
Apple hardware-media proof on `MAC-LAB-001` because that is the available
physical device. This is a runtime-evidence choice only. Presenter/media
contracts, fixtures, counters and failure semantics remain portable; Metal and
VideoToolbox code stay in Apple adapters, while matching Windows lanes remain
tracked by G1-WP02/G1-WP04 until a Windows device becomes available.

No stage or feature may claim cross-platform qualification from macOS evidence
alone. G1 cannot pass until the required Windows evidence exists.

The `windows-visual` source lane now binds the shared visual-program executor to
Skia D3D12 and an engine-owned DXGI presenter. This is a declared future build
and physical-test route, not passed Windows evidence. Native Metal/D3D/Vulkan
bindings may not include project/compiler headers or own FX-specific lowering.

The authoritative remediation, relocation and plugin admission contract is
[`PLAN-XPLAT-FIX-001`](../plans/FIX_CROSS_PLATFORM_ARCHITECTURE.md).

The admitted G1 mobile compile surface is fixed by `G1-WP07`: iPhoneOS arm64
uses UIKit/Metal without AppKit, and Android arm64-v8a/API 28 uses
ANativeWindow/Vulkan through the pinned official NDK. Both compile the same
Core/RFX/RenderPlan/SkiaCommon semantics and deliberately reject product
presentation. Compile evidence advances only the `compiled` matrix state;
physical run, semantic match, visual tolerance, performance and qualification
remain false until their own device receipts exist.
