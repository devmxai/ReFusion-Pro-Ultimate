---
id: PLAN-WEB-VS-001
kind: platform-vertical-slice-plan
status: proposed
master_plan: MP-001
decision_basis: RFC-0003
owner_role: web-platform-architecture
branch: feature/web-professional-spike
base_commit: 541124aaa4ff9c5244b169e75181c350a90016d1
decision_due: WEB-WP00
last_verified: 2026-08-11
---

# Web Platform Professional Vertical Slice

> Proposed bounded experiment. This plan does not activate a Master Plan gate,
> change `CURRENT.md`, qualify WebGPU hardware, or authorize a merge to `main`.

## Outcome

Produce a real browser-hosted ReFusion Studio that preserves the existing
project/command/revision/time/render-plan semantics and executes the common Skia
visual path on WebGPU. The first product-shaped slice must open/create a real
project, render Shape/Text through the same `VisualRenderProgram`, import and
play an admitted 2K/4K H.264 fixture through WebCodecs, and survive browser/Worker
lifecycle failures without corrupting project truth.

The Web shell mirrors the current QML Launcher and Studio detail: dark panel
tokens, header, tool rail, Canvas host, transport, Timeline ruler/tracks and
Inspector controls. UI implementation is Web-native React/TypeScript; UI
authority remains the existing Application/Core command path.

## Non-negotiable invariants

- `RevisionAuthority` remains the only accepted-project authority.
- React, browser storage, Workers and WebCodecs are clients/adapters; none owns
  the project, canonical clock, decoder selection, RenderPlan or LKG state.
- Core/Application/Runtime/RenderPlan contain no DOM, JS, WebGPU, WebCodecs,
  OPFS or browser handles.
- One accepted exact-time `VisualRenderProgram` lowers to one backend-neutral
  `VisualRenderPlan`; Skia executes its visual meaning.
- The WebGPU adapter owns device, queue, target, synchronization, device loss and
  presentation mechanics only. It never switches on Layer/Mask/Blend/FX kinds.
- WebCodecs is used only behind the shared media/time/color contracts. No software
  decoder, CPU YUV/RGB conversion, CPU RGBA upload, GPU readback or silent
  fallback exists in the Web professional path.
- `requestAnimationFrame`, `requestVideoFrameCallback`, AudioWorklet ticks and
  Worker message timestamps are correlated inputs, never a second ProjectClock.
- Project truth contains stable IDs, exact time, relative content-addressed assets
  and UTF-8 bytes only; no browser handles, URLs, origin paths or GPU objects.
- Preview and future export consume the same semantic evaluator and RenderPlan.
- An unsupported or ambiguous browser capability fails closed with structured
  diagnostics and preserves Last-Known-Good.

## Claim boundary

The Web profile is qualified independently from native profiles:

```text
defined -> wasm_compiled -> browser_run -> semantic_match
        -> visual_tolerance_passed -> performance_qualified -> qualified
```

The Web profile may only advertise `browser-mediated-hardware-preferred` when
WebGPU is active, the requested WebCodecs configuration accepts
`prefer-hardware`, the browser reports a power-efficient decode where available,
and ReFusion-owned counters show no CPU pixel transfer. This label does not claim
that the browser did not perform an internal copy or ignored a hardware hint.
`hardware-qualified` and `zero-copy-verified` remain native-style claims that
require an API/device receipt capable of proving them. No Web shipping decision
is made by this plan.

## Browser/device qualification tiers

The initial matrix is deliberately small and physical:

| Profile | Browser lane | Backend under browser | First target |
|---|---|---|---|
| `web-macos-safari-metal-v1` | Safari stable on Apple Silicon macOS | WebGPU over Metal | Shape/Text + 1080p60 + 4K30 |
| `web-windows-chromium-d3d12-v1` | Chrome or Edge stable on Windows x64 | WebGPU over D3D12 | Shape/Text + 1080p60 + 4K30 |
| `web-android-chromium-vulkan-v1` | Chrome stable on named arm64 Android device | WebGPU over Vulkan | Shape/Text + 1080p60 + 2K30 |
| `web-ios-safari-v1` | Safari stable on named iOS arm64 device | WebGPU over platform GPU | create/open + bounded Preview; mobile 4K not assumed |

Firefox/Linux and older browsers remain defined-but-not-admitted until a named
browser/device receipt exists. Browser capability, GPU adapter identity exposed
by the user agent, codec profile, OS, browser build, WASM/Skia digest and test
fixture digest are recorded in every receipt.

## Target source ownership

The Web lane extends existing boundaries; it does not create a second engine:

```text
src/platform/web/
  webgpu/                         # device/target/presenter mechanics
  media/                          # WebCodecs surface bridge and capability probe
  storage/                        # OPFS/file-grant adapter ports
  audio/                          # AudioWorklet bridge and tick correlation

src/adapters/skia/backends/webgpu/
  SkiaGpuContextWebGPU.cpp        # selected Skia backend context only
  SkiaVideoFrameBridgeWeb.cpp     # one selected VideoFrame import only

apps/web/
  shell/                          # React/TypeScript Launcher and Studio chrome
  engine/                         # typed Worker client and WASM bootstrap
  canvas/                         # WebGPU canvas host; no scene semantics
  media/                          # Worker queue and WebCodecs orchestration
  storage/                        # browser grants and OPFS UX
  audio/                          # AudioWorklet lifecycle UI/telemetry
  styles/                         # QML-derived design tokens
  tests/                          # browser/UI/capability fixtures

cmake/toolchains/emscripten.cmake
deps/profiles/skia/web-wasm-webgpu.gn
```

`apps/web` may translate browser events and immutable projections but may not
parse `Project.rfx`, construct IDs, evaluate layers, select frames by average
FPS or write canonical project bytes. The WASM public surface is a narrow typed
command/snapshot/capability ABI. JavaScript does not receive native handles.

## UI parity contract

The QML surface is the visual reference, not an authority to copy mutable state.
The Web shell must preserve these tokens and relationships before adding new
Web-only features:

```text
workspace:       #252b36
panel:           #12151c
panelRaised:     #181c25
border:          #252b36 / #2a3040 where launcher requires contrast
textMain:        #f2f4f8
textMuted:       #8891a4
accent:          #7c5cff
header:          52 px Studio / 64 px Launcher
toolRail:        64 px
inspector:       280–320 px
```

React components map to `Launcher`, `StudioHeader`, `ToolRail`, `CanvasHost`,
`CanvasTransport`, `TimelineProjection`, `InspectorProjection` and
`DiagnosticsConsole`. CSS Grid/Flex handles layout; Canvas/WebGPU handles the
project image and, when needed, dense Timeline drawing. All controls are real
DOM controls with keyboard, pointer, touch, focus, RTL and screen-reader labels.
UI state is a projection cache keyed by accepted Revision/EvaluationStamp, not a
second model.

## Media and timing contract

The media path is split into explicit stages:

```text
browser file grant / OPFS asset
    -> shared source lease and demux/index
    -> Media Worker
    -> WebCodecs VideoDecoder
    -> bounded PTS-ordered VideoFrame queue
    -> WebGPU external texture import
    -> common Skia DrawVideoFrame operation
```

The first corpus is the existing VFR/B-frame/AAC-offset H.264 fixture. Admission
must preserve coded dimensions, profile/level, Rec.709 metadata, exact PTS/DTS,
frame dependency order and the known audio offset. `VideoFrame` objects are
closed immediately after their GPU work is submitted. Queue watermarks, decode
latency, dropped frames, device generation and device-loss recovery are typed
telemetry.

The audio master-clock contract remains Core-owned. AudioWorklet supplies a
correlated endpoint only; pause/seek/loop and stale epoch behavior are tested
against the same ProjectTime as native lanes.

## Performance budgets to calibrate

These are entry targets, not current claims. A named device tier may revise a
budget only through WEB-WP00 evidence:

- UI long task p95 < 8 ms and no input handler > 16 ms;
- accepted command to immutable snapshot visible <= 1 presented frame;
- WASM evaluation + RenderPlan p95 <= 2 ms for the bounded reference project;
- 1080p60 preview: GPU frame p95 <= 12 ms, zero app-owned CPU pixel transfer;
- 2K60 preview where the device profile admits it; otherwise 2K30;
- 4K30 decode/composite: sustained 30 fps for five minutes, dropped frames <=
  0.5% after warm-up, bounded queue memory and no unbounded growth;
- cold first-frame <= 1.5 s after capability probe for a local asset;
- seek-to-correct-frame <= 100 ms warm and <= 250 ms cold on the reference tier;
- AV presentation drift <= 10 ms during the declared five-minute fixture;
- Worker restart, tab resume and device recreation retain the last accepted
  Revision and recover without stale-frame presentation.

4K60, HDR, HEVC/AV1, alpha video and production export are separate profiles;
they cannot be implied by 4K30 H.264 playback.

## Ordered work packages

### WEB-WP00 — Decision, dependency and evidence baseline

**Purpose:** disposition RFC-0003, freeze the Web profile/claim vocabulary,
browser/device matrix, Skia WebGPU option, dependency pins, security headers,
test corpus and performance budgets.

**Deliverables:** accepted/revised RFC or superseding ADR; `web-webgpu` capability
profile schema; Emscripten/React/TypeScript/Skia-Dawn origin and lock plan;
browser support table; baseline UI snapshots; Web qualification receipt schema;
COOP/COEP/CSP deployment contract; explicit unsupported behavior.

**Exit:** owner decision recorded, dependencies have immutable origin/license,
and no code package is activated until the browser-mediated claim boundary is
accepted.

### WEB-WP01 — WASM semantic closure

**Purpose:** compile portable Core/Application/Runtime/RenderPlan and common
Skia sources with Emscripten without Web conditionals in semantic code.

**Deliverables:** Emscripten CMake preset/toolchain; typed WASM ABI for commands,
snapshots, diagnostics and capabilities; canonical RFX/render-plan conformance;
Worker bootstrap and cancellation; deterministic packaged-font loading.

**Exit:** native and WASM produce identical canonical bytes, semantic digests,
registry receipts and RenderPlan digests for the fixed corpus. No DOM/WebGPU
header leaks into Core or common compositor.

### WEB-WP02 — Web-native UI parity shell

**Purpose:** reproduce Launcher and Studio behavior from the QML reference using
React/TypeScript and immutable snapshot projections.

**Deliverables:** Launcher, Header, ToolRail, CanvasHost, Transport, Timeline,
Inspector and Diagnostics projections; QML-derived tokens; keyboard/touch/RTL/
accessibility behavior; typed command client; responsive landscape/portrait
workspace; no fake Canvas or fake video.

**Exit:** browser UI tests prove equivalent commands, focus/shortcut behavior,
Arabic/Latin rendering and responsive geometry. Canvas remains an explicit
engine-presenter boundary and displays a typed unavailable diagnostic until
WEB-WP03 is green.

### WEB-WP03 — WebGPU presenter and Skia WebGPU compositor

**Purpose:** create the real WebGPU device/queue/target path and bind the common
Skia executor to one selected WebGPU backend.

**Deliverables:** adapter-owned opaque Web leases; device-lost/recreation logic;
canvas resize/visibility/swapchain behavior; Skia Graphite/Dawn or qualified
CanvasKit WebGPU materialization; Shape/Text/mask/FX fixture; GPU timestamp and
app-owned copy telemetry.

**Exit:** Safari/macOS and Chromium/Windows named profiles render the same
VisualRenderPlan; semantic digests match; calibrated visual bounds pass; no
WebGL/Canvas2D/DOM-video fallback is used by the professional path.

### WEB-WP04 — Web storage, import and project lifecycle

**Purpose:** adapt browser grants and OPFS to the existing project package and
ImportVideo transaction.

**Deliverables:** file picker/drag-drop grant adapter; project-relative AssetId
and SHA-256 copy; OPFS staging/journal/rollback; reopen/relink; Worker-safe
source leases; browser quota and permission diagnostics.

**Exit:** create/open/import/relink/save/reopen preserve canonical RFX6 bytes,
IDs, digests and LKG under refresh, permission denial, quota exhaustion and
interrupted writes. No browser URL or local absolute path enters project truth.

### WEB-WP05 — Hardware-preferred WebCodecs playback

**Purpose:** bind the existing exact-time media scheduler to WebCodecs and
WebGPU external textures for 2K/4K playback.

**Deliverables:** codec capability probe; H.264 AVC configuration conversion;
PTS/DTS/B-frame queue; VideoFrame lifetime and external texture bridge; Rec.709
color contract; pause/seek/loop/occlusion/background behavior; 2K/4K telemetry.

**Exit:** the fixed VFR/B-frame/AAC-offset corpus reaches the common
`DrawVideoFrame` path with exact frame selection, no app-owned pixel map/readback
or upload, and the declared performance receipt. If the browser cannot admit
the requested profile, presentation fails closed before activation.

### WEB-WP06 — AudioWorklet correlation and transport parity

**Purpose:** prove Web audio endpoint correlation without creating a second
clock or transport authority.

**Deliverables:** AudioWorklet bridge; exact sample-domain offset handling;
pause/seek/loop/visibility recovery; A/V drift telemetry; same command/snapshot
transport projections as native Studio.

**Exit:** the reference fixture meets the drift and stale-epoch budgets on each
admitted browser profile. Audio output remains outside the Canvas/media worker.

### WEB-WP07 — Preview/export consumer parity

**Purpose:** use one semantic RenderPlan for interactive Web preview and offline
Web export where browser codecs and memory limits permit.

**Deliverables:** worker export scheduler; WebCodecs VideoEncoder probe;
streaming mux/write to OPFS; cancellation/recovery; preview/export digest and
color receipts; explicit native/cloud export handoff for unsupported cases.

**Exit:** preview and export use identical project/revision/time/plan digests;
encoded output is qualified only for a declared browser/codec profile. A browser
export failure does not corrupt the project or silently switch semantics.

### WEB-WP08 — Browser qualification, packaging and handoff

**Purpose:** close a repeatable Web evidence loop without turning a browser demo
into a product claim.

**Deliverables:** Playwright/browser automation; real-device Safari/Chromium
receipts; pixel comparator; performance/thermal soak; device-loss/tab-lifecycle
corpus; accessibility audit; PWA/service-worker/cache policy; signed immutable
WASM/Skia assets; branch handoff record.

**Exit:** each profile advances only through states it physically proves. The
branch is merge-ready only after owner review, RFC/ADR disposition, affected
native receipt invalidation analysis and a clean descendant of `origin/main`.

## Dependency graph

```text
WEB-WP00 decision + locks
       |
       +--> WEB-WP01 WASM semantic closure
       |          |
       |          +--> WEB-WP02 UI parity shell
       |          |
       |          +--> WEB-WP03 WebGPU + Skia
       |                         |
       +--> WEB-WP04 storage/import +---> WEB-WP05 WebCodecs video
                                             |
                                             +--> WEB-WP06 audio/transport
                                             |
                                             +--> WEB-WP07 preview/export
                                                          |
                                                     WEB-WP08 qualification
```

WEB-WP02 may build read-only projections in parallel with WEB-WP01, but it may
not fake a project Canvas, timeline clock or video decoder. WEB-WP05 depends on
WEB-WP01, WEB-WP03 and WEB-WP04. WEB-WP07 depends on WP05/06 and the existing
preview/export semantic contract.

## Test and evidence contract

Every Web work package records:

- source commit and branch ancestry;
- Emscripten, Node, React/TypeScript, Skia/Dawn and browser versions/digests;
- OS, architecture, GPU/browser adapter information allowed by the browser;
- project, asset, font, color, registry and RenderPlan digests;
- exact ProjectTime, transport epoch and fixture row;
- defined/compiled/run/semantic/visual/performance/qualified state;
- diagnostics for capability rejection, permission, quota, context/device loss;
- app-owned CPU map/read/upload/readback counters and their known blind spots;
- browser tab lifecycle, Worker restart, memory and thermal observations.

Qualification captures may read back pixels only inside test evidence programs.
Production Web preview/export retain the zero-CPU-project-pixel rule.

## Security and operational gates

- HTTPS, CSP, Trusted Types and immutable WASM/Skia asset digests.
- COOP `same-origin` and COEP `require-corp` or an explicitly qualified
  `credentialless` variant; no cross-origin isolation downgrade.
- No implicit upload of project/media bytes; OPFS and file grants are local.
- Worker parsing quotas, cancellation, memory ceilings and malformed-media corpus.
- Service worker caches only content-addressed build artifacts and never project
  truth or credentials.
- Browser permission revocation, quota exhaustion, origin change and cache
  rollback preserve or safely reopen the last accepted project.
- No arbitrary downloaded native code, public in-process plugin or unbounded
  runtime shader. Declarative assets remain subject to the existing policy.

## Stop conditions

Stop and return to RFC/ADR review if any proposed implementation:

- adds a JS/TypeScript evaluator or second visual semantics;
- puts Web API/native handles into common contracts or project state;
- makes React, a Worker, WebCodecs or `requestAnimationFrame` authoritative;
- requires software decode, CPU pixel upload/readback or a silent fallback;
- cannot preserve canonical RFX/RenderPlan digests in WASM;
- cannot render the same fixture through one Skia compositor;
- treats browser capability hints as proof of native hardware;
- weakens current Desktop/mobile evidence or changes `main` directly.

## Definition of done

The Web vertical slice is complete only when:

1. RFC-0003 is accepted or superseded by an ADR.
2. One Web profile, one browser matrix and one claim boundary are versioned.
3. Native and WASM canonical project/command/registry/font/color/RenderPlan
   receipts match.
4. React UI is a command/snapshot client with parity tests against the QML
   reference and no fake render path.
5. Skia WebGPU executes the same Shape/Text/Mask/FX RenderPlan.
6. The admitted H.264 VFR/B-frame/AAC-offset fixture reaches WebCodecs and the
   common `DrawVideoFrame` path at its qualified 2K/4K tier.
7. Preview/export use the same semantics where export is admitted.
8. Browser lifecycle, permission, quota, device loss, memory and accessibility
   evidence passes for every claimed profile.
9. `docs-doctor`, `architecture-check`, WASM tests and required browser/device
   receipts pass on the same commit.
10. No change is promoted to `main` until the two-host/capability review and
    owner decision are recorded.

## Handoff and rollback

This branch is a proposal/experiment branch. Each package commits source,
tests, contracts and evidence together, pushes to `feature/web-professional-spike`
or a correctly classified child branch, and records `not-run` where a browser
or device is unavailable. Native receipts affected by shared RenderPlan,
compositor, font, color, UI projection or fixture changes are invalidated and
rerun against the same promoted commit. A failed Web package is removed or
disabled through capability admission; it never changes the accepted native
project schema or the Last-Known-Good path.

## Final status

Proposed on 2026-08-11 from source base `541124a`. The plan is intentionally
non-active until WEB-WP00 owner review and RFC-0003 disposition. It does not
claim WebGPU runtime, WebCodecs hardware decode, 2K/4K performance, export,
browser compatibility or cross-platform qualification.
