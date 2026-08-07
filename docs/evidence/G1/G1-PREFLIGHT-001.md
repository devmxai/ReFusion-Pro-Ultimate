---
id: EV-G1-PREFLIGHT-001
kind: evidence-record
gate: G1-preflight
status: macos-passed-windows-pending
source_commit: 075be91
date: 2026-08-07
---

# Clean Skia and engine-owned GPU preflight

This owner-directed preflight does not activate or pass G1. It removes the
previous placeholder state and proves the clean source/build/adapter contract
on macOS before the full G1 bake-off.

## Source provenance

- Skia origin: `https://skia.googlesource.com/skia.git`.
- Skia revision: `294d31e0b1aa295d585836ab41bd2fba170e0c5d`.
- depot_tools origin:
  `https://chromium.googlesource.com/chromium/tools/depot_tools.git`.
- depot_tools revision: `fa1fac8477c70532274e7244777a846537004750`.
- Both were cloned fresh beneath ReFusion `out/deps-src`; no external checkout,
  machine cache, copied source, mirror or submodule was used.
- Both root worktrees reported `clean=true` and `verified=true`.
- Skia's own `tools/git-sync-deps` resolved 46 Git dependencies.
- Pinned `DEPS` SHA-256:
  `dbd98c21c8736195075482f6e720717edb87da2bd00094a7ca127de86bd26e33`.
- GN SHA-256:
  `76c4542d5ad22fc944a5d50f4db5f2ad2ff3792ac48afb08d4415f0da41ce570`.
- Ninja SHA-256:
  `6c03e94e3ee141301a7e5151227508ac8cec05c12d79ed9240062a86a0e2d14f`.

The generated dependency inventory is retained inside the ignored project path
`out/deps-src/skia-dependencies.lock.json`.

## Build provenance

- Tracked profile: `deps/profiles/skia/macos-arm64-metal.gn`.
- Profile SHA-256:
  `44519d9a31f0b4fed738f6f866dda39bd3bc8fa817dd383bc6e7b5d300680a12`.
- Official GN plus pinned Ninja executed 1,473 build commands for target `skia`.
- The profile enabled Ganesh, Graphite and Metal and disabled GL, Vulkan, Dawn,
  system third-party libraries, partition allocator and performance tracing.
- ICU, HarfBuzz and image codec dependencies were built from Skia's pinned
  sources rather than host packages.
- Fourteen produced static archives were combined deterministically.
- Bundle size: 820,062,752 bytes.
- Bundle SHA-256, reproduced identically by two consecutive bundle operations:
  `f4bda2fdd752cece9983ee90863b1e1d9dac512b23f24948786cc67be95576f9`.
- Build record: ignored project path
  `out/deps-build/skia/macos-arm64-metal/refusion-build.json`.

## Runtime proof

`cmake --workflow --preset macos-graphics` passed 3/3 tests:

1. portable command/revision authority;
2. native Metal device and engine command-queue creation plus valid lease;
3. Skia identity plus Ganesh/Metal and Graphite/Metal context creation from the
   same borrowed engine device and queue.

The three-test workflow and GPU context test were repeated ten times with zero
failure. Core and Qt Studio workflows also remained green.

## Structural proof

- Runtime owns portable GPU identity and generation-bound leases.
- Apple Metal and Windows D3D12 implementations own native device/queue state.
- Skia exists only in the adapter and external dependency boundaries.
- CMake fails closed on incorrect Origin, HEAD, dirty root worktree, changed GN
  profile, missing build record, source/build revision mismatch, missing bundle,
  or artifact SHA-256 mismatch.
- Architecture Check scanned 15 owned source files with zero violations.

## Deliberately unclaimed

- No Windows Skia build or runtime test has run yet.
- No Ganesh-versus-Graphite selection has been made.
- No CAMetalLayer/DXGI presentation, video texture import, synchronization,
  device loss, Arabic typography, performance or packaging claim exists yet.
- No iOS or Android runtime implementation is claimed.
