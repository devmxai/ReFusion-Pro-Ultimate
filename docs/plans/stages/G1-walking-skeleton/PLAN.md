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

## Workstreams

1. Qt shell and engine-owned native viewport boundary.
2. `GpuDeviceService`, presenter, resource lease and fence contracts.
3. Skia Ganesh/Graphite/Dawn/native bake-off using the same physical device.
4. Apple VideoToolbox/CoreVideo/Metal surface proof.
5. Windows Media Foundation D3D surface proof and D3D11/D3D12 bridge comparison.
6. Runtime zero-CPU-pixel counters and fail-closed diagnostics.
7. macOS/Windows packaging, signing/notarization skeleton and clean-machine lab.
8. iOS Metal and Android Vulkan contract/build canaries.

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

