---
id: ARCH-XPLAT-001
kind: architecture-policy
status: accepted
owner_role: principal-architecture
canonical_for: cross-platform-build-and-evidence
last_verified: 2026-08-07
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

1. place semantics and public contracts in portable C++20;
2. keep OS/GPU/media/window implementation in an explicit platform adapter;
3. keep Studio commands and snapshots platform-neutral;
4. define the corresponding macOS and Windows build lanes before integration;
5. use the same fixture, IDs, time domains, semantic digest and failure model;
6. record separately whether each platform is defined, compiled, run and
   qualified.

The repository architecture check rejects platform preprocessor conditionals in
Core, Application, Runtime, Studio and CLI. It also rejects removal of any
macOS/Windows Core, Studio, or Graphics CMake lane.

## Current execution rule

G1-WP01 may produce the first real visual experience on `MAC-LAB-001` because
that is the available physical device. This is a runtime-evidence choice only.
The presenter contract, fixture and acceptance criteria remain portable; Metal
code stays in the Apple adapter and the matching D3D12/DXGI implementation stays
tracked by G1-WP02 until a Windows device becomes available.

No stage or feature may claim cross-platform qualification from macOS evidence
alone. G1 cannot pass until the required Windows evidence exists.
