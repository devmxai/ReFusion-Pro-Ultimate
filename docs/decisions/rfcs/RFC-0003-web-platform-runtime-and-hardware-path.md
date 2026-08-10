---
id: RFC-0003
kind: rfc
status: proposed
title: Web Platform runtime, Skia WebGPU backend and browser-mediated hardware media
owner_role: product-owner
decision_due: WEB-WP00
---

# Problem

ReFusion has native Metal, D3D12 and Vulkan lanes, one portable Core/Application/
Runtime semantic spine, and a common Skia visual-program executor. It has no Web
target. A Web product must preserve the same project truth, exact ProjectTime,
accepted-revision authority, `VisualRenderPlan`, Skia operation order and
fail-closed diagnostics while operating inside the browser security and lifecycle
model.

The Web also cannot expose native GPU handles or force a browser to select a
hardware decoder. WebGPU is a browser-mediated GPU API, and WebCodecs
`prefer-hardware` is a preference rather than an absolute guarantee. This RFC
therefore needs an explicit claim boundary before any Web implementation can be
called professional or cross-platform qualified.

# Required decision

Accept, revise or reject the following bounded architecture for the
`feature/web-professional-spike` experiment:

1. React/TypeScript owns the Web shell, panels, Timeline controls, Inspector,
   Launcher, accessibility and browser input. It emits typed intents and
   displays immutable snapshots; it owns no project truth, clock, decoder,
   render graph or GPU resource.
2. C++ Core/Application/Runtime/RenderPlan and the common Skia compositor are
   compiled to WebAssembly. No Web API type enters Core, project state or the
   common semantic renderer.
3. A thin Web platform adapter owns WebGPU device/context/presenter mechanics,
   Worker lifecycle, OPFS/File System Access, AudioWorklet integration and the
   JavaScript/WASM boundary.
4. The Web visual target uses one qualified Skia WebGPU route (Graphite/Dawn or
   the pinned equivalent) executing the existing `VisualRenderProgram`; it does
   not introduce a JavaScript/WGSL duplicate for Shape/Text/Mask/FX semantics.
5. Media uses the shared demux/index/time contracts and WebCodecs in a dedicated
   Worker. `VideoFrame` enters WebGPU as an external texture when supported; the
   Web route never adds a software-decoder or CPU RGBA fallback.
6. The Web profile is named `web-webgpu-professional-v1` and records browser,
   OS, GPU adapter class, browser build, codec configuration and evidence states
   independently. It may not inherit native Metal/D3D12/Vulkan qualification.
7. Until a browser/device receipt proves the required signals, the Web claim is
   `browser-mediated-hardware-preferred` and `zero-copy-unverified`, not the
   native `hardware-qualified` claim. A missing WebGPU/WebCodecs capability
   fails closed for the Web professional path.

# Options and trade-offs

## A — Qt Quick WebAssembly shell

This maximizes QML source reuse and may reproduce the current shell quickly. It
also makes the browser surface less native, increases bundle/runtime coupling,
and complicates DOM accessibility, IME, clipboard, file handles and the custom
Skia/WebGPU canvas boundary. It remains a possible prototype tool, not the
recommended product shell.

## B — React shell plus an independent JavaScript/WebGPU renderer

This provides a familiar Web stack but creates a second visual meaning. Any
Shape/Text/FX or color difference would have to be repaired twice and would
violate the common RenderPlan/Skia route. Rejected.

## C — React shell plus WebAssembly ReFusion engine and Skia WebGPU

This keeps Web accessibility and input behavior native while reusing the
accepted semantic engine and common compositor. It requires a new Emscripten
build target, a WebGPU Skia backend/context, asynchronous Worker orchestration
and a browser media/storage adapter. Recommended.

## D — Remote/cloud rendering only

This can provide predictable export hardware but does not deliver an interactive
local editor and adds upload, latency, privacy and operating cost. It is a
future export fallback or optional service, not the Web runtime.

# Proposed Web boundary

```text
React/TypeScript shell
  -> typed Web command client
  -> Engine Worker (WASM Core/Application/Runtime/RenderPlan)
  -> immutable snapshots + RenderPlan leases
  -> WebGPU presenter / Skia WebGPU backend

Media Worker
  -> shared demux/index contracts
  -> WebCodecs VideoDecoder
  -> VideoFrame -> GPUExternalTexture

AudioWorklet
  -> correlated realtime ticks only
  -> Core ProjectClock remains authoritative
```

The Web adapter may expose browser capabilities and diagnostics but may not
reinterpret project semantics. `requestAnimationFrame` and
`requestVideoFrameCallback` are presentation scheduling hints only; they never
become the project clock. Browser backgrounding, Worker termination, GPU device
loss and codec reclamation are typed failures that retain Last-Known-Good.

# Hardware and codec claim contract

The Web admission probe must check, at minimum:

- secure context and cross-origin isolation;
- WebGPU adapter/device creation and required limits/features;
- WebCodecs decoder configuration for the exact codec/profile/level, coded size,
  color metadata and B-frame/VFR fixture;
- `hardwareAcceleration: "prefer-hardware"` acceptance plus MediaCapabilities
  power-efficiency signal where available;
- external-texture import and a GPU-only composition submission;
- absence of ReFusion-owned CPU pixel maps, reads, RGB conversion or uploads;
- bounded queue, memory, decode latency and dropped-frame telemetry.

These signals are necessary but cannot reveal all browser-internal copies or
whether a user agent ignored a hardware preference. The evidence schema must
retain that limitation instead of promoting the profile to native hardware
qualification.

# Experiments and evidence

Before accepting this RFC or merging a Web implementation:

1. Compile the existing Core/RFX/RenderPlan test corpus to WASM with byte and
   digest equality against the native lane.
2. Render the existing Shape/Text/mask/Blur/Shadow/Glow fixture through Skia
   WebGPU and compare calibrated captures against the macOS reference.
3. Prove React UI commands and an Agent/CLI-equivalent command produce the same
   accepted revision, project bytes and RenderPlan digest.
4. Import the seven-row VFR/B-frame/AAC-offset corpus, decode a supported H.264
   profile through WebCodecs and preserve exact PTS and audio offset.
5. Exercise device loss, context loss, tab background/foreground, Worker restart,
   codec exhaustion, OPFS failure and stale revision rejection.
6. Measure 1080p60, 2K30/60 and 4K30 on named Safari/macOS, Edge or Chrome/
   Windows and Chrome/Android profiles. 4K60 is a separate performance tier.
7. Run accessibility, keyboard, RTL/Arabic, touch, file-drag and responsive
   layout tests on the same UI states as the QML launcher and Studio.

Evidence is recorded in the Web capability matrix and a qualification receipt;
no screenshot or compile result alone advances a profile beyond its proven
state.

# Security, licensing and platform impact

- The Web build is HTTPS-only and requires CSP, COOP/COEP and immutable,
  digest-pinned WASM/Skia assets.
- Project/media parsing runs in Workers with input-size, decode-count, memory and
  cancellation limits. No media or project bytes are uploaded implicitly.
- OPFS handles and File System Access grants remain adapter-local and never enter
  project truth.
- Emscripten, Skia Graphite/Dawn, React, TypeScript and WebCodecs-related build
  inputs require the normal official-origin, immutable-revision, license and
  SBOM intake before a distributable artifact.
- Browser APIs cannot expose native GPU handles; common contracts therefore use
  the existing opaque lease model plus Web profile identity/capability metadata.
- No downloadable native plugin is admitted. Declarative assets and bounded WASM
  workers remain subject to the existing extension policy.

# Recommendation

Choose Option C. Start with a read-only WebGPU Shape/Text vertical slice after
the decision gate, then add command parity, storage/import, WebCodecs playback,
AudioWorklet correlation and export in separate bounded packages. Keep the
strict native zero-CPU-pixel invariant intact and introduce no Web exception
until the owner explicitly accepts the browser-mediated claim boundary.

# Final disposition

Proposed on the `feature/web-professional-spike` branch. This RFC creates no
accepted architecture, no new Master Plan gate and no shipping Web claim.
The product owner must accept, revise or reject it at `WEB-WP00`; an accepted
outcome must be recorded in an ADR before the Web path is promoted to `main`.
