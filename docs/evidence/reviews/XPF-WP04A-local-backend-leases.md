---
id: EVID-XPF-WP04A-LOCAL-2026-08-08
kind: implementation-evidence
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP04
scope: common-contract-plus-macos-metal
status: passed-local-not-cross-platform-qualified
date: 2026-08-08
---

# XPF-WP04A local backend-lease receipt

## Outcome

The common GPU/presentation boundary no longer assumes that a backend is two
integer handles and one integer texture. ReFusion now passes lifetime-bearing,
backend-opaque leases with full adapter identity. Metal/D3D12/Vulkan native
types remain outside Core, Application, Runtime and common Skia.

This receipt covers the portable contract and real macOS Metal execution. It
does not qualify D3D12 or Vulkan.

## Replaced prototype contracts

| Removed prototype | Accepted replacement |
|---|---|
| `NativeHandles { uintptr_t device, command_queue }` | `BackendDeviceLease` with full `DeviceIdentity` and shared opaque device/submission lifetimes |
| `NativeFrameTarget { uintptr_t texture }` | `BackendFrameTargetLease` with full device identity, target ID, extent, format and opaque target/sync lifetime |
| `NativeViewportHost { uintptr_t handle }` | `NativeViewportHostLease` acquired and retained by the platform adapter |
| `FixtureFrame` | `PresentationFrameRequest` with exact ProjectTime, clock epoch, device identity and immutable accepted render-program lease |
| mutable program retained by `SkiaGpuContexts` | coherent program lease supplied per presentation request |

## Integrity behavior

- Device, target and request compare backend, adapter ID and generation before
  native wrapping or submission.
- An in-flight frame owns one immutable program, so live reload cannot change
  project/revision/composition halfway through that frame.
- The request sequence is telemetry only; Skia uses Core ProjectTime and the
  transport epoch and never chooses a timebase.
- Metal retains its NSView and MTLTexture for the lifetime of the corresponding
  leases. The device/queue lease aliases the engine-owned Metal state.
- Backend-private opaque access is limited by `architecture-check` to native
  adapter translation units; common code may inspect identity/metadata only.
- Missing, invalid, stale-generation and cross-adapter request/target pairs fail
  before GPU submission and preserve Last-Known-Good.

## Verification

```text
python3 tools/rfdev.py architecture-check
  checked_source_files: 97
  problems: []

cmake --build --preset macos-visual --parallel 8
  passed

ctest --preset macos-visual --output-on-failure
  47/47 passed
```

The physical Metal suite covers device lifecycle, real CAMetalLayer target
acquisition, Skia submission, retained target lifetime, zero CPU-pixel counters,
stale target generation, stale request generation and device-loss rejection.

## Remaining formal exit work

1. Implement the D3D12 lease private state with explicit resource state,
   acquire/release transition ownership, fence value and DXGI present lifetime.
2. Implement the Vulkan lease private state with image/image-view, current and
   required layouts, queue-family ownership, semaphores/fences and present
   lifetime.
3. Run common/Metal/D3D12/Vulkan lifecycle tests in their declared build lanes
   and physical device profiles.

Therefore the truthful state is: common/macOS WP04A passed; formal XPF-WP04
remains open and Windows/mobile evidence remains `not-run`.
