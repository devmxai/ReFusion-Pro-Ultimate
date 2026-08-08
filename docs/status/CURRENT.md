---
id: STATUS-001
kind: program-status
master_plan: MP-001
master_plan_status: active
current_gate: G0
gate_status: active
active_work_packages:
  - G0-WP03
  - G0-WP04B
  - G0-WP05
active_guardrails:
  - PLAN-XPLAT-FIX-001
baseline_commit: 1a70f5a
last_green_commit: a697c6a873760366bb957590038cbf3416e644d0
last_checkpoint: CP-G1-0009
blocking_risks:
  - RISK-002
  - RISK-003
last_updated: 2026-08-09
---

# Current program status

## Exact resume point

The cross-platform remediation guardrail
[`PLAN-XPLAT-FIX-001`](../plans/FIX_CROSS_PLATFORM_ARCHITECTURE.md) is active.
The locally executable `XPF-WP01A/B`, `XPF-WP02A/B`, `XPF-WP03A`, common/
Metal `XPF-WP04A`, Windows-source `XPF-WP05A`, local `XPF-WP06A` and iOS
`XPF-WP07A` slices, plus the local `XPF-WP08A` Preview/Export semantic contract,
now pass their available checks; formal Windows and Android receipts remain
`not-run`. D3D12 context/target and DXGI swapchain/fence mechanics are defined
and Windows Studio selects the common visual runtime. The exact resume point is
now the `Pre-Windows Source Closure` in
[`PLAN-XPLAT-FIX-001`](../plans/FIX_CROSS_PLATFORM_ARCHITECTURE.md): accept and
bind the Desktop v1 SDR color contract, finish the Windows Core/Graphics/Visual
build entrance, replace unbounded D3D12 waits with typed bounded failure,
complete the test-only offscreen qualification consumer/corpus, commit the
mobile-canary automation, preserve the full working source in one reviewed Git
checkpoint, and rerun the macOS/iPhoneOS readiness gates. This work is local and
does not enter G4 production Export.

Only the resulting `XPF-PRE-WINDOWS-READY` commit is handed through GitHub to
the physical Windows host. That host materializes and locks official Windows
Skia, builds Core -> Graphics -> Visual under MSVC, runs the common semantic,
pixel, performance, recovery and non-WARP corpus, and returns human/machine
evidence in a source-descendant commit. macOS then reconciles the receipts; any
shared correction invalidates affected evidence and is rerun on both Desktop
lanes. Android official-NDK compile evidence remains a separate plan-exit input.
Windows Media Foundation video decode is still open G1/G4 work and is not
implied by a successful Windows Canvas Preview.

`XPF-WP02C` fixes a reproduced Agent live-reload self-deadlock. Application no
longer emits Studio/Qt projections while holding its accepted-state admission
mutex: Core and Runtime commit first under exclusion, then Canvas, Timeline,
Inspector and diagnostics publish immutable projections after unlock. A test
re-enters `active_snapshot()` from synchronous projection publication and
observes the new Revision. The real live-reload tests and all 49 macOS Visual
tests pass, as do all 28 sanitized Core tests; the rebuilt Studio opened the
affected `mm1` project at Revision 2, wrote its accepted journal receipt and
remains in the normal Qt event loop.

`XPF-WP08A` adds one consumer-neutral Runtime entrance for interactive Preview
and future Offline Export. Consumer identity is metadata only; the accepted
program at the same exact ProjectTime must produce the same immutable
VisualRenderPlan and semantic digest. Production Preview uses this route, and
the contract rejects different project samples, invalid consumer pairs and
unknown consumers. Independent scheduler epochs remain legal. The updated
iPhoneOS Graphics closure compiles the same contract and shared Skia consumer.
macOS passes 49/49, architecture-check covers 110 sources with zero problems
and zero visual-boundary debt, and policy/docs checks pass. This is not a G4
Export, encoded-output, visual-tolerance or performance claim.

`XPF-WP08B` records the bounded physical macOS Metal Preview profile. The
corrected 10,000-frame loop submitted every frame with zero CPU pixel transfer,
GPU readback or unattributed copy. GPU observability stayed inside the named
Apple-M1 640x360 budget, temperature was nominal, injected device loss was
observed and no stale-generation resource was accepted. The first attempt
correctly failed when the test requested a ProjectTime outside the 30-second
Composition; the test now loops legal time and Runtime remains fail-closed.
The Core closure passes 28/28 under ASan/UBSan. This still does not qualify
Export, Windows, Android or cross-backend pixels.

`XPF-WP07A` now has real iPhoneOS arm64 compile evidence. Both
`ios-core-canary` and `ios-graphics-canary` built successfully; the latter
imports a digest-verified official `ios-arm64-metal-canary` Skia artifact and
compiles the same Core/RFX/RenderPlan/SkiaCommon sources plus thin UIKit/Metal
contracts. iOS advances only through `compiled` in the capability matrix.
Android arm64-v8a/API 28 presets, Vulkan/ANativeWindow canaries, pinned Skia
profile and official-NDK CI are source-defined, but this host has no Android
SDK/NDK, so Android remains `defined=true, compiled=false`. Both mobile
presenters fail closed and make no runtime-product claim. The unused second
Graphite product context was removed; policy now rejects its return. The latest
aggregate macOS and architecture results are recorded in `XPF-WP08A` above.

`XPF-WP06A` adds one portable Registry for every currently admitted Mask/FX
ID, capability, schema, owner, typed parameter, unit, default, range and
validation rule. Inspector controls, Timeline names and generated project-local
Agent documentation derive from it; concrete FX/mask vocabulary branches are
gone from Studio/QML. Typed scale/opacity keyframes now enforce their ranges.
The guarded capability matrix reports macOS, Windows, iOS and Android states
without promoting source definition into compile/run/qualification evidence.
The macOS Visual lane passes 48/48; architecture-check inspects 105 sources with
zero active visual-boundary allowances and zero problems. RFX5 binds the
contribution Registry and uses one generic typed-parameter codec while
RFX1–RFX4 remain readable migrations. RenderPlan contributions carry Registry
descriptor/capability/schema identity in their semantic digest. This completes
the locally executable WP06 route; non-macOS matrix and final profile-admission
evidence still prevent formal WP06 closure.

Application performs
Runtime candidate preparation before the accepted Core commit; Studio file/UI
observers no longer activate Runtime afterward. `RuntimeRender` lowers Core's
single exact-time evaluation to immutable backend-neutral VisualRenderPlans.
All current Shape/Text/fill/gradient/stroke/corner/mask/blend/Blur/Shadow/Glow
execution moved out of Metal into `SkiaSceneCompositor.cpp`; effect isolation
is bounded. `SkiaCommon` and `SkiaMetalBackend` are separate targets and the
Windows common build is no longer stopped by CMake. Nineteen active boundary
allowances were retired, architecture/docs checks pass, and the macOS Visual
lane passes. This is not full cross-platform qualification:
Windows MSVC execution, D3D12/DXGI presentation, the full
cross-backend pixel corpus and mobile canaries remain open.

`XPF-WP02B` now also has a repository-owned cross-toolchain conformance fixture.
One legacy RFX4 migration fixture canonicalizes to RFX5 and covers all current
fill/content/blend/mask/FX families,
Group hierarchy and animation at four exact frames with padding-free expected
RenderPlan digests. A test-only Metal capture renders the same fixture through
SkiaCommon and passes calibrated coverage/background predicates; production
still performs no pixel readback. The corpus runs in Core lanes and is ready
for MSVC, but Windows execution remains `not-run`, so cross-backend parity is
not yet claimed.

`XPF-WP01A` now establishes the local canonical project contract. RFX, Agent
JSON numeric values and project/registry/text/RenderPlan receipts use explicit
locale-free numeric/hex spelling; invalid UTF-8 and prohibited controls fail
before publication; valid Unicode bytes are preserved without silent
normalization; and entity IDs use a path-free, case-sensitive ASCII grammar.
The same Arabic/Latin corpus has fixed project and registry receipts and stays
byte/digest-identical under a synthetic decimal-comma locale. MSVC remains
`not-run`; authored-coordinate quantization, command-output receipts,
copied-workspace regeneration and mobile canaries are still open WP01 work.

`XPF-WP01B` now closes those locally executable coordinate/workspace items.
Typed Transform/property and measured Align pixel results commit on the Core
`1/1024 px` grid, and a hostile-locale command receipt fixes the exact Revision,
project digest/size and coordinates. A copied `.refusion` state—including a
lock copied from an active source session—is regenerated for the destination
canonical path without weakening same-path exclusion. Agent context schema v2
stores unsigned Revision values losslessly. WP01 still cannot close until the
same receipts and canonical resave pass under MSVC/Windows and mobile canaries.

`XPF-WP03A` closes the locally executable deterministic-font slice. Official
Noto Sans 2.015 and Noto Sans Arabic 2.013 font bytes and OFL notices are pinned
by archive/member SHA-256, materialized only inside ReFusion and transactionally
packaged into each new project. A path-free Core resolver feeds the same exact
bytes to Inspector and Canvas; common Skia constructs typefaces through embedded
FreeType and shapes/wraps through HarfBuzz/ICU with cluster-safe spacing and no
hinting. Platform font managers are isolated and explicitly unqualified. The
Latin/Arabic/RTL/mixed/diacritic/wrap receipt and full macOS Visual lane pass
47/47, while architecture-check inspects 97 source files with zero problems.
MSVC execution remains `not-run`, and future general fallback chains must be
authored and digested explicitly; therefore formal WP03 is not yet closed.

`XPF-WP04A` closes the locally executable backend-lease slice. Raw integer
device/queue/texture/viewport-host envelopes were replaced by full-identity,
lifetime-bearing opaque device, target and host leases. Every presentation
request carries exact Core ProjectTime, transport epoch, complete device
identity and one immutable accepted render-program lease. Skia no longer owns
a second mutable project/render-program pointer; stale target/request generation
or cross-adapter identity fails before target wrapping/submission. Metal retains
the NSView/MTLTexture lifetimes, and architecture-check forbids backend-private
access from common sources. The full macOS Visual lane passes 47/47. Formal WP04
still requires the D3D12 implementation's Windows build/device receipt and a
Vulkan layout/queue-family/sync implementation plus receipts.

`XPF-WP05A` now defines the Windows visual source route without a semantic
fork. The D3D12 binding verifies the engine LUID/device/queue, wraps BGRA8 DXGI
targets and calls the same common exact-time visual-program executor used by
Metal. The DXGI presenter owns its flip-model swapchain, back-buffer fences,
resize/occlusion/device-loss mechanics, and `windows-visual` selects the same
Studio runtime. Native renderers are now automatically forbidden from
including project/compiler headers or owning current FX semantics. This source
state is not Windows evidence: MSVC, pinned Windows Skia linking, a physical
non-WARP adapter, soak/device-removal and pixel parity remain `not-run`.

`G0-WP02` passed after the product owner selected Qt Commercial in ADR-0005.
`G0-WP03` Core code and CI definition remain complete but still need a real
Windows x64 run. By explicit owner direction, checkpoint `CP-G0-0004` also
materialized a fresh official Skia dependency graph inside ReFusion and proved
the engine-owned Metal plus same-device Ganesh/Graphite contract on macOS.
Checkpoint `CP-G0-0005` passed `G0-WP04A`: one private Application Host now
owns mutable project authority, while Studio and CLI submit commands through the
same service. The local controls in `G0-WP04B` are code complete and fail closed;
Windows materialization/build/runtime still requires external evidence. By the
product owner's direction in `CP-G0-0006`, Qt Commercial SDK and entitlement
verification is explicitly deferred to the redistributable release-candidate
gate and does not block G0/G1 engineering. Checkpoint `CP-G0-0007` accepts
cross-platform-first implementation with macOS-only physical runtime evidence
while no Windows device is available. `G1-WP01` is active for the first real
visual experience on macOS; Windows remains `not-run`, and neither G0 nor G1 may
pass from the macOS evidence alone.

Source commit `4c91df7f6abf0734a47cfb549a726d15ef35281e`
delivered the first real visual application experience: the Qt shell embeds an
engine-owned CAMetalLayer, Skia renders a GPU-backed Arabic/Latin Text/Shape
fixture on the same Metal device/queue, and a 10,000-frame presenter soak
retained zero CPU pixel maps, uploads, readbacks, or unattributed GPU copies.
`G1-WP01` remains active for physical sleep/wake, occlusion and device-loss
qualification; this visual slice does not pass Windows or G1.

By explicit product-owner direction, source commit
`b850a84ce8aff860bf8aac786440b396eb77024e` made the macOS experience a
file-backed project-open test instead of a hard-coded visual mock. The app now
opens a validated 30-second 1080x1920 Reels Composition with stable Layer IDs,
exact integer time and keyframed Shape/Text content; Runtime owns continuous
30 fps looping and the UI only displays telemetry/snapshot metadata. Proposed
ADR-0008 and the experimental schema record the boundary. This is a G2 seed,
not G2 activation or a Video Layer/media/save/export claim.

Source commit `73d59a7960bae38ba7e2e9c41ae2df4898c8f565` hardened the
GPU lifecycle contract across the declared Metal and D3D12 lanes. macOS now
observes native system sleep/wake and window occlusion, presentation reports
typed health/generation telemetry, loss advances the generation, and Runtime
stops fail-closed on its first rejected frame. Physical full-window
occlusion/resume and injected device-loss receipts pass with zero CPU pixel
transfer. The product owner then reported a successful manual sleep/wake test
and explicitly made retention of the automated physical receipt non-blocking
for current development. `G1-WP01` is therefore accepted for the bounded macOS
walking runtime, while its automated sleep/wake receipt remains deferred and
must not be represented as lab-qualified evidence.

By the same cross-platform-first/macOS-runtime-only direction, `G1-WP03` was
admitted for the Apple H.264 hardware-media surface proof and is now closed by
`CP-G1-0008`. Its semantic contracts, strict counters and failure model remain
portable and the matching Windows build lane remains fail-closed. Windows
runtime qualification and G1 exit remain blocked until a physical Windows
device exists.

Source commit `ac8de22c329a64a9f351796ae5ded280e701984c` delivered the
first G1-WP03 slice. Portable Runtime now owns exact source-frame timing, the
narrow H.264/NV12/SDR Rec.709 profile, typed capability outcomes and strict
zero-CPU/fallback/cross-adapter counters. The Apple adapter proved that H.264
hardware decode capability exists and that a two-plane NV12 native surface can
bind to Metal textures on the engine-owned adapter/generation. The matching
Windows media lane exists but deliberately returns not-qualified until G1-WP04
runs on Windows. This slice does not yet decode a compressed H.264 sample.

By explicit product-owner request, source commit
`79aafb072ad0f254ef64726231b2e7c7e50b5fe8` added a bounded exact
transport-control slice to the real file-backed walking project. Portable
Runtime owns typed Play/Pause/Seek-to-frame commands, frame/rational-time
mapping and immutable playback state. A separate Qt transport bridge submits
commands and exposes read-only snapshots/track ranges; QML owns no timer or
transport clock. The native Timeline now has a centered Play/Pause control,
time ruler, real track extents and a clickable/draggable playhead. Physical
macOS observation proved pause stability, resume without reset and paused seek
updating the Skia Canvas. This is not decoded-video playback or G4 completion.

Source commit `1433a576e75f47fa4377259da9c095eab20291a9` delivered the
first actual compressed-video GPU slice. A repository-owned H.264 High/Rec.709
fixture enters a VideoToolbox decompression session that requires and confirms
hardware acceleration. Exact PTS/duration survive into an opaque engine surface
lease; the NV12 output binds as two textures on the engine Metal adapter, and
Skia composites the decoded surface into a private GPU render target. All
software-decode, CPU-pixel, readback and cross-adapter counters remain zero.
This is a bounded all-IDR single-frame proof, not MP4 import or Timeline video
playback.

Source commit `a697c6a873760366bb957590038cbf3416e644d0` makes portable
Core `ProjectClock` the executable single authority for transport state,
ProjectTime, source generation and epoch under accepted ADR-0009. The viewport
scheduler no longer owns mutable playback position. One hardware-required
VideoToolbox session now decodes all eight bounded all-IDR access units into an
immutable exact-PTS-indexed same-device surface queue; Core seeks `[3, 7, 1]`
select and GPU-composite source frames `[3, 7, 1]` in Skia with all forbidden
counters zero. This remains a G1 proof, not the final audio ClockSource, MP4
import, dependency-aware production seek or Timeline Video Layer playback.

Checkpoint `CP-G1-0008` closes bounded Apple `G1-WP03`. A repository-owned
16-frame H.264 High fixture now contains real B-frames, variable durations, two
sync-rooted GOPs and a non-zero three-second source origin. Portable Runtime
plans a minimal decode-order dependency window, bounds the immutable PTS queue
and admits it only while the Core transport epoch and engine GPU generation
remain current. Physical VideoToolbox tests decoded and flushed three windows,
released all 19 native surface leases and rejected stale epoch/device results.
Skia selected/composited source frames `[9, 10, 9]` from a two-surface GPU queue
with every forbidden pixel/fallback counter at zero. This completes the bounded
Apple hardware-media surface package; Windows G1-WP04 and product G4 MP4/MOV
import/Video Layer remain separate and not-run/unimplemented respectively.

Checkpoint `CP-G1-0009` closes bounded macOS `G1-WP05`. One portable,
authority-free observability service is now shared by the real VideoToolbox,
Skia and CAMetalLayer paths. It records typed resource/fence leases, attributed
submissions/copies/conversions, actual resident extent, real completion latency,
thermal samples, device loss and stale-generation rejection. The physical Apple
M1 proof released all 15 resources, completed all 3 fences, attributed all 15
submissions and accepted no stale resources; every CPU video-pixel,
copy/conversion, software-decoder, readback and cross-adapter counter remained
zero. The named 640x360 device-tier budget passed. Windows runtime remains
`not-run`, so this does not pass cross-platform G1.

By explicit product-owner direction, pre-G2 `EXP-001` now implements the first
real typed single-file authoring trial. `Project.rfx` compiles in portable Core
to the same immutable project snapshot used by Timeline/Canvas/Skia; accepted
external edits pass through `ReplaceProjectCommand`, while malformed or stale
candidates retain Last-Known-Good and write source-located diagnostics. The
real 30-second Reels example, CLI validate/describe commands and project-local
Agent Skill are ready for owner/agent authoring evaluation. This is evidence for
the G2 format decision, not adoption of RFX as the shipping project format.

`EXP-001A` now supplies the real no-argument startup path for that trial. The
native Qt Project Launcher reads engine-owned presets and submits one typed
creation request; Core creates stable IDs and a validated empty Revision 1,
while the desktop adapter transactionally materializes `Project.rfx`, the
project lock, Media root and project-local Agent Skill. The source is committed
last, reopened through the normal compiler/authority path and activated as an
empty Canvas/Timeline. macOS Core, sanitized and physical Visual lanes pass;
the owner still needs to exercise the native folder picker and the first
external-agent Revision 2 edit before the experiment can inform G2.

By explicit product-owner request, the 2026-08-07 visual-authoring review now
has a proposed, linked decision and delivery package: non-authoritative research
draft `RESEARCH-VA-001`, proposed hierarchy/compositing `RFC-0002`, proposed
architecture candidate `ARCH-VA-001`, and the detailed seven-package G2 plan.
The review moves one bounded LayerGroup proof into G2 to prevent the current
flat Layer/Timeline experiment from becoming the shipping schema. MP-001 remains
the only delivery-order authority; G2 is still planned, neither RFC is accepted,
and broad materials/FX, media and advanced Glass/Motion Blur remain in their
later gated scopes.

After the product owner instructed implementation, bounded pre-G2 `EXP-002`
materialized the first hierarchy slice without activating G2. Portable Core now
owns typed Layer/Group/root identity, validates one parent/range/reference/cycle
rules and evaluates parent transforms into immutable world matrices. Canonical
experimental writing emits RFX2 while RFX1 remains readable. The Timeline shows
the six-part Hero fixture as one collapsed Group row with engine-snapshot
drill-down, and the Agent CLI describes exact Group children/root order. macOS
Core, Studio, Graphics and full Visual lanes pass; the physical app opened the
30-second grouped project and presented through Metal/Skia. Group isolation,
opacity/masks/FX, Precomp, broad G3 features and all Windows qualification remain
explicitly unsupported or not-run. Owner drill-down/motion evaluation and the
formal RFC-0002 decision are still pending.

After the owner instructed continued implementation, bounded pre-G2 `EXP-003`
delivered the first typed visual-property UI transaction without activating G2.
Timeline selection now addresses one stable Layer/Group ID; Inspector projects
Position, Anchor, Scale, Rotation and Opacity from the accepted snapshot; and
one `SetVisualTransformCommand` passes through Core RevisionAuthority before the
same accepted revision reaches Runtime, canonical Project.rfx persistence and
all Studio projections. Invalid transforms and unknown IDs retain
Last-Known-Good. macOS Studio and Visual lanes pass; physical owner observation,
the general property registry/ChangeSet system and Windows evidence remain
pending. This is not broad Layer authoring or FX completion.

By further owner instruction on 2026-08-08, bounded pre-G3 `EXP-004` and
`EXP-005` now provide the first testable modern visual-style slice. A portable
22-descriptor Registry and Core-owned BG/SHP/TXT presets allow an empty project
to create real full-duration Layers. Shape state supports solid, ordered
multi-stop linear and radial fills, border and four bounded Layer blend modes.
Layer-local ordered rounded-rectangle masks and Gaussian Blur/Drop Shadow/Glow
are accepted through typed CAS commands, persisted in experimental RFX3 and
consumed by the macOS Metal/Skia path. RFX1/RFX2 stay readable, and newly
created project-local Agent instructions describe the RFX3 contract. Automated
Core and Visual lanes pass. The owner visual/Agent evaluation is now recorded:
EXP-004 and EXP-005 require revision rather than acceptance.

Owner review `EV-VA-0001` found that the implemented LayerGroup mechanism works
for the seven-part Subscribe component, but the Agent authored a composite
Background as independent root Layers and approximated animated Glow with
duplicate Text Layers. It also confirmed that Composition dimensions are real
while pixel-true TextBox, baseline/logical/ink/effect/world measurement and
typed node alignment are not yet implemented. `EXP-006A` now implements and
tests atomic GroupNodes/ReparentNodes/AddEffect through the shared command
authority, topology postconditions, capability-based rejection for unavailable
Align/animated-FX requests, advisory semantic lint and new-workspace Agent
guardrails. `EXP-006B` adds the bounded portable TextBox/paragraph schema,
qualified packaged Font identity, RFX4 migration, explicit `parent_px` versus
`local_px` transform vocabulary and one Registry digest shared by RFX,
Inspector, project lock and generated Agent catalog. `EXP-006C` now adds one
backend-neutral TextLayoutPort/result, Skia HarfBuzz/ICU implementation, exact
layout/logical/ink/clipped/baseline and geometry/mask/effect/world bounds, and
one Skia-enabled CLI measurement path sharing the preview layout-engine digest.
Missing system/packaged Font resolution fails closed without fallback. Core,
sanitized, Studio and macOS Visual lanes pass. `EXP-006D` now admits one-shot
atomic AlignNodes through the same Core/Application revision authority. It
measures geometry/logical/ink bounds from one exact-time evaluated scene,
inverse-transforms the world delta through the subject parent, preserves
animation-curve shape and rechecks a 0.25 px postcondition before publication.
Accepted authored Transforms survive canonical RFX save/reopen while derived
metrics remain out of project truth. `EXP-006E` now projects Mask/FX/Transform-
animation lanes beneath their owning Timeline Layer and adds read-only measured
bounds plus typed exact-time AlignNodes controls to Inspector. Runtime supplies
time, Core supplies hierarchy/measurement semantics and QML computes no offset.
`EXP-006F` now exposes JSON outline/inspect/measure/capabilities/validate/lint/
diff over the accepted portable snapshot. Its typed Group/AddGlow/Align commits
construct the same Core commands used by Application clients, enforce revision
CAS and LKG, then publish canonical Project.rfx through a platform-selected
atomic file adapter for Studio revalidation. Parent paths, Timeline rows, exact
ranges, semantic ownership, measured bounds and Font/layout digests are derived
from shared Core services. New project Skills receive a Registry-bound generated
command catalog and fail-closed recipes. `EXP-006G` now adds the repository-owned
1080x1920@60 sanitized Reels fixture with one Background Group, one Title Layer
with owner-local Shadow/Glow and one Subscribe Group. Automated save/reopen,
arbitrary seek, Skia measurement and UI/Agent semantic-digest parity all pass.
The product owner visually accepted this sanitized fixture on 2026-08-08, so
EXP-006 is closed as bounded pre-G2 evidence. This does not activate G2 or pull
general animated FX from G3.

This does not complete G3 or the original unlimited-creation vision. Glass
needs an explicit Backdrop pass; paper/noise/textures need admitted source and
procedural descriptors; Group masks/FX need isolated group compositing; and
Precomposition needs a multi-Composition reference/time-map contract. Those
capabilities remain unimplemented rather than being approximated. Windows
physical evidence remains not-run.

## Read next

1. `docs/plans/stages/G0-foundation/PLAN.md`
2. `docs/architecture/INVARIANTS.md`
3. `docs/product/PRODUCT_CONTRACT.md`

## Next actions

1. Run the checked-in `XPF-WP01A` project and `XPF-WP02B` RenderPlan receipts
   plus the `XPF-WP03A` exact layout receipt under real MSVC when a Windows
   runner is available. Materialize pinned Windows Skia, build the defined
   `windows-visual` lane and execute `XPF-WP05` D3D12/DXGI physical tests;
   never add semantic draw, text, color, mask or FX logic to Windows sources.
2. Preserve the checked-in canonical project, command and RenderPlan receipts.
   They remain prerequisites for a truthful Mac-create -> Windows-open parity
   claim and do not activate G2 or G3.
3. Preserve `G1-WP03` as passed bounded Apple evidence. Do not extend its
   elementary-stream scheduler into container demux or call it product Video
   Layer playback; those contracts enter G4 after the intervening gates.
4. Preserve `G1-WP05` as passed bounded macOS evidence. Continue `G1-WP06`
   for reproducible local development packaging and clean-machine launch/remove
   evidence without redistribution or a Qt entitlement claim after the bounded
   renderer extraction is green.
5. Keep the accepted EXP-006F/G results bounded as pre-G2 evidence. Formal MCP,
   ChangeSet envelopes, time/render probes and broader recipes remain G2-WP06;
   do not claim that this experimental CLI closes that proposed work package.
   Do not modify the owner's external Reels workspace.
6. Keep every new semantic/media contract portable and define the matching
   Windows lane/fixture; native code may enter Apple/Windows adapters only.
7. Retain an automated physical `G1-WP01` sleep/wake receipt when convenient;
   it is deferred, not silently converted into lab-qualified evidence.
8. When a Windows x64 runner/device becomes available, run G0-WP03, create the
   repository-local Windows Skia lock, then execute G1-WP02 D3D12/DXGI proof.
9. Complete G0-WP05 and the G0/G1 cross-platform exit reviews only after the
   missing Windows receipts exist.
10. Keep the experimental project schema bounded until G2 formally decides
   persistence, migrations, journals, ChangeSets and save/reopen semantics.
11. At formal G2 admission, review RFC-0001 and RFC-0002 with
   EXP-001/EXP-001A/EXP-002/EXP-006
   evidence, record accepted outcomes as ADRs, and activate no G2 work package
   until its entry evidence is satisfied.


## Do not repeat

- Do not turn the research draft into an implementation plan.
- Do not reuse, search for, or copy any external Skia checkout or machine cache;
  only the ReFusion-local clean bootstrap path is admitted.
- Do not reopen or expand the passed bounded macOS `G1-WP03` hardware-media
  proof into G4 product media while G0 Windows evidence is pending.
- Do not repeat Qt/Skia/media/font intake research; use ADR-0005 through ADR-0007.
- Do not reopen the Qt licensing lane without a superseding ADR.
- Do not request or claim Qt Commercial SDK/entitlement evidence during G0/G1;
  preserve the fail-closed release gate for the later redistributable RC.
- Do not re-implement the Application Host boundary or the macOS Skia lock; use
  source commit `24d5946e442de09ac9ccc798f9e7aeedeee04502`.
- Do not interpret macOS runtime success as Windows/iOS/Android qualification;
  their state remains `not-run` until platform evidence exists.
- Do not call the 30-second Shape/Text project a decoded video or stable project
  format; actual Video/Audio import and transport remain G4.
- Do not call the bounded G1-WP03 all-IDR single-frame decode proof MP4 import,
  a production decoder queue, Video Layer playback, Timeline seek qualification
  or G4 media completion.
- Do not call the Timeline transport control decoded-video playback or complete
  G4 transport; it currently drives the bounded Shape/Text walking Composition.
- Do not call `Project.rfx` adopted or shipping-ready from EXP-001; owner agent-
  authoring evaluation and the formal G2 format decision are still pending.
- Do not bypass the admitted EXP-006F typed commit surface with anchor guesses
  or duplicate-Layer FX. A published file candidate still requires Studio
  revalidation before it is accepted by the live application.
- Do not approximate unsupported animated FX by duplicating Layers. Reject the
  request with a typed capability diagnostic and retain Last-Known-Good.
