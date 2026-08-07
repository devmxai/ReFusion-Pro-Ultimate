---
id: G1
kind: stage-plan
status: active-macos-runtime-only
master_plan: MP-001
owner_role: gpu-release-lead
last_verified: 2026-08-07
---

# G1 — Development walking skeleton and kill-risk proofs

## Outcome

Produce non-redistributable development artifacts on macOS and Windows
containing the native Qt shell, engine-owned GPU viewport, GPU-backed Skia
Text/Shape fixture, structured diagnostics, and isolated hardware-media interop
proofs.

## Entry gate

The normal exit remains cross-platform. By product-owner direction, the bounded
G1-WP01 macOS runtime package may execute while G0 Windows evidence is pending,
because no Windows device is currently available. This overlap cannot pass G0,
G1, or any Windows criterion. Commercial SDK/entitlement verification is
deferred and does not block technical work.

## Work packages

1. [`G1-WP01`](work-packages/G1-WP01-macos-presenter.md) — macOS native
   presenter and Skia fixture.
2. [`G1-WP02`](work-packages/G1-WP02-windows-presenter-skia.md) — Windows
   D3D12/DXGI presenter and real Skia/D3D/Dawn bake-off.
3. [`G1-WP03`](work-packages/G1-WP03-apple-media-surface.md) — Apple hardware
   media-surface proof.
4. [`G1-WP04`](work-packages/G1-WP04-windows-media-surface.md) — Windows
   hardware media-surface proof.
5. [`G1-WP05`](work-packages/G1-WP05-gpu-observability.md) — leases, fences,
   counters, traces and device loss.
6. [`G1-WP06`](work-packages/G1-WP06-packaging-signing.md) — reproducible local
   development packaging and clean-machine evidence without redistribution.
7. [`G1-WP07`](work-packages/G1-WP07-mobile-canaries.md) — iOS/Android compile
   canaries without runtime claims.
8. [`G1-WP08`](work-packages/G1-WP08-walking-product.md) — accepted-candidate
   integration and G1 exit artifact.

## Measurements

- physical adapter/device/queue identities;
- native surface import path and all GPU copies/conversions;
- CPU pixel map/readback/upload counters (must be zero in admitted path);
- frame latency, GPU submissions, memory residency, fence ownership;
- unsupported/device-loss behavior;
- install/launch/uninstall evidence on clean systems.

## Kill criteria

Reject a candidate that requires a permanent CPU pixel bridge, UI-owned
swapchain/frame, competing GPU device owner, hidden software decoder, ambiguous
sync/lifetime, or commercially incompatible distribution terms.

## Exit evidence

Mac and Windows development artifacts, traces/captures, dependency fingerprints,
device profiles, diagnostics, clean-machine receipts, and accepted ADRs for the
selected GPU/Skia/media/presentation/package paths. No redistribution or
commercial-entitlement claim arises from G1.
