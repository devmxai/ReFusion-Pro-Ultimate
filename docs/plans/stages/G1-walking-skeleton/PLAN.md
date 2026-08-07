---
id: G1
kind: stage-plan
status: planned
master_plan: MP-001
owner_role: gpu-release-lead
last_verified: 2026-08-07
---

# G1 — Signed walking skeleton and kill-risk proofs

## Outcome

Ship internal macOS and Windows application artifacts containing the native Qt
shell, engine-owned GPU viewport, GPU-backed Skia Text/Shape fixture, structured
diagnostics, and isolated hardware-media interop proofs.

## Entry gate

G0 is passed; Qt license/module lane and pinned foundation dependencies are
approved; device lab and signing paths have named owners.

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
6. [`G1-WP06`](work-packages/G1-WP06-packaging-signing.md) — packaging,
   licensing, signing and clean-machine evidence.
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

Mac and Windows artifacts, traces/captures, dependency fingerprints, device
profiles, diagnostics, clean-machine receipts, and accepted ADRs for the selected
GPU/Skia/media/presentation/package paths.
