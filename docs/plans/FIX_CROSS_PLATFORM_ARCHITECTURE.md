---
id: PLAN-XPLAT-FIX-001
kind: cross-cutting-remediation-plan
status: active
version: 22
master_plan: MP-001
policy: ARCH-XPLAT-001
owner_role: principal-cross-platform-architecture
last_verified: 2026-08-09
audit_baseline: repository-read-only-audit-2026-08-08
---

# Fix Cross-Platform Architecture

## Authority and scope

This plan records the remediation required to make ReFusion one semantic
product across macOS, Windows, iOS and Android. It is linked to `MP-001`; it is
not a second master plan, does not activate a later gate, and does not waive the
evidence required by an existing work package.

The owner requirement is exact:

> A project authored on macOS must open on Windows with the same project IDs,
> composition, time, hierarchy, properties, animation, masks, FX and visual
> intent. Feature development occurs once. Platform differences start only at
> native GPU/media/window adapters: Metal, D3D12 or Vulkan.

This plan covers the currently implemented Shape/Text/Group experiment and all
future visual capabilities. It does not call currently missing Image, Video,
Audio, SVG, Adjustment, Precomposition, Glass/Backdrop, Motion Blur, plugin or
export functionality complete.

## Non-negotiable target contract

The one admitted route is:

```text
Portable Project Package
  Project.rfx + stable IDs + exact time + relative content-addressed assets
                         |
                         v
Portable Core compile / validate / command / revision authority
                         |
                         v
Immutable Evaluated Composition + backend-neutral VisualRenderPlan
                         |
                         v
Shared Skia Scene Compositor and Text Layout
  one ordering, one mask/FX/blend/color meaning, one diagnostics model
                         |
            +------------+-------------+
            |                          |
            v                          v
   Preview target                Offline/export target
            |                          |
            +------------+-------------+
                         |
       +-----------------+------------------+
       |                 |                  |
       v                 v                  v
 Metal context/target  D3D12 context/target  Vulkan context/target
 macOS + iOS           Windows               Android
       |                 |                  |
       v                 v                  v
 Native present/sync/import only; no semantic reimplementation
```

Core, project state, commands, property descriptors, evaluation, render-plan
lowering, text rules, compositing order and feature diagnostics must not contain
Qt, Skia-native handles, Objective-C, Win32, D3D, Metal or Vulkan types.

Backend adapters may only:

- own or borrow the qualified native device, queue and synchronization objects;
- create the Skia backend context and wrap a native render target;
- import native hardware-video surfaces without CPU video pixels;
- submit, synchronize and present;
- report typed capabilities, failures and evidence.

They must not redefine fill, gradient, text, alignment, mask, blend, FX,
animation, hierarchy, color or project semantics.

## Audit method and classification

The 2026-08-08 audit inspected Core, Application, Runtime, RFX persistence,
Studio composition selection, Skia rendering/text, Metal/D3D platform code,
CMake presets, CI, current tests and governing documents. The architecture
checker also passed its current bounded rule set: 75 inspected source files and
zero forbidden-boundary findings. That result proves only the rules it checks;
it does not prove renderer or pixel parity.

Definitions:

- **Correct**: one portable semantic implementation exists for its declared
  bounded capability and no contrary repository evidence was found.
- **Partial**: a portable contract exists, but execution, determinism or
  cross-platform evidence is incomplete.
- **Wrong / blocking**: current placement or behavior can cause platforms to
  redefine semantics or produce silently divergent project truth.
- **Missing**: no implementation/evidence exists; absence is not a failure if
  the capability is still outside its admitted gate, but it may not be claimed.

## Baseline classification

| Area | Classification | Current evidence and consequence |
|---|---|---|
| Stable IDs, exact integer/rational time, Canvas dimensions, transforms | **Correct, bounded** | Portable definitions and evaluation live in `src/core`; no platform/native type is project truth. |
| Project validation, CAS revision activation and Last-Known-Good | **Correct, bounded** | One Core authority validates before publishing; Qt and native filesystem operations remain adapters. |
| Atomic semantic + Runtime visual activation | **Wrong / blocking** | Studio currently accepts some Core/UI revisions before Runtime activation; a later Runtime failure can leave Canvas behind the accepted UI/Timeline revision. A prepared bundle must pass before atomic publication. |
| UI-command / Agent-command authority | **Correct, bounded** | Both submit to the same Core command path; current Glow and Align parity fixtures exercise the same authority. |
| RFX grammar and parser | **Correct, bounded** | Text grammar, ordered arrays and `from_chars` are portable for the current Shape/Text/Group subset. |
| Canonical RFX/JSON numeric output | **Wrong / blocking** | Default-locale streams and locale-sensitive character helpers can produce different bytes/digests across hosts. |
| UTF-8 and Unicode normalization policy | **Missing** | RFX/Agent JSON accept raw bytes without a documented validation, NUL/control or normalization contract. |
| Transferable project package | **Partial** | Current `Project.rfx` transfers the experimental Shape/Text/Group state, but there is no stable asset catalog, relative media/font identity, migration contract or package manifest. |
| `.refusion/agent-context.json` | **Correct only as generated state** | Absolute host paths are allowed only because Studio regenerates this file. It is never portable project truth and must be ignored/rebuilt on another host. |
| Shapes, solid/linear/radial backgrounds, stroke and rounded corners | **Partial** | Portable Core descriptions exist, but the sole real renderer is Metal and no cross-backend golden exists. Background is correctly a Shape/Group recipe, not platform code. |
| Transform animation | **Partial** | Evaluation is portable for the bounded scalar set; property-specific animated value ranges and cross-toolchain canonical quantization are incomplete. |
| Layer groups | **Partial** | Portable pass-through hierarchy exists. Group isolation, group opacity/masks/FX and Precomposition are not implemented. |
| Rounded-rectangle layer masks | **Partial** | Portable descriptor and Metal execution exist. Inverted-mask measurement bounds do not yet describe the same result as rendering. |
| Gaussian Blur, Drop Shadow and Glow | **Partial** | Portable project structs exist. Execution/order is currently inside Metal code; Glow is a bounded zero-offset shadow construction, not a complete generalized Glow system. |
| Blend modes Normal/Multiply/Screen/Overlay | **Partial** | Portable enum exists, but mapping and compositing policy are implemented only inside the Metal translation unit. |
| Effect/mask descriptor registry | **Partial / structural gap** | General visual properties have a portable registry, but effect/mask parameters still require independent switches across validation, RFX and rendering. |
| Text schema and TextBox alignment intent | **Correct, bounded** | Direction, horizontal/vertical alignment, wrap, overflow, padding and metrics contracts are portable. |
| Exact text rendering and logical/ink alignment | **Wrong for qualification** | System font managers and locally installed font bytes differ; packaged font resolution currently fails closed. The current engine digest cannot prove identical font input. |
| General multilingual wrapping | **Partial** | Current bounded wrapping contains simple space-based segmentation; it is not yet the admitted Unicode line-breaking policy. |
| Shared Skia scene compositor | **Wrong / blocking** | Fill, gradient, blend, mask, Shape/Text draw and FX lowering are inside `SkiaGpuContextsMetal.mm`; a Windows port would duplicate semantics. |
| macOS Metal device/presenter/Skia path | **Correct, bounded macOS evidence** | The current walking viewport runs on the engine-owned Metal path. This is not Windows/mobile qualification. |
| Windows D3D12 device authority | **Correct, bounded definition** | A hardware adapter/device/queue service exists and rejects software adapters; there is no DXGI presenter or Skia D3D context yet. |
| Windows Canvas/Studio rendering | **Missing / blocking Desktop parity** | Windows Skia configuration fails closed and Studio selects `NullRuntimeComposition` outside Apple. |
| iOS Metal and Android Vulkan lanes | **Missing** | No product adapter/presenter or current preset/canary qualifies them. Mobile remains a G1 contract-canary and G9 product lane. |
| Color/compositing contract | **Partial / blocking parity** | Current Metal target is BGRA8 sRGB, but working space, premultiplication, gradient interpolation, filter edges and output transfer are not accepted as one portable policy. |
| Preview/export semantic parity | **Missing** | No export renderer consumes a shared render plan yet. |
| Cross-backend visual goldens | **Missing** | Existing tests prove submission/layout bounds, not Metal-vs-D3D12-vs-Vulkan pixels. |
| Image/Video/Audio/SVG/Adjustment/Precomp/Glass/Motion Blur/plugins | **Missing by gate** | These capabilities remain later-gate work and must enter through this plan's admission rule rather than a platform-specific shortcut. |

## Blocking findings with repository evidence

### XPF-F00 — Accepted revision and active Canvas can diverge

`apps/studio/ProjectLiveReloadController.cpp` currently observes an already
accepted UI revision and then calls `runtime_->validate_candidate` and
`runtime_->accept_candidate`. A Runtime failure is logged after Core acceptance.
The file-reload path performs a separate Runtime validation, accepts through
Core, and then calls Runtime activation again. `StudioRuntimeComposition` is
therefore acting as a second admission authority after or beside Core.

Replace this with `PreparedVisualRevision` and one Application-owned two-phase
candidate preparation/atomic publication boundary. Timeline, Inspector, Canvas,
Console and persistence must receive one `AcceptedRevisionBundle` carrying the
same project/revision/evaluation/capability stamp. This is the highest integrity
prerequisite; extracting Metal drawing alone cannot fix mixed-revision state.

### XPF-F01 — Visual semantics are embedded in Metal

`src/adapters/skia/SkiaGpuContextsMetal.mm` currently contains the common
effect stack, blend mapping, gradient construction, masks, Shape/Text drawing
and compositing order as well as Metal context/target code. The corresponding
Skia CMake file deliberately fails on Windows, and Studio selects the live
visual runtime only on Apple.

This is the highest architectural risk. Copying `draw_project` into D3D or
Vulkan would create multiple render meanings and is prohibited. It must be
extracted without changing the already observed macOS output.

### XPF-F02 — Font input is not deterministic

`src/adapters/skia/SkiaTextLayout.cpp` selects CoreText, DirectWrite or the
Android font manager. `packaged_asset` is present in the Core contract but has
no byte resolver, so it fails with `RFX-FONT-ASSET-RESOLUTION-001`. Current
fixtures use `system_family("Arial")`, whose bytes, glyph selection, metrics and
availability are not a portable input. The layout digest does not contain the
actual font asset digest or the full shaping configuration.

No cross-platform text or alignment qualification may use a system font.
System fonts may remain an explicitly unqualified author convenience with
fail-closed relink diagnostics.

### XPF-F03 — Project bytes are not fully host-independent

Parsing uses locale-independent numeric conversion, but canonical serialization
and some digests/JSON output use default-locale streams or locale-sensitive
character functions. `AlignNodes` can also commit measured floating-point
coordinates derived from platform font metrics and math without a canonical
subpixel quantization policy.

The same accepted command must produce the same canonical project bytes and
semantic digest under AppleClang and MSVC, including when the host locale is
Arabic or uses a decimal comma.

### XPF-F04 — Native target contracts are too Metal-shaped

The portable runtime target/device envelopes currently expose only integer-like
device, queue and texture handles. D3D12 and especially Vulkan require resource
state, fences, queue-family/layout and lifetime synchronization that must not be
guessed or pushed into Core.

Replace the raw common handle assumption with an adapter-owned opaque target
lease/capability interface. Native types and synchronization stay in the
backend; common Runtime sees stable target identity, dimensions, format,
generation and typed readiness/failure only.

### XPF-F05 — Color meaning is under-specified

RGBA8 project colors and a current sRGB Metal target are insufficient to prove
the same result across APIs. The accepted contract must define input color
encoding, premultiplied-alpha policy, blend/filter working space, gradient
interpolation, filter edge/tile behavior and output transfer. HDR can remain a
separate later profile; it must not silently change the SDR profile.

### XPF-F06 — Cross-platform evidence is absent, not failed

macOS has physical runtime evidence. Windows visual/media, iOS and Android are
`not-run` or missing, not passed. Exact bit-for-bit floating-point pixels across
GPU vendors are not a professional promise. ReFusion requires identical
project/render semantics plus calibrated image tolerances and separate
performance budgets per qualified device profile.

## Required target components

### Portable project and resource contract

- stable project/composition/node/effect/mask/asset IDs;
- exact frame/rational-time truth and a declared subpixel coordinate grid;
- locale-independent canonical number formatting and semantic hashing;
- validated UTF-8 with an explicit preserve/normalization policy;
- project-relative, content-addressed assets and immutable asset digests;
- schema version, capability dependencies and deterministic migrations;
- no absolute host path, native handle or platform font as portable truth;
- generated `.refusion` session/diagnostic/context state remains disposable.

### Backend-neutral visual render plan

Core evaluation must lower an immutable accepted revision at an exact time into
one `VisualRenderPlan`. It records the ordered drawing/compositing operations,
resolved properties, world transforms, masks, isolation boundaries, effects,
color profile, font/asset digests and an `EvaluationStamp`. The plan contains no
native GPU handle and produces a deterministic semantic digest.

### Shared Skia scene compositor

A common C++ Skia compositor executes the same `VisualRenderPlan` for every
Skia backend. It owns:

- Shape/Text/Image draw semantics admitted by the current gate;
- fill/gradient/stroke/corner behavior;
- mask ordering and inversion;
- blend and group isolation rules;
- FX order, parameter lowering and filter-edge policy;
- text layout invocation and diagnostic propagation;
- preview/export operation parity.

It must compile as ordinary portable C++; it must not live in `.mm`, Win32/D3D
or Vulkan adapter files.

### Thin native backend bridges

- **Metal**: device/context creation, CAMetalLayer/native texture wrap,
  synchronization and presentation for macOS/iOS.
- **D3D12**: hardware adapter/device/queue, DXGI swapchain/target wrap,
  resource-state/fence ownership and presentation for Windows.
- **Vulkan**: instance/device/queue-family, image/layout/semaphore/fence and
  native presentation for Android.

Each bridge advertises typed capabilities before revision activation and fails
closed. WARP/software GPU, CPU project-pixel bridges, hidden software video
decode, Qt rendering/media ownership and silent feature degradation are banned.

### Deterministic text path

- pin approved font files, licenses and SHA-256 digests in official intake;
- resolve a project font asset to the same bytes on every platform;
- construct the typeface from those bytes, never from local font discovery;
- pin Skia, HarfBuzz and ICU revisions/options used by the qualified profile;
- include font bytes, face/style/variation and shaping configuration in the
  layout digest;
- qualify Latin, Arabic, RTL, mixed direction, diacritics, wrapping, baseline,
  logical bounds and ink bounds;
- keep geometric alignment postconditions at or tighter than 0.25 px while
  canonicalizing committed authored coordinates.

### Capability certification

The existing property registry remains the semantic seed. Add one generated or
single-source capability certification view—never parallel feature meanings—so
every capability records, per required platform/profile:

```text
defined -> compiled -> run -> qualified
```

An effect is not complete because a struct and one shader/filter exist. The
same descriptor must drive validation, persistence/migration, Inspector/Agent
introspection, animation/type rules, render-plan lowering, shared Skia
execution, diagnostics, preview/export and conformance fixtures.

## Normative implementation blueprint

This section is normative for the remediation. The names may be refined during
implementation only if the ownership boundaries and one-source rules remain
identical. Any proposal that introduces a second project evaluator, a second FX
meaning or platform-specific scene drawing requires an ADR that supersedes this
plan and `ARCH-XPLAT-001` before code is merged.

### One authoritative command-to-frame route

```text
UI Command -----+
                +--> Application Command Authority
Agent Command --+              |
                               v
                      Candidate Revision
                               |
                               v
                  Semantic + runtime preparation
                               |
                               v
                    PreparedVisualRevision
                               |
                               v
              Atomic AcceptedRevisionBundle publish
                               |
                               v
                  Portable exact-time evaluation
                               |
                               v
                       VisualRenderPlan
                               |
                               v
                 Shared SkiaSceneCompositor
                               |
            +------------------+------------------+
            |                  |                  |
            v                  v                  v
      Metal target       D3D12 target       Vulkan target
      macOS / iOS          Windows             Android
```

Timeline, Canvas, Inspector and Console are projections of the same accepted
revision. They never own project mutation, animation time, draw order, FX
meaning, native GPU resources or fallback policy. A file/CLI/MCP Agent edit and
an equivalent UI edit must enter the same Application command and validation
surface and publish the same semantic digest.

### Atomic candidate preparation and publication

The current prototype can accept a Core revision and only afterwards discover
that Runtime cannot activate it. The final route forbids that split-brain state.

Application owns one candidate-admission coordinator:

```text
Candidate command/file
  -> Core semantic validation and CAS preconditions
  -> resolve assets/fonts/effect/plugin capabilities
  -> evaluator and backend-neutral render-program compilation checks
  -> prepare required Runtime resources without publishing them
  -> PreparedVisualRevision
       {candidate snapshot, compiled render program, capability digest,
        asset/font/plugin digests, admission receipts, diagnostics}
  -> atomic AcceptedRevisionBundle publication
  -> Timeline + Inspector + Canvas + Console observe the same stamp
  -> exact ProjectTime sample + EvaluationStamp -> VisualRenderPlan per frame
```

If any preparation step fails, no accepted revision changes and Last-Known-Good
remains active. Device loss after a valid publication is a typed presentation
failure/recovery event; it may not make the UI publish another semantic state.

`RevisionAuthority` remains the sole legal Core revision authority. Application
coordinates preparation through Runtime ports and publishes one immutable
accepted bundle; Studio, file watchers and Skia contexts never perform a second
post-accept admission.

### Planned source ownership

The target layout follows the existing dependency boundaries rather than
creating a new parallel engine root:

```text
src/core/
  include/refusion/core/visual/
    EffectDescriptor.hpp          # portable ID/version/ports/units/defaults
    MaskDescriptor.hpp            # portable mask meaning
    ColorContract.hpp             # portable accepted color profile identity
  VisualPropertyRegistry.cpp      # one semantic property vocabulary
  EffectRegistry.cpp              # one effect/mask descriptor vocabulary

src/application/
  VisualCommands.cpp              # UI/Agent commands through one authority
  VisualCapabilityAdmission.cpp   # fail-closed pre-revision admission

src/runtime/render/
  include/refusion/runtime/render/
    VisualRenderPlan.hpp           # immutable backend-neutral operations
    RenderOperation.hpp            # typed ordered operation variant
    RenderPlanCompiler.hpp         # accepted revision -> exact-time plan
  VisualRenderPlan.cpp
  RenderPlanCompiler.cpp

src/adapters/skia/
  SkiaSceneCompositor.hpp          # consumes only VisualRenderPlan + target port
  SkiaSceneCompositor.cpp          # common Shape/Text/Image/composite semantics
  SkiaEffectExecutor.cpp           # common Blur/Shadow/Glow/SkSL/pass execution
  SkiaTextLayout.cpp               # common byte-backed shaping/layout
  SkiaColorPipeline.cpp            # common color/alpha/filter policy
  SkiaGpuContextMetal.mm           # Skia Metal context/target binding only
  SkiaGpuContextD3D12.cpp          # Skia D3D12 context/target binding only
  SkiaGpuContextVulkan.cpp         # Skia Vulkan context/target binding only

src/platform/apple/metal/
  MetalGpuDeviceService.mm         # native device/queue/fence authority
  MetalViewportPresenter.mm        # CAMetalLayer acquire/present only

src/platform/windows/d3d12/
  D3D12GpuDeviceService.cpp        # native device/queue/fence authority
  DxgiViewportPresenter.cpp        # HWND/DXGI acquire/present only

src/platform/android/vulkan/
  VulkanGpuDeviceService.cpp       # native instance/device/queue authority
  VulkanViewportPresenter.cpp      # ANativeWindow acquire/present only

apps/studio/
  InspectorProjection.*            # descriptor-driven view and commands only
  TimelineProjection.*             # accepted-revision projection only
  CanvasHost.*                     # native viewport host, never renderer owner
```

`src/core` remains standard C++ without Skia. `src/runtime/render` owns
evaluation-to-plan orchestration without native GPU types. `src/adapters/skia`
is one replaceable technology binding and may use portable Skia APIs, but it may
not create a competing physical device/queue. `src/platform` owns native
device/window/surface/synchronization details and contains no project or FX
semantics.

### VisualRenderPlan legal boundary

The exact C++ API is implementation work, but the legal information boundary is
fixed:

```cpp
struct VisualRenderPlan final {
  ProjectId project_id;
  RevisionId revision;
  CompositionId composition_id;
  ProjectTime time;
  EvaluationStamp evaluation;
  ColorProfileId color_profile;
  std::vector<RenderOperation> operations;
  RenderPlanDigest digest;
};
```

The ordered operation vocabulary begins with the admitted subset and expands
only through a versioned descriptor:

```text
BeginIsolation / EndIsolation
PushTransform / PopTransform
PushMask / PopMask
DrawShape / DrawText / DrawImage
ApplyEffect
Composite
```

The plan stores resolved values, stable IDs, asset/font digests, operation
order, isolation boundaries and the accepted color profile. It never stores a
`SkCanvas`, `SkSurface`, Metal texture, `ID3D12Resource`, `VkImage`, Qt object,
window handle or host filesystem path.

The plan is immutable after publication. Preview and export consume the same
operation meaning. A backend capability failure rejects presentation or the
candidate revision according to the typed admission policy; it never rewrites
the plan or silently removes an operation.

### One descriptor funnel for every FX and property

Every new visual capability starts from one portable, versioned descriptor. A
representative Glow descriptor has this semantic shape:

```text
id: refusion.fx.glow
version: 1
input_port: color_image
output_port: color_image
parameters:
  radius_px: non-negative float, animatable
  intensity: non-negative float, animatable
  color: linear RGBA, animatable
  opacity: ratio [0, 1], animatable
color_policy: accepted SDR working space
edge_policy: explicit transparent/clamp rule
```

That one descriptor must feed this complete route:

```text
Descriptor/Registry
  -> Core validation and typed animation ranges
  -> canonical Project.rfx persistence and migration
  -> Inspector controls and Agent/CLI/MCP introspection
  -> exact-time evaluation
  -> VisualRenderPlan lowering
  -> shared SkiaEffectExecutor / SkSL / shared pass graph
  -> diagnostics and capability certification
  -> preview/export and cross-platform fixtures
```

There is no platform-specific FX registry. Metal, D3D12 and Vulkan bridges are
not permitted to switch on `EffectId`, `LayerKind`, masks, blend modes or text
alignment.

Standard Skia image/color/blend filters are preferred for their admitted
semantics. A custom shader is authored once in SkSL/`SkRuntimeEffect` when that
model is sufficient. A multipass effect such as professional Motion Blur is
described once as a backend-neutral ordered pass graph. Native MSL/HLSL/GLSL
variants are not the feature source of truth; if an unavoidable native kernel
is ever proposed, it requires a common semantic reference, identical descriptor
and conformance implementation, explicit capability gating and an ADR before
admission.

### Thin backend lease contract

The common compositor receives a qualified target lease rather than raw native
objects. The portable view exposes only stable identity, backend kind, extent,
format/profile, device generation and readiness. Backend-private storage owns:

- Metal texture/command-buffer/fence lifetime;
- D3D12 resource state, queue, fence and DXGI back-buffer lifetime;
- Vulkan image view/layout, queue family, semaphore and fence lifetime.

The backend may acquire, wrap, synchronize and present a target. It may not
change draw order, choose a different effect, substitute a CPU path, use WARP,
create a second GPU authority or allow Qt to own Canvas/media rendering.

### Portable project package target

The shipping package contract is planned as:

```text
MyProject/
  Project.rfx                    # sole legal semantic project document
  Assets/
    Fonts/<asset-id>/...
    Images/<asset-id>/...
    Video/<asset-id>/...
    Audio/<asset-id>/...
  .refusion/
    Cache/                       # derived and rebuildable
    Diagnostics/                 # local evidence
    agent-context.json           # generated host-local paths only
    session.lock                 # host-local session state
```

`Project.rfx` references assets by stable `AssetId`, project-relative logical
location and content digest. Absolute macOS/Windows paths and native handles are
never portable truth. Moving the folder to another platform regenerates
`.refusion` host metadata and preserves project/revision/composition/layer/
effect IDs, exact frames, hierarchy, properties and asset digests.

### Desktop v1 color baseline

Before cross-backend visual qualification, one accepted SDR profile must define
all of the following, without backend defaults:

```text
authored color encoding
SDR Rec.709/sRGB interpretation
premultiplied-alpha rules
blend/filter working space
gradient interpolation space
filter edge/tile/crop behavior
preview target format and output transfer
```

HDR/wide-gamut profiles may be added later as separate versioned capabilities;
they may not silently change Desktop v1 SDR project meaning.

### Strict feature merge gate

Branch ownership and two-host synchronization are governed by the accepted
[`Canonical two-host Git workflow`](../architecture/CROSS_PLATFORM_POLICY.md#canonical-two-host-git-workflow).
In particular, shared Qt/QML UI and engine changes use a feature/fix branch and
are promoted to `main` only after review; platform evidence branches are not
parallel product trunks. Any affected evidence is rerun against the same
promoted `main` commit on macOS and Windows.

A visual capability may merge as an experiment only if it is explicitly marked
non-shipping and fail-closed elsewhere. It may be called cross-platform complete
only when the following chain is green for every required profile:

```text
defined -> canonical -> compiled -> physically run -> semantically matched
        -> visual tolerance passed -> performance qualified
```

The merge review must answer all of these questions with repository evidence:

1. Is there exactly one portable descriptor and one project meaning?
2. Do UI and Agent commands publish the same accepted revision and digest?
3. Does one RenderPlan compiler define ordering, time and values?
4. Does one common Skia compositor execute the capability?
5. Are native files free of Layer/FX/text/mask semantics?
6. Are font/assets/color inputs identical and content-addressed?
7. Do macOS and Windows compile the same fixture, and have required physical
   profiles passed their semantic and calibrated visual evidence?
8. Do unsupported devices fail before activation/presentation without project
   corruption or a hidden fallback?

Any `no`, missing receipt or `not-run` state prevents the corresponding
cross-platform/shipping claim. It does not justify a platform-specific copy.

## Normative FX and plugin portability contract

ReFusion is plugin-ready through the same internal Registry used by built-ins;
it is not an in-process public native-ABI product. “Unlimited extensibility”
means a new contribution can enter one versioned semantic pipeline without
forking project truth or backend meaning. It does not mean arbitrary code may
run inside the UI or main render process.

### Extensible project representation

The bounded experimental `LayerEffectParameters` variant for Blur, Shadow and
Glow is not the final extension representation. The target is a versioned,
registry-resolved instance:

```text
EffectInstance
  instance_id
  descriptor_id
  descriptor_version
  enabled
  ordered typed parameter values
  optional preserved unknown payload for safe round-trip
```

One `EffectDescriptor` defines ports, parameter IDs/types/units/defaults/ranges,
animation rules, color/alpha/edge policy, deterministic bounds, resource budget,
migrations, Inspector/Agent metadata and RenderPlan lowering identity.

RFX parsing/writing, migrations, Core validation, CLI/MCP introspection,
Inspector controls and documentation must derive from the Registry/contract
catalog or from one code-generated source. Adding a contribution may not add a
new hand-written vocabulary switch in Core, Studio, QML, CLI and every backend.

Existing RFX1-RFX4 Blur/Shadow/Glow syntax remains readable through explicit
migrations into versioned descriptor instances. Unknown or missing extension
instances round-trip without data loss as `UnresolvedContribution`; they do not
render/export through an approximation and never cause project corruption.

### Plugin tiers

| Tier | Contract | Platform rule |
|---|---|---|
| 0 — Built-in contribution | Engine-owned descriptor, evaluator/lowering and shared Skia/SkSL/pass implementation | Same source and conformance corpus on every required product lane. Built-ins obey the plugin contract even before a public SDK exists. |
| 1 — Declarative preset/graph | Signed/versioned descriptors, graph, parameters and assets only; no executable native code | Preferred universally portable tier. The same package is interpretable by the common engine on macOS, Windows, iOS and Android, subject to declared capability budgets. |
| 2 — Certified SkSL contribution | One pinned/signed SkSL source or admitted intermediate representation with digest, declared resources and bounded execution | Skia compiles it for Metal/D3D12/Vulkan. Not arbitrary public SkSL in v1; validation, loop/resource limits, deterministic color/edge policy and visual qualification are mandatory. |
| 3 — Isolated worker | Sandboxed WASM/worker contribution for import, analysis or generation outside the realtime render path | No raw project/GPU/filesystem pointers. Communicates through versioned messages, quotas, cancellation and deterministic artifacts. Runtime/render state remains engine-owned. |
| 4 — Native C++ extension | Post-v1 source SDK with a stable C ABI/versioned IPC boundary inside an out-of-process signed Plugin Host | C++ is authoring convenience, not a cross-OS C++ binary ABI. Build one artifact per admitted target triple from the same semantic source; crash/hang/capability escalation is contained and rollbackable. |

iOS and Android do not admit downloaded executable native plugins. A native
contribution may reach mobile only when packaged with the application under
store policy and qualified as part of that product build. Declarative/assets
and other allowed content remain the normal mobile extension route.

Target ownership does not place plugin code inside Studio or a backend:

```text
contracts/extensions/              # manifest, descriptor, state, migration schemas
src/core/visual/EffectRegistry     # portable identity/types/validation only
src/runtime/extensions/            # admission, graph/pass lowering, budgets
src/adapters/skia/extensions/      # common certified SkSL/pass execution
services/plugin_host/              # post-v1 native C ABI + versioned IPC isolation
sdk/extensions/                    # generated headers/tools after G10 admission
packaging/plugins/                 # per-platform signing/bundling/store policy
```

The Plugin Host never links Studio internals or publishes project revisions. It
receives bounded immutable messages/artifacts; Application remains the only
candidate admission/publication coordinator.

### Plugin package identity

A plugin package manifest must eventually contain, at minimum:

```text
plugin_id + publisher_id + plugin_version
contribution descriptor IDs + descriptor versions
project/state schema versions + migrations
engine contract range + plugin-host ABI/IPC version
target triples and per-artifact SHA-256 digests
portable descriptor/SkSL/asset digests
permissions/capability requirements/resource budgets
color/time/preview/export behavior declarations
signature/provenance/revocation/rollback metadata
```

The project stores stable plugin/contribution identity, versioned state and
content digests—not a `.dll`, `.dylib`, filesystem path or native handle.

### Cross-platform plugin admission

A contribution is called cross-platform only if:

1. one descriptor/version and one project meaning exist;
2. UI and Agent use generated/introspected controls and commands;
3. exact-time evaluation and one RenderPlan lowering are shared;
4. Tier 0/1/2 render execution uses the common compositor/pass graph;
5. native artifacts, when applicable, expose the same stable C ABI/IPC contract
   and contribution IDs rather than independent semantics;
6. the same canonical state, migrations, semantic digest, diagnostics and
   reference corpus pass every required platform/profile;
7. missing, unsigned, revoked, incompatible, crashing or over-budget plugins
   fail closed as unresolved/quarantined contributions, retain Last-Known-Good,
   preserve source state and never substitute another FX;
8. preview and export use the same contribution meaning;
9. packaging/signing/SBOM/license/store-policy evidence exists for each shipped
   target;
10. compile, run, visual tolerance, performance and recovery states are recorded
    separately—one platform or one binary never proves another.

Binary identity is not the promise. Semantic identity, versioned contracts,
qualified results and failure containment are the promise.

Public plugin SDK/API details remain G10 work. G2/G3 must nevertheless build
all built-in FX through this internal descriptor/registry shape so G10 extends
the engine instead of replacing a hard-coded variant system.

## Governance, active guardrail and ratchet

This document is an active architecture guardrail, not background reading.
`docs/status/CURRENT.md` identifies `PLAN-XPLAT-FIX-001` in
`active_guardrails`. Root onboarding documents link here instead of copying its
details. Any Agent or engineer touching visual state, FX, plugins, project
persistence, RenderPlan, Skia, Studio Inspector/Timeline, GPU targets, fonts,
color or export must read this guardrail before changing code.

`XPF-WP00A` extends `architecture-check` with a ratcheted source-boundary check.
It admits only the frozen debt recorded at implementation time and enforces:

1. native Metal/D3D12/Vulkan targets may not include project documents or
   mention project/composition/layer/mask/effect/blend/text/shape/evaluator
   semantic symbols;
2. common RenderPlan/Skia compositor targets may contain no Qt, native SDK
   headers or `__APPLE__`, `_WIN32` or `__ANDROID__` semantic branches;
3. Studio/QML may not gain an FX-specific default, validation rule or renderer
   switch; descriptor-driven projection is the only admitted UI path;
4. common Skia targets may not link PlatformMedia or a platform presenter;
5. Runtime/Application may depend on portable capability/lease ports, while a
   native bridge may not depend back on project semantics;
6. `rfdev.py context` and `docs-doctor` must load/validate every active
   guardrail ID so a resumed Agent cannot silently miss this plan.

Known violations are frozen by exact rule/file/symbol and occurrence count in
`contracts/architecture/cross-platform-visual-boundary-exceptions.json`. Its
baseline digest is pinned in `rfdev.py`; wildcards, new allowances, changed
signatures and count growth fail. Active allowances must be reduced or removed
as code is cleaned. The immutable baseline remains only as historical audit;
it is not an active exception. `XPF-WP02` exits with zero native visual-semantic
allowances.

## Code relocation ledger — double-check 2026-08-08

This ledger is the second, symbol-level inspection of the current source. It is
the authoritative cleanup map for `XPF-WP02` through `XPF-WP06`. Line numbers
refer to the audit baseline and may move; the named symbols and responsibilities
are the durable identifiers.

Relocation actions:

- **MOVE**: behavior is common and moves intact to the shared target.
- **SPLIT**: a current class/file owns both common and native responsibilities;
  separate them and leave only the listed native part behind.
- **REPLACE**: the current contract is prototype-shaped and is superseded by a
  portable/opaque contract before adding another backend.
- **GENERATE**: duplicated UI/Agent vocabulary is replaced by the registry.
- **QUARANTINE**: bounded G1 proof code remains test-only until its later-gate
  replacement exists; it must not become product architecture.
- **KEEP**: code is correctly native or correctly shared and must not be moved.

### A. Metal/Skia monolith: exact split

Current file:
`src/adapters/skia/SkiaGpuContextsMetal.mm`.

| Current symbol/responsibility | Current location | Action | Correct owner/target | Required result |
|---|---:|---|---|---|
| `to_sk_color` | 76-80 | **MOVE** | `src/adapters/skia/SkiaColorPipeline.cpp` | Converts the accepted portable color/alpha contract once for all Skia backends. It must not infer a backend color policy. |
| `make_effect_stack` | 82-111 | **MOVE + redesign** | `src/adapters/skia/SkiaEffectExecutor.cpp` | Consumes typed `ApplyEffect` operations/descriptors. Blur, Shadow and Glow order/edge/color behavior becomes common; no native backend switches on effect type. |
| `to_sk_blend_mode` | 113-125 | **MOVE** | `src/adapters/skia/SkiaSceneCompositor.cpp` or a common `SkiaBlendLowering.cpp` | One portable BlendMode-to-Skia mapping used by Metal, D3D12, Vulkan, preview and export. |
| `make_gradient_shader` | 127-143 | **MOVE** | `src/adapters/skia/SkiaColorPipeline.cpp` | One explicit tile/interpolation/color-space implementation; remove `SkGradient::Interpolation{}` as an implicit platform/build default. |
| `draw_project` Canvas viewport scaling | 145-152 | **SPLIT** | `src/runtime/render/ViewportMapping.cpp` -> `VisualRenderPlan`; execution in `SkiaSceneCompositor.cpp` | Aspect-fit/fill, zoom, pan and pixel mapping are common view semantics. Independent `scale_x/scale_y` must not silently distort a Composition. |
| exact-time `core::evaluate_visual_layers` call | 153-155 | **MOVE ownership** | `src/runtime/render/RenderPlanCompiler.cpp` | Reuse the existing `core::EvaluatedVisualScene`; evaluate once outside the GPU context, then lower the accepted revision to an immutable plan. |
| world-transform application | 156-162 | **MOVE** | `SkiaSceneCompositor.cpp` consuming `PushTransform`/`PopTransform` | Native context files never inspect `AffineTransform2D`. |
| effect isolation/`saveLayer(nullptr)` and blend | 164-172 | **REPLACE**, then move | bounded common RenderPlan isolation operations + `SkiaEffectExecutor.cpp` | The compiler supplies deterministic isolation bounds, effect expansion, crop/tile/filter-edge policy and a resource budget. Unbounded backend-local layers are forbidden. |
| rounded mask clipping/inversion | 173-191 | **MOVE** | common `PushMask` lowering/execution | Mask order, antialiasing, inversion and edge policy become descriptor-defined. |
| Shape fill/gradient/stroke/corners | 192-239 | **MOVE** | `SkiaSceneCompositor.cpp` | `ShapeLayerContent` disappears from `.mm`; one Shape executor serves all targets. |
| Text clipping and cached blob drawing | 240-260 | **MOVE** | `SkiaSceneCompositor.cpp` + shared `SkiaTextBlobStore` | The compositor draws only the already accepted text layout; backend files never reshape or interpret alignment/overflow. |
| Layer iteration/composite order | 153-266 | **MOVE** | `RenderPlanCompiler.cpp` + `SkiaSceneCompositor.cpp` | One operation order and one plan digest; no per-backend scene traversal. |
| `draw_decoded_video_fixture` | 269-278 | **QUARANTINE then remove** | G1 test helper only; G4 `DrawVideoFrame` RenderOperation later | The current decoded frame is drawn as an overlay after project Layers and is not a real Video Layer. It must not survive into product compositing. |
| `make_decoded_video_image` high-level Rec.709/YUVA choice | 281-332 | **SPLIT** | common media color/plane descriptor + `SkiaMetalVideoImageBridge.mm` | Plane/color intent comes from the qualified common media profile; Metal texture borrowing/wrapping stays native. |
| `make_decoded_video_image` `id<MTLTexture>`/`GrMtlTextureInfo` work | 285-315 | **KEEP after move** | `src/adapters/skia/SkiaMetalVideoImageBridge.mm` | This is legitimate Metal-to-Skia binding; the Windows equivalent wraps D3D surfaces without duplicating color or Layer semantics. |
| `SkiaGpuContexts::create` composition validation/storage | 343-357, 415-427 | **MOVE/REMOVE** | Application accepts revision; Runtime owns `VisualRenderPlan` publication | A GPU context accepts a device/backend configuration, never a mutable `CompositionSnapshot`. Duplicate validation inside the renderer is removed. |
| Metal Ganesh context creation | 359-370 | **KEEP after split** | `src/adapters/skia/SkiaGpuContextMetal.mm` | Only engine-device borrowing and Skia Metal context creation remain. |
| Graphite context creation beside active Ganesh | 372-379 | **QUARANTINE/select** | independent probe target, or delete after its evidence is migrated | Do not create an unused second context in the product path. A qualified profile selects one execution engine; an experiment has its own target, tests and claim. |
| text-layout engine construction | 381 | **SPLIT/inject** | common `SkiaTextLayoutEngine` created by Runtime/adapter composition root | Context creation does not choose fonts or text semantics. |
| decoded queue ownership/import loop | 392-413 | **MOVE/SPLIT** | Runtime media selection + per-backend video image bridge | The GPU context must not own a PTS queue or eagerly import every decoded frame. It receives the exact selected frame/resource for the current plan. |
| `replace_composition` | 445-458 | **REMOVE/replace** | `RuntimeRenderSession::publish_plan(VisualRenderPlan)` | Backends receive immutable revision-stamped plans, not mutable project snapshots. |
| `selected_video_source_frame_index` | 460-469 | **MOVE** | Runtime media/presentation telemetry | Selected-source truth belongs to the exact media scheduler, not Skia Metal state. |
| Metal target validation/wrap | 475-511 | **KEEP after split** | `SkiaGpuContextMetal.mm` / Metal target lease bridge | Validate backend, device generation, format and ownership, then return a common Skia surface target. |
| `canvas.clear(SK_ColorBLACK)` | 513-514 | **MOVE** | RenderPlan/output profile clear operation | Canvas background/transparent clear is visual/export semantics, not a Metal default. |
| composition atomic-load and `draw_project` call | 515-531 | **REMOVE/replace** | common compositor executes an immutable plan supplied by Runtime | Metal context no longer knows `ProjectSnapshot`, `CompositionSnapshot`, Layer or FX. |
| PTS selection and video overlay draw | 533-565 | **MOVE/QUARANTINE** | Runtime media scheduler + future `DrawVideoFrame` operation | Exact frame selection is shared G4 behavior. The current G1 overlay remains bounded evidence only. |
| observability, flush, abandonment | 566-585 | **KEEP** | backend context/submission layer | GPU submission, context loss and backend telemetry are legitimate adapter responsibilities. |

After this split, `SkiaGpuContextMetal.mm` is forbidden from including
`ProjectDocument.hpp` or mentioning `LayerEffect`, `LayerMask`, `BlendMode`,
`ShapeLayerContent`, `TextLayerContent`, `CompositionSnapshot` or
`evaluate_visual_*`. An automated source-boundary check must enforce that list.

### B. Public Skia API split

Current file:
`src/adapters/skia/include/refusion/adapters/skia/SkiaGpuContexts.hpp`.

The current `SkiaGpuContexts` type combines device context, scene evaluator,
composition storage, text engine, video queue, renderer and telemetry. Replace
it with narrow responsibilities:

```text
SkiaBackendContext
  create from qualified BackendDeviceLease
  wrap BackendFrameTargetLease -> SkSurface target
  submit / flush / report loss

SkiaSceneCompositor
  execute(const VisualRenderPlan&, SkCanvas&)
  no native handles and no platform branch

SkiaTextLayoutEngine / SkiaTextBlobStore
  shape qualified font bytes once
  return accepted layout + stable cached draw resource

NativeVideoImageBridge
  import one exact selected native surface for one backend
  no PTS selection and no Layer ordering

RuntimeRenderSession
  own accepted revision, exact time, RenderPlan compilation/publication,
  media-frame selection and presentation request
```

The replacement `ViewportFrameRenderer` entry accepts a revision/evaluation-
stamped plan or plan lease. It does not accept a Composition and does not call
Core evaluation from a backend translation unit.

### C. Text layout cleanup

Current file: `src/adapters/skia/SkiaTextLayout.cpp`.

| Current responsibility | Current location | Action | Correct target/result |
|---|---:|---|---|
| CoreText/DirectWrite/Android includes and `make_platform_font_manager` branches | 15-21, 261-271 | **SPLIT** | `SkiaSystemFontProviderApple.mm`, `SkiaSystemFontProviderWindows.cpp`, `SkiaSystemFontProviderAndroid.cpp`; available only for explicitly unqualified author convenience. |
| qualified `packaged_asset` rejection | 419-425 | **REPLACE** | project `AssetId`/SHA-256 resolver supplies identical bytes; common Skia code creates `SkTypeface` from those bytes. |
| `exact_system_family` and local family lookup | 282-291, 426-437 | **KEEP only behind unqualified provider** | Never used by cross-platform fixtures, semantic qualification or shipping template defaults. |
| engine digest based only on Skia revision | 398-406 | **REPLACE** | digest includes Skia/HarfBuzz/ICU pins, font bytes, face/style/variation, line-break policy and shaping options. |
| letter spacing applied per glyph | 106-179 | **REPLACE in place** | deterministic Unicode grapheme/cluster spacing with an explicit direction policy; Arabic marks and ligatures are never separated by backend accident. |
| face construction fixes `Normal` and permits platform fallback | 431-440 | **REPLACE** | byte-backed face index, weight/style/width, variation axes, features, language, script and deterministic fallback-chain coverage are qualified inputs and digest fields. |
| ASCII-space word wrapping | 248-259, 321-383 | **REPLACE in place** | common deterministic ICU Unicode line-break segmentation; this is shared typography behavior, not platform code. |
| `draw_cached` on the layout engine | 561-579 | **SPLIT** | measurement/shaping remains the TextLayout port; cached Skia draw resources are exposed through the shared compositor/blob store without native backend knowledge. |

The common text implementation may remain a Skia adapter. What is forbidden is
allowing platform font discovery to define qualified glyphs, metrics, wrapping
or alignment. `SkiaTextLayoutInternal.hpp`, typeface/font-byte caches and
text-blob lifetime are part of this split: they move behind common qualified
font/blob-store interfaces and may not borrow an OS font manager implicitly.

### D. Runtime GPU/presentation contract cleanup

| Current contract | Current location | Action | Replacement |
|---|---:|---|---|
| `NativeHandles { device, command_queue }` | `GpuDeviceService.hpp:33-36` | **REPLACE** | adapter-owned `BackendDeviceLease` with opaque lifetime/capability access; Runtime sees identity/generation, while Metal/D3D/Vulkan private bridges obtain their typed native state safely. |
| `NativeFrameTarget { uintptr_t texture }` | `ViewportPresentation.hpp:51-60` | **REPLACE** | `BackendFrameTargetLease` carrying identity, extent, accepted format/profile, generation and opaque lifetime. Backend-private data carries D3D resource state/fence or Vulkan layout/queue-family/sync. |
| `FixtureFrame` and copied `PlaybackSpec`/presentation nanoseconds | `ViewportPresentation.hpp:62-83`, `ViewportPresentation.cpp:24-35,209-217` | **REPLACE** | `PresentationFrameRequest` carries exact `ProjectTime`, accepted project/revision/composition, clock epoch, device generation and `EvaluationStamp`/RenderPlan lease. Skia does not convert nanoseconds or choose a timebase; a frame counter is telemetry only. |
| `ViewportFrameRenderer::render(target, frame)` | `ViewportPresentation.hpp:150-158` | **REPLACE** | render a validated `VisualRenderPlanLease` into a qualified target lease; stale revision/device generation is rejected before submission. |

Replace raw `NativeViewportHost {uintptr_t}` use with a lifetime-bearing host
lease, or prove an equivalent attach/detach ownership contract. No host handle
may outlive the native view or enter project state, Agent context or persistent
diagnostics.

### E. Build-target cleanup

Current file: `src/adapters/skia/CMakeLists.txt`.

1. Build `SkiaRuntime`, deterministic text, RenderPlan executor,
   `SkiaSceneCompositor`, `SkiaEffectExecutor` and `SkiaColorPipeline` as common
   sources on every Graphics lane.
2. Move Metal context and Metal video-image bridge to a Metal backend target;
   add matching D3D12 and later Vulkan backend targets.
3. Remove the Windows `FATAL_ERROR` that makes the declared `windows-graphics`
   lane non-configurable. An incomplete backend must fail through an explicit
   capability/test target, not by pretending the common renderer cannot build.
4. Remove the common Skia adapter's direct Apple PlatformMedia link and
   `REFUSION_SKIA_APPLE_MEDIA` branch. Inject a `NativeVideoImageBridge` backend
   target instead.
5. Build Studio visual runtime when the generic GPU, presenter and Skia backend
   capabilities are present—not when `APPLE` is true.
6. Add architecture checks that configure/link every declared lane and reject
   project/Layer/FX symbols in native backend files.

Target dependency shape:

```text
ReFusion::RuntimeRenderPlan
            |
            v
ReFusion::SkiaCommonCompositor
            |
     +------+------+
     |             |
     v             v
SkiaMetalBackend  SkiaD3D12Backend  [SkiaVulkanBackend]
     |             |                       |
PlatformMetal   PlatformD3D12         PlatformVulkan
```

### F. Studio/UI duplication cleanup

These files are not native GPU backends, but they currently duplicate feature
vocabulary and would force every FX addition to touch UI-specific code.

| Current duplication | Current location | Action | Correct result |
|---|---:|---|---|
| hard-coded Blur/Shadow/Glow projection | `StudioBridge.cpp:380-420` | **GENERATE** | generic effect-instance projection from `EffectDescriptor` and typed parameter values. |
| UI creates FX structs/defaults and parses each parameter family | `StudioBridge.cpp:984-1110` | **GENERATE/replace** | `AddEffectCommand(descriptor_id)` obtains defaults from Core registry; generic typed `SetEffectParameterCommand` submits values. UI never constructs effect meaning. |
| hard-coded rounded-mask defaults/shape branching | `StudioBridge.cpp:1160+` | **GENERATE/replace** | Mask descriptor/command owns compatible default geometry; Inspector projects it. |
| effect/mask names in Timeline | `StudioTransportBridge.cpp:54-90, 139+` | **GENERATE** | display names and property rows come from registry/snapshot projection. |
| three FX buttons and per-kind QML controls | `Main.qml:1200-1344` | **GENERATE** | descriptor-driven effect catalog and generic editors selected by value kind/unit/range, not Effect ID. |
| Studio compile branch creates or omits Skia measurement | `main.cpp:10-12, 39-45` | **MOVE ownership** | Application/runtime session factory injects the qualified TextLayout service; Studio receives capability/diagnostic projections only. |
| Studio process constructs device, Skia context, presenter and render session | `VisualRuntimeComposition.cpp:21-40` | **MOVE orchestration** | Application/bootstrap `EngineVisualSessionFactory`; Studio holds a narrow session/viewport-host command interface. |
| Studio runtime stores/republishes Composition separately | `VisualRuntimeComposition.cpp:71-110` | **REPLACE** | accepted revision publication creates one Runtime plan; Studio observes the same snapshot projection. |
| UI revision is accepted before Runtime activation | `ProjectLiveReloadController.cpp:95-115` | **REPLACE** | Application `CandidateAdmissionCoordinator` prepares semantics, assets, capabilities and compiled render program, then publishes one `AcceptedRevisionBundle`; activation failure happens before acceptance. |
| file reload validates, accepts and activates through separate phases | `ProjectLiveReloadController.cpp:180-225` | **REPLACE** | file watcher/persistence adapter submits one candidate to the same coordinator used by UI/Agent; it is not an admission authority. |
| Studio owns candidate canvas/rate/duration validation | `StudioRuntimeComposition.hpp`, `VisualRuntimeComposition.cpp:55-81` | **MOVE/REMOVE** | portable Application capability admission; Studio cannot be a second validator or redefine the accepted profile. |
| `NullRuntimeComposition` selected outside Apple | `NullRuntimeComposition.cpp`, `apps/studio/CMakeLists.txt:156-166` | **REPLACE** | real generic visual runtime on qualified backends; headless/unavailable mode returns an explicit typed diagnostic and is never a silent shipping fallback. |
| `EngineViewportWindow` stores/traverses a Composition copy | `EngineViewportWindow.cpp:14-97,135-139` | **REDUCE** | QWindow owns host lifecycle/extent only; display metadata comes from immutable Studio projections, not a second composition copy. |

Timeline projection may read an accepted immutable snapshot. That is valid UI
projection. The cleanup removes duplicated defaults, validation, semantics and
mutable ownership—not legitimate read-only display formatting.

### G. Media proof quarantine and future cleanup

`src/platform/apple/media/AppleHardwareVideoDecoder.mm` is accepted only as the
bounded G1 hardware proof. Its VideoToolbox/CoreVideo/Metal operations are
correctly platform-native, but the following must not become the G4 production
contract:

| Current proof code | Action | G4 owner/result |
|---|---|---|
| `NalUnit`, `AnnexBStream`, `find_start_code`, `parse_annex_b` at 62-135 | **QUARANTINE then move/delete** | Common demux/compressed-sample adapter produces codec sample leases. Windows must not copy this parser into Media Foundation code. |
| direct `std::ifstream(request.source_path)` at 469+ | **REPLACE** | Project `AssetId` -> filesystem/source adapter -> opaque `CompressedSourceLease`; native Unicode paths never enter Runtime media semantics. |
| `make_avcc_sample` at 137-151 | **SPLIT** | Common compressed sample remains codec-canonical; any VideoToolbox packaging detail stays in the Apple decoder adapter. |
| Rec.709 attachment validation at 153-173 | **KEEP native validation** | Compare native output metadata against a common qualified `VideoColorProfile`; the platform does not choose the profile. |
| VideoToolbox session, CVPixelBuffer, CVMetalTextureCache and Metal plane leases | **KEEP** | Legitimate Apple hardware decode/import implementation behind portable Runtime media ports. |

This ledger does not activate G4 or expand the bounded decoder. It prevents the
temporary fixture parser/path/overlay from fossilizing into the Windows or
mobile product architecture.

### H. Files confirmed correctly placed

Do not “clean” platform code merely because it is platform-specific. The
following responsibilities are correct and remain native:

- `MetalGpuDeviceService.mm`: Metal device/queue identity, lifecycle and native
  lease creation;
- `D3D12GpuDeviceService.cpp`: hardware adapter selection, WARP rejection,
  D3D12 device/queue identity and lifecycle;
- `MetalViewportPresenter.mm`: NSView/CAMetalLayer attachment, resize,
  occlusion, drawable acquisition, command-buffer presentation and native
  device-loss telemetry;
- future `DxgiViewportPresenter.cpp`: HWND/DXGI swapchain, resource state/fence
  and presentation only;
- Apple VideoToolbox/CoreVideo and Windows Media Foundation native decode
  session/surface binding behind the same Runtime media contracts;
- `SkiaRuntime.cpp`: Skia initialization and build-capability reporting. These
  flags describe the build; they must not choose project semantics.

The rule is not “platform files must be empty.” The rule is “platform files own
native resource mechanics only.”

### I. Public contracts, build graph and test relocation

The cleanup includes interfaces and evidence, not only implementation files:

| Current surface/evidence | Action | Required destination/result |
|---|---|---|
| `SkiaGpuContexts.hpp` | **DELETE after split** | narrow `SkiaBackendContext`, common `SkiaSceneCompositor`, text/blob-store and native-video bridge interfaces; no umbrella object retaining old ownership. |
| `PlatformViewportPresenter.hpp`, `PlatformGpuDeviceService.hpp` | **REPLACE narrow raw envelopes** | lifetime-bearing device/host/target leases with backend-private typed state and portable identity/generation/capability projections. |
| `AppleMediaSurface.hpp` | **KEEP native, narrow** | Apple surface lease remains behind `NativeVideoImageBridge`; common compositor never links or includes it. |
| common Skia CMake target | **SPLIT dependencies** | depends on RuntimeRenderPlan and Skia only; no PlatformMedia, AppKit, Metal, D3D, Vulkan or Qt link edge. |
| `skia_fixture_renderer_test.mm` | **SPLIT** | backend-neutral operation/compositor tests plus a narrow Metal target-binding/submission test. |
| `skia_decoded_video_surface_test.mm` | **QUARANTINE** | bounded G1 native-surface proof; future G4 video-layer corpus replaces it. |
| Graphite readiness assertion in `skia_adapter_test.cpp` | **MOVE/REMOVE** | independent Graphite probe target if retained; product readiness asserts only the selected renderer. |
| presenter/observability tests | **MIGRATE** | drive lifetime-bearing leases, exact `EvaluationStamp`, stale revision/device rejection, detach and device-loss recovery. |

Architecture targets must enforce the dependency direction:

```text
Core semantics <- Application admission <- Runtime render program/plan
                                      |
                                      v
                            Skia common compositor
                                      |
                    +-----------------+-----------------+
                    v                 v                 v
              Metal binding      D3D12 binding     Vulkan binding
                    |                 |                 |
              native platform mechanics and presentation only
```

A native bridge depending on `ProjectDocument`, an Effect Registry, Studio or
QML is a boundary violation. A common compositor depending on a platform
presenter/media target is the reverse violation.

## Safe cleanup sequence

The cleanup is behavior-preserving and must not be performed as one destructive
rewrite:

1. **Characterize the current Metal output.** Freeze the current Reels fixture,
   operation order, text metrics, semantic digest, accepted diagnostics,
   presentation counters and test-only reference captures.
2. **Make candidate activation atomic.** Add `PreparedVisualRevision`, the
   exact revision/evaluation stamp and two-phase admission beside the current
   path. Prove a failed preparation cannot advance any UI/Canvas projection.
3. **Introduce the render program beside the old path.** Add
   `VisualRenderPlan`, plan digest and common compositor interfaces without
   switching Studio; produce dual-path plan/receipt evidence.
4. **Compile from the existing evaluator.** Lower the current
   `core::EvaluatedVisualScene` into the first RenderPlan; do not create a second
   animation/hierarchy evaluator.
5. **Extract the bounded common compositor.** Move color, gradient, blend,
   bounded effect isolation, mask, Shape, Text and FX execution into ordinary
   C++ common Skia files; run old/new A/B through the same Metal target.
6. **Resolve qualified font bytes and blob lifetime.** Split system-font
   convenience from packaged deterministic typography before using text as a
   parity oracle.
7. **Split native Metal mechanics and probes.** Move context/target wrapping,
   submission and native video-image import into thin Metal files; quarantine
   Graphite separately; remove project/Layer/FX symbols from native code.
8. **Switch Studio through the Application session factory.** Publish one
   accepted bundle, make the viewport host-only, and move revision/time/media
   ownership to Runtime. Preserve the G1 video proof only in its test harness.
9. **Replace raw handle contracts.** Introduce opaque lifetime-safe device,
   host and target leases, requalify detach/device-loss/stale-stamp tests, then
   use those ports for D3D12 and Vulkan.
10. **Make common code build on Windows.** Configure/compile/link the common
    RenderPlan/Skia/Text/FX targets under MSVC before adding D3D12 mechanics.
11. **Add D3D12 without semantic code.** Implement context/target/DXGI and run
    the same plan digest and visual corpus. A visual defect is fixed in common
    code and rerun on Metal, never patched only in D3D12.
12. **Make Studio descriptor-driven.** Replace hard-coded FX/mask names,
   defaults, parameter parsing and QML branches after the descriptor registry
   can supply the same functionality. UI continues to submit commands only.
13. **Admit mobile canaries.** Reuse the common targets; add only iOS Metal and
    Android Vulkan native mechanics. Physical product qualification remains G9.
14. **Delete compatibility scaffolding.** Remove the old monolithic
    `SkiaGpuContextsMetal.mm`, Apple-only Studio gate, silent Null runtime and
    proof-only paths only after their replacement tests are green and no
    production target references them.

### Cleanup exit checks

The cleanup is complete only when repository checks prove:

```text
native backend files mention 0 Project/Composition/Layer/Mask/Effect types
common compositor contains 0 Metal/D3D12/Vulkan/Qt platform branches
Studio/QML contains 0 built-in FX-specific defaults or validation branches
one accepted revision/time produces one RenderPlan digest
Metal and D3D12 execute that same plan and pass the same semantic corpus
unsupported capability produces one typed diagnostic and 0 silent fallbacks
```

Each relocation commit must keep macOS Core, Studio and Visual lanes green.
Windows common Graphics compile/link evidence is required as soon as the common
targets are extracted; physical Windows qualification remains separately
recorded and may not be inferred from compilation.

## Ordered remediation work packages

### XPF-WP00 — Audit baseline and claim freeze — complete

**Maps to:** MP-001 cross-platform policy and all active-gate merge reviews.

**Delivered:** this classification, the prohibited drift paths and the target
architecture. Until the later packages pass, current visuals are described as
bounded macOS evidence plus portable Core definitions—not as qualified Windows
or mobile rendering.

### XPF-WP00A — Active-guardrail enforcement and architecture ratchet — complete

**Maps to:** repository governance and the entry condition of XPF-WP02.

Delivered:

1. load and display `CURRENT.md` `active_guardrails` through `rfdev.py context`;
2. make `docs-doctor` reject unknown/missing active guardrail IDs;
3. add source/build dependency checks for native-semantic, common-platform and
   Studio FX-vocabulary violations defined in the governance section;
4. create one exact temporary exception manifest for the violations in this
   ledger, with owner, removal package and expiry and no wildcard support;
5. require every touched exception to shrink and prohibit adding an exception;
6. add negative fixtures proving each forbidden boundary fails the check.

Exit: repository automation catches a newly introduced backend FX switch,
platform branch in common compositor code, Studio default or reverse dependency.
The baseline violations remain visible only as exact, expiring debt assigned to
XPF-WP02/XPF-WP03/XPF-WP06. This package changes governance, not renderer
behavior.

Evidence: [`EVID-XPF-WP00A-2026-08-08`](../evidence/reviews/XPF-WP00A-architecture-ratchet.md).

### XPF-WP01 — Portable project conformance and canonical bytes

**Maps to:** G0-WP03/G0-WP05 hardening and G2 persistence admission.

Deliver:

1. locale-free numeric/JSON/digest formatting using a specified `to_chars`
   policy and explicit non-finite rejection;
2. ASCII identifier/case rules and validated UTF-8 string policy;
3. canonical subpixel quantization for authored coordinate commits;
4. a cross-toolchain `xplat-project-conformance` corpus containing canonical
   RFX bytes, snapshot/registry/render-plan digests and command outputs;
5. Mac-create -> Windows-open -> canonical-resave tests with stable IDs,
   revisions, exact frames, hierarchy, FX order and semantic digest;
6. copied-workspace tests proving `.refusion` state is regenerated locally.

Exit: AppleClang and MSVC produce the exact prescribed canonical bytes/digests
for the corpus under C, decimal-comma and Arabic host locales. iOS/Android
compile canaries parse the same corpus when their lanes enter.

#### Local implementation receipt — XPF-WP01A (2026-08-08)

The locally executable canonical-text slice is complete. Core now owns one
locale-free `to_chars` contract for finite binary64, unsigned and fixed-width
hexadecimal values; RFX, Agent JSON numeric values and project/registry/text/
RenderPlan receipts use that contract or an explicitly compatible fixed-six
schema spelling. Both IEEE zero encodings have one spelling and non-finite
values fail closed.

RFX and programmatic Composition admission now require preserved, well-formed
UTF-8; reject NUL, prohibited C0/C1 controls and DEL; and never silently apply
NFC/NFD. Project entities use a case-sensitive portable ASCII ID grammar that
cannot contain host path separators. Touched blank/case classification uses
explicit ASCII instead of the host locale.

The Arabic/Latin `xplat-visual-v1` corpus now has a checked-in project snapshot
and registry receipt. `refusion.xplat_project_conformance` proves canonical
serialize/recompile equality and identical bytes/digests under a synthetic
decimal-comma/grouping locale; RenderPlan receipts are also locale invariant.
The macOS Visual lane passes 44/44 tests.

This is not the WP01 exit: MSVC execution, canonical authored-coordinate
quantization, checked command-output receipts, Mac/Windows resave, copied
workspace regeneration and mobile canaries remain open.

Evidence: [`EVID-XPF-WP01A-LOCAL-2026-08-08`](../evidence/reviews/XPF-WP01A-local-canonical-project.md).

#### Local implementation receipt — XPF-WP01B (2026-08-08)

Core now owns a binary-exact `1/1024 px` commit grid for derived/UI-authored
pixel values. Typed Transform and numeric pixel-property commands quantize
before candidate validation, while `AlignNodes` quantizes translated base and
keyframed positions before its measured postcondition and Revision publication.
Explicit existing RFX literals, ratios, degrees and opacity are not silently
reinterpreted as pixel values.

The cross-toolchain corpus includes a command receipt for Transform, property
and rotated/nested geometry alignment under a hostile decimal-comma locale. It
binds the final Revision, canonical project digest/size and exact command output
coordinates, then requires canonical reopen equality.

Studio also treats `.refusion` as generated host-local state. A context that
proves relocation to another canonical project path permits replacement of the
copied target lock and regeneration of context/cache/journal/diagnostics. An
unknown or same-path active lock still fails closed. The integration fixture
copies a workspace while its source session remains active and proves both
source exclusion and destination regeneration. Agent context schema v2 stores
unsigned Revision values as decimal strings.

This closes the locally executable WP01B slice. WP01's formal exit still needs
the same project/command receipts under MSVC, real Mac/Windows canonical resave
and mobile compile canaries.

Evidence: [`EVID-XPF-WP01B-LOCAL-2026-08-08`](../evidence/reviews/XPF-WP01B-local-coordinates-workspace.md).

### XPF-WP02 — Shared VisualRenderPlan and Skia scene compositor

**Maps to:** G1-WP01 regression, G1-WP02 Windows visual proof, G2 evaluator and
G3 unified visual authoring.

**Relocation scope:** ledger sections A, B and E. Section A is the symbol-level
acceptance list; none of its common scene responsibilities may remain in the
Metal translation unit at exit.

Deliver:

1. Application-owned two-phase candidate preparation and one atomic
   `AcceptedRevisionBundle`, with failure-before-publication receipts;
2. immutable backend-neutral `VisualRenderPlan` plus deterministic digest and
   exact `EvaluationStamp`;
3. extract `draw_project`, fill, gradient, blend, masks, Shape/Text and FX
   lowering from `SkiaGpuContextsMetal.mm` into common C++;
4. accept one explicit SDR color/compositing/filter-edge profile and bounded
   effect-isolation/resource policy;
5. retain one semantic ordering for Canvas preview and future export;
6. compile the common executor in macOS and Windows Graphics lanes;
7. add operation-order fixtures for solid/gradients/stroke/corners, four blend
   modes, normal/inverted masks, Blur/Shadow/Glow permutations, hierarchy,
   transforms, animation and text.

Exit: the Metal adapter contains no project/FX/text semantics; the common plan
and executor reproduce the accepted macOS fixture with no regression and are
compilable for the Windows Skia profile. A rejected runtime candidate cannot
advance the accepted revision observed by Timeline, Inspector or Canvas.

#### Implementation receipt — XPF-WP02A (2026-08-08)

The first behavior-preserving implementation slice is complete:

- Application previews every typed command and invokes Runtime candidate
  preparation before the real authority commit. A rejected Runtime candidate
  leaves Core and Last-Known-Good unchanged; accepted publication occurs while
  the Application admission mutex excludes readers.
- Studio Runtime now implements `ProjectCandidateAdmissionPort`. Project file
  observation and UI result observers no longer validate or activate Runtime
  after Core acceptance. They only feed candidates or persist accepted truth.
- `RuntimeRender` owns immutable `VisualRenderProgram`, exact
  `EvaluationStamp`, backend-neutral `VisualRenderPlan`, deterministic semantic
  receipt and the sole lowering from Core's existing evaluated scene.
- Shape, Text cache drawing, solid/linear/radial fills, strokes, corners,
  transforms, Blend modes, ordered/inverted masks, Blur, Drop Shadow and Glow
  execution moved from `SkiaGpuContextsMetal.mm` to the shared
  `SkiaSceneCompositor.cpp`. Effect isolation now receives deterministic
  Core-derived local bounds instead of `saveLayer(nullptr)`.
- `SkiaCommon` and `SkiaMetalBackend` are distinct build targets. The common
  target has no native media link or platform branch; the Metal target owns
  device/context/target/video-surface binding and submission only. The former
  Windows CMake `FATAL_ERROR` was removed, so the common target can enter the
  Windows Graphics lane without a parallel renderer implementation.
- 19 active ratchet allowances were retired (all 17 native project-semantic
  signatures plus the two common PlatformMedia/definition leaks). Their frozen
  audit records remain immutable; reintroduction fails policy.
- macOS Visual builds and all 41 tests pass, including new RenderPlan digest/
  bounded-isolation coverage, Application Runtime rejection/LKG coverage,
  Studio live reload, physical Metal/Skia/media tests and repository policy.

This closes the macOS/common architectural relocation slice, not the complete
WP02 exit. Remaining WP02 qualification is: compile/link the common target
under MSVC in the real Windows Graphics lane, add the full operation-order and
calibrated pixel corpus, and record Windows state as evidence rather than
inferring it from macOS. XPF-WP03 packaged fonts also remains a prerequisite
for qualified text parity.

#### Local conformance receipt — XPF-WP02B (2026-08-08)

The repository now contains
`tests/fixtures/render-plan/xplat-visual-v1/Project.rfx` and immutable expected
RenderPlan receipts at frames 0, 30, 60 and 119. The fixture traverses the real
RFX compiler, Core evaluator and RenderPlan lowering and covers root/Group
ordering, transforms/animation, Solid/Linear/Radial/Text content, Normal/
Multiply/Screen/Overlay, normal and inverted Masks, strokes/corners and ordered
Blur -> Shadow -> Glow. The digest uses explicit little-endian primitive
encoding rather than host object bytes or C++ padding. Transport epoch remains
in `EvaluationStamp` for stale-frame rejection but does not redefine identical
visual semantics.

The Metal qualification test now renders that same fixture through
`SkiaCommon`, waits for completion on the engine queue and uses a CPU-visible
test target to verify opaque coverage, foreground/color/highlight coverage and
the reference background corner. This readback is restricted to the test
executable and does not alter the production zero-CPU-pixel route.

This completes the locally executable WP02 conformance slice. The exact corpus
is automatically included in every Core test lane, but its MSVC receipt remains
`not-run` until a Windows runner is available. Cross-backend calibrated pixel
comparison and D3D12/DXGI presentation remain WP02/05 exit work, not inferred
from the Metal result.

#### Reentrant projection-publication receipt — XPF-WP02C (2026-08-09)

The prepared revision contract now separates `commit_engine_state()` from
`publish_observer_projections()`. Application commits accepted Core and Runtime
engine state while holding admission exclusion, releases that mutex, and only
then permits Studio to reset models or emit Canvas/Timeline/Inspector signals.
This preserves one accepted engine bundle while allowing synchronous QML
bindings to read the new snapshot without self-deadlock. Recursive locking and
post-accept Runtime validation remain forbidden.

The regression fixture deliberately calls `active_snapshot()` from synchronous
projection publication and requires the newly committed Revision. The real
filesystem live-reload test, all 49 macOS Visual tests and all 28 sanitized Core
tests pass. The reproduced Agent-authored gradient project reopens at Revision
2 and produces its accepted journal/diagnostic receipts.

Evidence:
[`EVID-XPF-WP02C-REENTRANT-PROJECTION-2026-08-09`](../evidence/reviews/XPF-WP02C-reentrant-projection-publication.md).

### XPF-WP03 — Deterministic asset and font resolution

**Maps to:** G0 dependency/legal intake, G2 project package, G3 typography.

**Relocation scope:** ledger section C and the project-package contract. System
font providers remain isolated and explicitly unqualified.

Deliver:

1. approved packaged Latin/Arabic baseline fonts from official immutable
   sources with license notices and SHA-256;
2. project-relative `AssetId`/digest resolver and byte-backed Skia typeface;
3. qualified layout digest containing font bytes, face index, style/weight/
   width, variation axes, language/script/features, shaping and fallback inputs;
4. explicit unqualified-system-font and missing-asset diagnostics;
5. grapheme/cluster-safe spacing, deterministic Unicode line breaks and a
   byte-backed fallback chain with explicit coverage;
6. cross-platform multilingual metrics corpus.

Exit: qualified fixtures use no system font; glyph IDs, advances, line breaks,
baselines and logical/ink bounds match the declared deterministic policy on
AppleClang and MSVC. Alignment commands then persist the same canonical result.

#### Local implementation receipt — XPF-WP03A (2026-08-08)

The locally executable deterministic-font slice is implemented. Official
immutable Noto Sans 2.015 and Noto Sans Arabic 2.013 archives, selected font
members and OFL notices are SHA-256 pinned in `deps/manifest.lock.json` and are
materialized only through the repository bootstrap into ReFusion-local
`out/deps-src`. New workspaces transactionally receive byte-identical fonts,
license notices and a digest-bound `Assets/Fonts/catalog.lock`.

Core now exposes a path-free `FontAssetResolverPort` and a portable SHA-256
content-digest implementation. The Studio filesystem adapter resolves only
project-relative `Assets/Fonts/<AssetId>/font.ttf`, validates containment and
content digest, and returns owned bytes. Skia constructs its qualified
`SkTypeface` from those exact bytes through its embedded FreeType font manager;
CoreText, DirectWrite and Android system providers are isolated behind a
separate explicitly unqualified adapter target and cannot qualify a project.

Qualified layout uses HarfBuzz shaping, ICU line breaking, cluster-safe spacing
and disabled hinting. Its receipt binds the exact font digest, face index,
weight/width/slant, fixed variation/language/script/features policy, primary-
only fallback policy and layout-engine configuration. The checked-in Latin,
Arabic, RTL, mixed-direction, diacritic and wrapping corpus fixes line metrics,
baselines, glyph IDs and glyph positions. Missing assets, digest mismatch,
family mismatch and unavailable system fonts fail closed with typed diagnostics.

The macOS Visual lane rebuilds cleanly and passes 47/47 tests; the architecture
ratchet inspects 97 source files with zero problems and no new allowance. This
is a local receipt, not formal WP03 exit: the exact corpus still needs its MSVC
receipt, and a future general multi-font fallback-chain schema must remain
explicit rather than silently consulting the host. The currently qualified
policy is deliberately `primary-only` and is fully recorded in the digest.

Evidence: [`EVID-XPF-WP03A-LOCAL-2026-08-08`](../evidence/reviews/XPF-WP03A-local-deterministic-fonts.md).

### XPF-WP04 — Backend target/context lease boundary

**Maps to:** G1-WP01/G1-WP02 presenters and G1-WP07 mobile canaries.

**Relocation scope:** ledger section D and the native portions retained by
sections A/H.

Deliver adapter-owned device and frame-target leases that express lifetime,
format, generation and readiness without leaking native types or assuming a
single `texture` integer is enough. Document D3D12 resource-state/fence and
Vulkan layout/queue-family/synchronization ownership. Retain one engine GPU
authority and reject cross-adapter targets before submission.

Exit: the common renderer does not branch on Metal/D3D/Vulkan and cannot access
native window/media types; each backend proves correct target lifetime and
fail-closed device-loss behavior.

#### Local implementation receipt — XPF-WP04A (2026-08-08)

The portable contract and macOS Metal implementation are complete. Raw
`NativeHandles`, integer-texture `NativeFrameTarget` and raw
`NativeViewportHost` contracts were removed. `BackendDeviceLease` now carries
the full immutable device identity plus type-erased, shared-lifetime device and
submission-queue state. `BackendFrameTargetLease` carries the full device
identity, stable target ID, extent, pixel format and one opaque native
target/synchronization lifetime. `NativeViewportHostLease` is acquired and
retained by the platform adapter before Runtime sees it.

`FixtureFrame` was replaced by `PresentationFrameRequest`. A request carries
the exact Core ProjectTime, transport epoch, telemetry-only request sequence,
full device identity and an immutable `VisualRenderProgram` lease. The program
itself binds accepted project/revision/composition identity. Skia no longer
stores or publishes a second mutable render-program pointer: it evaluates only
the coherent program/time/epoch lease in the request, then rejects an invalid,
stale-generation or cross-adapter request before wrapping/submitting a target.
Studio publishes prepared programs through `ViewportRenderSession`, so an
in-flight request retains either the complete previous program or the complete
new program rather than observing a torn update.

Metal retains NSView and MTLTexture objects for the complete host/target lease
lifetime. Platform media, presenter and Skia Metal code borrow opaque native
state only after validating the full backend/adapter/generation identity. A new
architecture rule forbids those backend-private accessors from Core,
Application, Runtime, Studio and common Skia sources.

The macOS Visual lane rebuilds and passes 47/47 tests; stale target and stale
request generations fail closed, and architecture-check reports 97 inspected
source files with zero problems. This is local WP04A evidence, not formal WP04
exit: D3D12 resource-state/fence ownership and Vulkan image-layout/queue-family/
synchronization leases still require their platform implementations and build/
device receipts.

Evidence: [`EVID-XPF-WP04A-LOCAL-2026-08-08`](../evidence/reviews/XPF-WP04A-local-backend-leases.md).

### XPF-WP05 — Windows D3D12/DXGI visual parity

**Maps to:** existing G1-WP02. This package does not replace or bypass it.

Deliver the repository-local Windows Skia materialization, D3D12 Skia context,
DXGI presenter, target wrap/sync path, Studio live composition selection and
physical hardware receipts. No semantic drawing code may be copied into the
Windows backend.

Exit: the same portable corpus opens and renders on macOS and Windows; semantic
digests match exactly; visual goldens pass calibrated tolerance; GPU/device
counters prove no WARP, CPU project-pixel transfer or silent fallback.

#### Source implementation receipt — XPF-WP05A (2026-08-08)

The Windows source route is now defined. `D3D12GpuDeviceService` continues to
select one non-software adapter/device/direct queue. A D3D12 Skia binding
verifies the exact LUID, creates Ganesh from that borrowed device/queue and
wraps only same-device BGRA8 targets. `DxgiViewportPresenter` owns an HWND
flip-discard swapchain, three back buffers, per-buffer fence values, resize
quiescence, occlusion probes and present/device-loss handling. Windows Studio
selects the same live visual composition through the `windows-visual` lane.

No project, Layer, Mask, Blend or FX semantics were added to either native
binding. Metal and D3D12 now call `execute_visual_render_program`; the common
translation unit alone performs exact-time evaluation and invokes the common
Skia compositor. The architecture check fails if a native renderer includes
Core project/compiler headers or owns any current visual-effect token.

The macOS Visual build remains green at 47/47 and architecture-check inspects
101 source files with zero problems. This does **not** close WP05: MSVC compile,
Windows Skia materialization/link closure, a named physical D3D12 run,
10,000-frame/device-loss receipts and calibrated cross-backend pixels all
remain `not-run`.

Evidence: [`EVID-XPF-WP05A-SOURCE-2026-08-08`](../evidence/reviews/XPF-WP05A-windows-source-wiring.md).

### XPF-WP06 — FX/property admission and conformance matrix

**Maps to:** G2 schema/Inspector/Agent parity and G3 unified visual authoring.

**Relocation scope:** ledger section F. No built-in FX-specific default,
validation or parameter switch remains in Studio/QML after this package.

Deliver a single-source descriptor/admission pipeline for every visual
property, mask and effect. Record platform/profile state as defined, compiled,
run and qualified. Repair inverted-mask measurement semantics and enforce
typed/ranged animated values before adding animated FX.

Exit: a new admitted FX requires no platform renderer fork, no independent UI
switch and no duplicate project syntax; unsupported combinations fail before
revision activation with the same diagnostic on UI/CLI/MCP.

#### Local implementation receipt — XPF-WP06A (2026-08-08)

`VisualContributionRegistry` now owns all current Mask/FX descriptor IDs,
capability IDs, typed parameters, units, defaults, ranges, validation,
Inspector/Agent metadata and generated project-local documentation. Studio QML
and Timeline projection are descriptor-driven; no concrete Blur/Shadow/Glow or
rounded-mask UI switch remains. Property-specific scale/opacity keyframe ranges
are validated before publication.

RFX5 now binds canonical project bytes to the contribution Registry digest and
serializes every current Mask/FX through one ordered typed-parameter codec.
RFX1–RFX4 remain explicit migration inputs; unknown or missing RFX5 parameters
fail closed. `VisualRenderPlan` stores each lowered contribution's descriptor
ID, capability ID and schema version and includes that identity in its semantic
digest. The common Skia compositor consumes that Registry-bound plan; native
bindings still receive no project or effect vocabulary.

The machine-readable capability matrix names the macOS Metal, Windows D3D12,
iOS Metal canary and Android Vulkan canary profiles and records `defined`,
`compiled`, `physically_run`, `semantically_matched`,
`visual_tolerance_passed`, `performance_qualified` and `qualified` separately.
Architecture checking rejects missing Registry capabilities and evidence-state
jumps. Descriptor-addressed Application commands removed the final generic
project-stack types from Studio, so active visual-boundary debt fell from 26
occurrences to zero. The macOS Visual lane passes 48/48 and architecture-check
inspects 105 source files with zero problems.

This completes the locally executable WP06 architecture slice. Formal WP06
remains open only for the required non-macOS compile/run/semantic/visual/
performance receipts and final per-profile capability admission evidence. No
platform is promoted to qualified by this local result.

Evidence: [`EVID-XPF-WP06A-LOCAL-2026-08-08`](../evidence/reviews/XPF-WP06A-local-contribution-admission.md).

### XPF-WP07 — iOS Metal and Android Vulkan contract canaries

**Maps to:** existing G1-WP07 and later G9 productization.

Deliver toolchain presets/workflows, platform adapter stubs, shared Core/RFX/
RenderPlan/common-Skia compilation, and fail-closed capability tests. Runtime
product qualification remains G9 and must not delay Desktop v1.

Exit: mobile canaries cannot redefine project or renderer semantics. Actual
device state remains `not-run` until physical evidence exists.

**WP07 local receipt (2026-08-09):** separate Core and Graphics configure/build
workflows now exist for iPhoneOS arm64 and Android arm64-v8a/API 28. Both
Graphics lanes compile the same Core/RFX/RenderPlan/SkiaCommon closure; native
canaries own leases/mechanics only and fail closed with
`RFX-*-CANARY-NOT-PRODUCT`. CI installs the exact pinned Android NDK and builds
both official pinned mobile Skia profiles before CMake admission. Repository
policy rejects missing lanes, iOS AppKit leakage, missing fail-closed markers,
native semantics and a second unqualified product render context.

The local iPhoneOS environment built both prescribed workflows successfully,
including the verified official `ios-arm64-metal-canary` Skia artifact. The
capability matrix therefore records iOS as `defined=true, compiled=true` and
keeps every runtime/semantic/visual/performance/qualification state false.
Android is `defined=true, compiled=false`: this host has no Android SDK/NDK, so
no local compile is fabricated. Formal WP07 remains open until the Android
Core/Graphics CI or equivalent official-NDK receipt passes.

Evidence:
[`EVID-XPF-WP07A-IOS-COMPILE-2026-08-08`](../evidence/reviews/XPF-WP07A-ios-compile-canary.md),
[`EVID-XPF-WP07B-ANDROID-SOURCE-2026-08-08`](../evidence/reviews/XPF-WP07B-android-source-canary.md).

### XPF-WP08 — Preview/export and full profile qualification

**Maps to:** G4 export seed, G5 creator loop and G6 release hardening.

Deliver preview and export consumers of the same `VisualRenderPlan`, reference
image captures in test-only qualification paths, calibrated pixel metrics,
performance budgets per device/profile, and recovery/device-loss receipts.

Exit: supported project features produce the same operation/semantic digest in
preview and export. Mac/Windows Desktop v1 profiles pass semantic, visual and
performance evidence independently; mobile follows its G9 matrix.

**WP08 local receipt (2026-08-09):** Runtime now exposes one
consumer-neutral `prepare_visual_output_frame(...)` contract for interactive
Preview and future Offline Export. It lowers the accepted program at exact
ProjectTime through the same `VisualRenderPlan`; the consumer identity cannot
alter evaluation, operation order or semantic digest. Production Skia Preview
enters through this contract. A fail-closed parity receipt requires one Preview
and one Offline Export sample with identical project, Revision, Composition,
ProjectTime, canvas and semantic digest while allowing independent scheduler
epochs.

The local contract fixture passes, the full macOS Visual lane passes 49/49, and
the updated iPhoneOS arm64 Graphics workflow compiles the same Runtime contract
and shared Skia consumer. Architecture checking covers 110 sources with zero
problems and zero visual-boundary debt; documentation and negative policy tests
also pass. This completes only the locally executable WP08A semantic slice. It
does not implement or claim G4 production export, encoded-output parity,
Windows physical rendering, calibrated cross-backend pixels, performance,
recovery or device-loss qualification.

Evidence:
[`EVID-XPF-WP08A-LOCAL-2026-08-09`](../evidence/reviews/XPF-WP08A-local-preview-export-semantic-parity.md).

**WP08 bounded macOS profile receipt (2026-08-09):** the physical Metal
presenter completed a corrected 10,000-frame loop with 10,000 submissions and
zero CPU pixel maps/uploads, GPU readbacks or unattributed copies. The joint
GPU-observability path retained zero copies/conversions, a 2,089,728-byte peak,
89,967,083 ns maximum fence latency, nominal thermal state, one injected
device-loss event and zero stale-generation resources accepted. The first soak
attempt proved the exact-time guard by rejecting a test timestamp outside the
30-second Composition; the fixture was corrected to loop legal ProjectTime
without weakening Runtime. All 49 macOS Visual tests pass afterward, and the
Core contract closure passes 28/28 under ASan/UBSan.

This receipt is limited to the named macOS interactive Preview tier. It does
not qualify production Export, sustained thermal behavior, Windows, mobile or
cross-backend pixels and therefore does not close WP08.

Evidence:
[`EVID-XPF-WP08B-MACOS-2026-08-09`](../evidence/reviews/XPF-WP08B-local-macos-metal-profile.md).

## Dependency and execution order

```text
XPF-WP00 complete
      |
      v
XPF-WP00A guardrail ratchet
      |
      +--> XPF-WP01 canonical project --------+
      +--> XPF-WP02 atomic publication + ------+--> XPF-WP04 target leases
      |             shared render plan/kernel  |             |
      +--> XPF-WP03 deterministic fonts -------+             v
                                                     XPF-WP05 Windows visual
                                                              |
                                                     XPF-WP06 certification
                                                              |
                                                     XPF-WP07 mobile canaries
                                                              |
                                                     XPF-WP08 preview/export
```

After WP00A, WP01, WP02 and WP03 may proceed as bounded parallel implementation
streams, but WP05 may not claim parity until all three provide their inputs.

`XPF-WP00A`, the macOS/common `XPF-WP02A` relocation, locally executable
`XPF-WP02B` conformance corpus and local `XPF-WP01A/B` canonical project,
command-coordinate/workspace slices and `XPF-WP03A` deterministic-font slice
and the common/Metal `XPF-WP04A` lease slice are complete. `XPF-WP05A` now
defines the D3D12/DXGI/Windows-Studio source route without copying semantic
execution. Windows build/device receipts remain `not-run`; this prevents formal
WP01 through WP05 closure. `XPF-WP06A` completes the locally executable
descriptor/Inspector/Agent, generic RFX5 codec, Registry-bound RenderPlan
identity and guarded matrix slice while keeping formal WP06 open. `XPF-WP07A`
now compiles the complete iPhoneOS Core and shared-graphics closures;
`XPF-WP07B` defines the guarded Android source/CI route but stays uncompiled on
this SDK/NDK-free host. `XPF-WP08A` completes the locally executable
Preview/Export semantic-consumer contract without entering G4 product export.
The remaining plan-closing actions require the complete `windows-visual`,
Android official-NDK and full-profile semantic/visual/performance receipts;
none may be inferred from the local macOS/iPhoneOS results.

## Two-host closure route — macOS source closure, GitHub handoff and Windows evidence

This section is the authoritative execution route from the current repository
state to the first real Windows qualification. It removes an ambiguity in the
earlier handoff: the existence of the Windows source files does not mean that
all locally executable source-closure work is finished, and a successful macOS
run does not mean that Windows has only a missing Preview screenshot.

The intended outcome before the first Windows-device run is precisely:

```text
one complete portable semantic implementation
          +
one qualified common RenderPlan/Skia execution route
          +
thin Metal and D3D12 mechanics from the same source checkpoint
          +
macOS physical evidence now
          +
Windows compile/run/visual evidence after the GitHub handoff
```

`source-ready for Windows` means that the shared contracts, common compositor,
D3D12/DXGI source, build orchestration, conformance inputs, failure behavior and
evidence schema are committed from one reviewed checkpoint. It does **not** mean
that MSVC compiled them, that the pinned Windows Skia closure linked, or that a
physical D3D12 device passed. Those states remain `not-run` until the Windows
host produces their receipts.

### Current truthful platform split

| Lane | Present state | Still required before this plan can close |
|---|---|---|
| Portable/shared | Core authority, canonical RFX5, exact-time evaluation, Registry-bound contributions, common `VisualRenderPlan`, common Skia compositor, deterministic packaged-font path, backend leases, Preview/Offline qualification consumer and full fixed conformance corpus exist; active native-semantic debt is zero. ADR-0010 and its paired visual bounds are accepted; `XPF-PRE-WINDOWS-READY` binds the immutable source. | No additional Phase-A work. Any shared-source change invalidates the receipt and must rerun the affected gates. |
| macOS Metal | The application physically renders through the common route; Core 30/30, sanitized Core 30/30 and Visual 51/51 pass; the bounded 10,000-frame Preview receipt and committed 640x360 Desktop-v1 reference capture exist. The commit-bound machine receipt is issued. | Retain this profile as the Windows comparison reference and rerun it only if an affected shared contract/source changes. |
| Windows D3D12 | The hardware-only device, Skia binding, DXGI presenter, Windows Studio route, bounded/typed fence failures and same-program offscreen capture executable are source-defined without a semantic renderer fork. | On Windows, first generate/review/commit the host-specific Skia transitive lock; then compile/link with MSVC, exercise a named non-WARP device, open/resave the same projects, run the semantic/pixel/performance corpus and retain receipts. |
| iOS Metal canary | The Core and shared Graphics closures compile and the product presenter fails closed by design. | No Desktop-v1 product runtime is required here; retain the compile canary and honest non-runtime matrix state. |
| Android Vulkan canary | The source/profile/official-NDK CI route is defined and fails closed as a product presenter. | Obtain the official-NDK Core/Graphics compile receipt. Full Android product runtime remains G9. |

### Phase A — Pre-Windows Source Closure on the macOS host

Complete these items before publishing the Windows handoff checkpoint:

1. **Stabilize repository truth.** Review every tracked and untracked source,
   contract, workflow, dependency profile and evidence file; exclude generated
   `out/` material and accidental artifacts; then create a non-destructive Git
   checkpoint whose commit contains the complete implementation. No Windows
   handoff may be based on an older `HEAD` while required source exists only in
   the macOS working tree.
2. **Accept one explicit Desktop v1 SDR color profile.** Bind authored encoding,
   Rec.709/sRGB interpretation, premultiplied alpha, blend/filter working space,
   gradient interpolation, effect edge/tile/crop rules, BGRA8 target meaning and
   output transfer to the project capability, `VisualRenderPlan` and semantic
   digest. Metal and D3D12 may not inherit different API defaults.
3. **Finish the Windows build entrance.** Commit separate Windows Core,
   Graphics and Visual automation, exact toolchain/dependency checks, the
   official Windows Skia materialization/lock command, and fail-closed guidance
   for missing Qt, font assets, SDKs or hardware. Qt commercial entitlement and
   redistributable-release proof remain deliberately deferred to G6.
4. **Harden D3D12 source behavior before the device run.** Replace unbounded
   waits with declared timeouts, propagate timeout/device-removed/occluded and
   stale-generation diagnostics, and make all failure paths preserve
   Last-Known-Good without WARP, CPU project pixels or silent fallback.
5. **Complete the qualification consumers and corpus.** A test-only offscreen
   consumer must execute the same accepted `VisualRenderPlan` as Preview and
   emit deterministic semantic/capture artifacts. This is qualification
   infrastructure, not a G4 encoder or a production Export claim. Extend the
   corpus and receipt schema so AppleClang and MSVC can compare canonical RFX,
   commands, Registry, font/layout, color and RenderPlan digests plus calibrated
   captures.
6. **Commit the mobile canary automation.** Keep iOS and Android on the same
   shared closure; no mobile semantic implementation or product claim may be
   introduced to make a canary compile.
7. **Run the local readiness gates.** At minimum run documentation and
   architecture checks, macOS Core, sanitized Core, the complete macOS Visual
   lane and both iPhoneOS canaries after the final shared source checkpoint.

Phase A exits only with a review record named
`XPF-PRE-WINDOWS-READY` under `docs/evidence/reviews/`. That record binds the
source commit, dependency/profile digests, toolchain, exact test results,
macOS reference artifact digests and the honest matrix state. Windows and
Android remain explicitly `not-run` where no external receipt exists.

**Phase A closure state (2026-08-09): closed.** The shared source work is bound
to accepted implementation commit
`57d000fc51b7156e08c362f8b04979b4aee5b3fe`, and
[`XPF-PRE-WINDOWS-READY`](../evidence/reviews/XPF-PRE-WINDOWS-READY.md) is the
named exit record. Core binds the accepted
`refusion.color.desktop-v1-sdr.v1` descriptor and SHA-256 into every
program/RenderPlan and the common compositor rejects drift. Presentation
requests carry a validated output-consumer identity; Preview and Offline render
independent GPU targets through the same exact-time common executor, and the
macOS qualification test requires byte-exact pixels. The visual fixture now
uses digest-pinned packaged Noto bytes rather than Arial/system fallback.

Windows source owns a hardware-only D3D12 offscreen qualification executable,
bounded fence waits, typed timeout/device-loss/occlusion/stale-generation
failures and a PowerShell bring-up route. Hosted GitHub automation is explicitly
`compile-only`; only a physical invocation may create the D3D12 capture,
Metal-versus-D3D12 comparison and schema-bound host receipt. The repository
requires a two-pass dependency-lock entrance: a non-qualifying `CompileOnly`
run may generate the Windows transitive Skia lock, but physical qualification
fails closed until that lock has been reviewed, committed and checked out from
a clean source revision. Generated dependency state can therefore never be
silently attributed to an older commit. The repository
also contains one P6 capture comparator, accepted calibrated Desktop bounds,
canonical RFX/command/Registry/font/color/RenderPlan receipts and the iOS/
Android compile-canary automation.

The final shared tree passes the locally executable readiness gates:

```text
macos-core             30/30 passed
macos-core-sanitized   30/30 passed under ASan/UBSan
macos-visual           51/51 passed
ios-core-canary        BUILD SUCCEEDED (iPhoneOS arm64)
ios-graphics-canary    BUILD SUCCEEDED (iPhoneOS arm64 + common Skia)
docs-doctor            107 documents, 0 problems
architecture-check     112 source files, 0 problems, 0 boundary debt
```

The product owner accepted ADR-0010 after the physical macOS visual review on
2026-08-09. The machine receipt binds the accepted commit, toolchain, GPU,
dependency/font/color/RenderPlan identities, 51/51 Visual suite and immutable
reference capture. Phase A has no remaining action. Windows MSVC/build/device/
pixel/performance and Android official-NDK execution states remain `not-run`
until their external receipts exist; this is the honest Phase-B/C boundary, not
unfinished local source closure.

### Phase B — Reproducible GitHub handoff

Push the exact Phase A commit to one canonical ReFusion repository. Build
outputs, local absolute paths, `.refusion` host state, credentials and mutable
dependency checkouts are not committed. The commit must contain every source,
contract, bootstrap command, profile, test fixture and evidence schema needed
to reproduce the work.

The Windows host checks out that exact commit. It does not receive Skia, fonts,
Qt or generated build directories copied from macOS. Official dependencies are
materialized through the repository bootstrap. The Windows-specific transitive
Skia lock is generated and reviewed on Windows, because its resolved closure
and toolchain evidence cannot be fabricated on macOS; the reviewed lock and
receipt are returned through a dedicated evidence branch/commit.

### Phase C — Physical Windows build and qualification

The Windows Agent executes this phase through the root
[`README_FOR_WINDOWS.md`](../../README_FOR_WINDOWS.md), including its editable
diagnostic handoff block. That runbook operationalizes—but does not replace—the
following ordered route:

1. record OS, MSVC/SDK/CMake/Ninja/Python, GPU/driver and Qt engineering-path
   fingerprints;
2. run bootstrap doctor, sync/hydrate the official pinned Skia graph, generate
   and verify `skia-transitive-windows-x64.lock.json`, then build the admitted
   Windows Skia profile;
3. build and test Windows Core, then Graphics, then Studio/Visual—never skip a
   lower failure by launching the UI directly;
4. open and canonical-resave the same macOS-authored project and compare exact
   IDs, revisions, bytes and semantic/Registry/font/color/RenderPlan digests;
5. run D3D12/DXGI attach, resize, occlusion, timeout, stale-generation,
   device-removal and 10,000-frame non-WARP tests with zero CPU project-pixel
   transfer/readback outside the declared qualification capture path;
6. capture the shared visual corpus and calculate its declared mean/max channel,
   alpha/edge and structural metrics against the bound macOS references;
7. commit a human review at
   `docs/evidence/reviews/XPF-WINDOWS-DESKTOP-V1-<date>.md` and a machine receipt
   at `docs/evidence/G1/artifacts/XPF-WINDOWS-DESKTOP-V1-<date>.json`, then update
   the capability matrix only to the states actually proved.

A Windows compilation error is evidence, not permission to create Windows-only
project, FX, text, animation or color semantics. Build/dependency defects are
fixed in the portable build/profile surface. Native resource/presentation
defects are fixed under the D3D12/DXGI adapter. Semantic defects are fixed once
in Core/Runtime/SkiaCommon and re-run on both platforms.

### Phase D — macOS reconciliation and formal closure

The macOS host pulls the Windows evidence commit, verifies its schema and
source ancestry, then compares its canonical and visual receipts against the
bound macOS references. Any code correction creates a new shared checkpoint
and invalidates older affected receipts; macOS and Windows rerun the impacted
corpus from the same new commit.

Only after both Desktop lanes pass independently may the matrix advance the
corresponding capabilities through semantic match, visual tolerance,
performance and `qualified`. The plan then also requires the Android
official-NDK compile receipt, truthful iOS/Android canary state, synchronized
`MP-001`/`CURRENT.md`/evidence, and every criterion in **Plan exit** below.

### Explicit media boundary

This route qualifies the current portable project/Canvas/Text/Shape/Group/FX
visual architecture. Windows Media Foundation hardware video decoding is still
an open `RISK-003`/G1-WP04 and later G4 product requirement. A successful
Windows Canvas Preview must not be reported as complete cross-platform Video
import/playback. When admitted, Windows video uses the same portable media/time/
color contracts and a thin Media Foundation/D3D surface bridge; it does not
enter the common visual pipeline through a second project or renderer meaning.

## Cross-platform conformance corpus

One repository-owned corpus must include, at minimum:

1. Mac-authored and Windows-authored project packages containing the same
   canonical semantic state;
2. IDs, revisions, Composition/canvas/rate/duration, exact start/end frames,
   hierarchy, layer order, transform keys, mask/effect order and asset digests;
3. Shape solid/linear/radial fills, stroke, corners and alpha;
4. Normal/Multiply/Screen/Overlay compositing;
5. normal/inverted ordered masks and nested transforms;
6. Blur -> Shadow -> Glow permutations and declared edge behavior;
7. Latin, Arabic, RTL, mixed direction, diacritics, wrapping and alignment with
   identical packaged font bytes;
8. exact-time samples at representative beginning, middle, keyframe boundary
   and end frames;
9. expected canonical RFX bytes, semantic/registry/layout/render-plan digests;
10. reference captures with dimensions, color profile, mean/max channel error,
    structural metric and an accepted tolerance recorded by profile;
11. preview/export plan-digest equality;
12. explicit unsupported-capability fixtures that prove fail-closed behavior.

GPU qualification captures may read back pixels only inside the declared test
evidence path. Production preview/media/export paths remain subject to the
zero-CPU-project/video-pixel policies.

## Definition of done for any future visual capability

A Layer property, animation family, mask, FX, background recipe or content type
is cross-platform complete only when all applicable items are present:

- portable descriptor, types, units, defaults and compatibility rules;
- Core validation and deterministic evaluation at exact time;
- canonical persistence, migration and semantic digest;
- candidate capability/resource preparation before atomic accepted-bundle
  publication, with Last-Known-Good retained on failure;
- UI and Agent introspection/command parity through one authority;
- backend-neutral RenderPlan lowering;
- one shared Skia execution path and explicit color/edge semantics;
- typed backend capability admission and fail-closed diagnostics;
- same fixture in every required build lane;
- same semantic digest and calibrated visual goldens on every required physical
  platform profile;
- preview/export semantic-path equality when export is in scope;
- performance, device-loss and unsupported-state evidence;
- documentation and capability matrix updated without claiming `not-run` as
  passed.

For a plugin contribution, this also requires stable package/contribution IDs,
versioned state and migrations, signed/digested artifacts where executable,
permission/resource budgets, unresolved round-trip behavior, crash/revocation/
rollback evidence and the tier-specific isolation policy.

## Prohibited remediation paths

- copying Shape/Text/FX/mask/blend logic into Metal, D3D12 and Vulkan files;
- storing native handles, Qt values, absolute host paths or platform font names
  as qualified project truth;
- adding an FX only to Inspector, only to Agent syntax or only to one renderer;
- using a system-installed font in cross-platform text/layout qualification;
- treating a successfully compiled backend as a physically qualified backend;
- treating macOS output as proof for Windows/iOS/Android;
- silently changing color space, filter quality, blend order or unsupported FX;
- allowing a software GPU/decoder, CPU pixel bridge, WARP or Qt media/rendering
  fallback in the production route;
- duplicating Layers to approximate an unsupported animated FX;
- expanding this remediation into G3/G4 features before their normal admission.

## Plan exit

This plan closes only when:

1. project/package, render-plan, text/font, color and capability contracts are
   accepted and covered by cross-toolchain tests;
2. active-guardrail automation passes and temporary native-semantic exceptions
   are zero;
3. common Skia semantics are absent from native backend files;
4. accepted revision publication cannot diverge between Core/UI and Runtime;
5. macOS Metal and Windows D3D12 Desktop v1 profiles independently pass the
   same semantic and visual conformance corpus;
6. iOS/Android contract/build canaries pass without redefining semantics;
7. every claimed current FX/property/plugin has a truthful certification state;
8. `MP-001`, `CURRENT.md`, platform matrix and release evidence point to the
   same result.

Until then, the precise statement is: **ReFusion has a portable semantic Core
and bounded macOS visual evidence; its full renderer/project capability set is
not yet cross-platform qualified.**

## Final review receipt — 2026-08-08

Three independent read-only advisory passes—principal code relocation,
repository governance and FX/plugin extension architecture—were reconciled
into version 4. The review added the atomic publication prerequisite, exact
time/device stamps, bounded effect isolation, deterministic typography details,
public-interface/test relocation, plugin tiers and the active-guardrail ratchet.

Documentation verification after reconciliation:

```text
rfdev.py docs-doctor:        90 documents, 0 problems
rfdev.py architecture-check: 75 source files, 0 problems in its current rules
rfdev_policy_test.py:        passed
```

Version 5 then completed `XPF-WP00A`. The ratchet froze 45 exact debt
signatures/85 occurrences, rejects new or growing allowances, loads active
guardrails into context, validates them in docs-doctor and passes its negative
fixtures plus the 19-test macOS Core workflow. No renderer/platform code was
moved or deleted by this governance implementation.

Version 6 records the implemented `XPF-WP02A` relocation. Atomic Runtime
preparation now precedes accepted revision publication; immutable RenderPlan
lowering and all current visual semantics execute in common C++; Metal is a
native binding/submission backend; and the common/Metal build targets are
separate. Nineteen active debt allowances were retired and the macOS Visual
lane passes 41/41 tests. Windows MSVC/D3D12 evidence, the complete visual
conformance corpus and deterministic packaged fonts remain open and are not
claimed from this macOS result.

Version 7 adds the first cross-toolchain RenderPlan conformance corpus and the
bounded Metal pixel qualification. Four exact-time receipts cover every
currently implemented content/fill/blend/mask/FX family and Group animation;
their primitive digest encoding is explicit and padding-free. The shared
compositor passes the test-only pixel predicates without changing production
readback policy. The corpus is ready for MSVC, but Windows remains `not-run`.

Version 8 adds the locally executable WP01A canonical-project contract.
Locale-free finite numeric and digest formatting, preserved validated UTF-8,
portable case-sensitive ASCII IDs and hostile-locale receipts now protect RFX,
Agent JSON and semantic hashes. The macOS Visual lane passes 44/44 tests; MSVC,
coordinate quantization, copied-workspace and mobile evidence remain open.

Version 9 adds the local WP01B coordinate and workspace contracts. Typed and
derived pixel commands commit to the binary-exact 1/1024 px grid with a fixed
hostile-locale receipt. Copied host-local state is safely regenerated without
bypassing same-path locks, and full unsigned Revisions survive Agent JSON.
MSVC, Windows canonical resave and mobile canaries remain `not-run`.

Version 10 records local WP03A. Two official Noto baselines and their OFL
notices are pinned and project-local; qualified Skia typefaces are constructed
from digest-verified bytes through embedded FreeType, while platform font
providers are isolated and explicitly unqualified. ICU line breaking,
cluster-safe spacing and a fixed Latin/Arabic/RTL layout receipt now pass in the
47-test macOS Visual lane. MSVC exact-receipt execution and a future explicit
multi-font fallback-chain schema remain open, so formal WP03 is not claimed.

Version 11 records local WP04A. Raw integer device, queue, render-target and
viewport-host contracts were replaced by full-identity, shared-lifetime opaque
leases. Each frame now carries exact Core time/epoch and one immutable accepted
render-program lease; Skia no longer owns a separate mutable program. The
47-test macOS Visual lane passes stale target/request rejection. D3D12 and
Vulkan lifetime/synchronization implementations remain `not-run`/open, so
formal WP04 is not claimed.

Version 12 records WP05A source readiness. The D3D12 Skia context, same-device
LUID checks, BGRA8 target wrap, DXGI swapchain/fence presenter, Windows live
Studio selection, Windows-only presenter test and `windows-visual` workflow are
defined. A new common visual-program executor also removes the last indirect
ProjectDocument/compiler include from native Metal/D3D bindings. The macOS
Visual lane remains 47/47 and architecture-check reports 101 source files with
zero problems. Windows compilation, physical rendering, soak/device-loss and
cross-backend pixel comparison remain `not-run`, so WP05 is not closed.

Version 13 records local WP06A. A portable Mask/FX contribution Registry now
drives validation, defaults, Inspector controls, Timeline names and generated
Agent documentation; scale/opacity animation values enforce their admitted
ranges. The capability matrix is machine-readable and guarded against
Registry drift or evidence-state jumps. Descriptor-addressed Application
commands remove both concrete and generic project-stack vocabulary from Studio;
active visual-boundary debt is zero. The macOS Visual lane passes 48/48. Generic RFX
persistence/migration/lowering binding and non-macOS receipts remain open, so
WP06 is not closed.

Version 14 completes the locally executable WP06 architecture route. RFX5
binds canonical project bytes to both property and contribution Registry
digests and uses one ordered typed-parameter codec for Masks and FX while
retaining RFX1–RFX4 as migration inputs. Unknown or missing contribution
parameters fail closed. RenderPlan lowering carries descriptor ID, capability
ID and schema version into the semantic digest consumed by the one common Skia
compositor. The macOS Visual lane passes 48/48, architecture-check inspects 105
source files with zero active visual-boundary occurrences, and docs-doctor
passes 99 documents. Formal WP06 remains open for non-macOS matrix receipts and
final profile admission evidence; no missing run is inferred from this local
result.

Version 15 records the locally executable WP07 slice. The iPhoneOS arm64 Core
and Graphics workflows both build successfully, and the Graphics closure uses
the verified official mobile Skia artifact plus the same Core/RFX/RenderPlan/
SkiaCommon sources as Desktop. iOS native code is UIKit/Metal mechanics only
and deliberately rejects product presentation. Android arm64-v8a/API 28 source,
Skia profile, CMake workflows and official-NDK CI are defined and guarded, but
remain locally uncompiled because this host has no Android SDK/NDK. The matrix
advances iOS only through `compiled`, Android only through `defined`, and no
mobile runtime/semantic/visual/performance claim is inferred. The unused second
Graphite context was also removed from the Metal product path; a negative policy
test now prevents its return. macOS remains green at 48/48 and architecture
checking reports 108 sources with zero problems.

Version 16 records local WP08A. Runtime now prepares interactive Preview and
future Offline Export samples through one exact-time, consumer-neutral
VisualRenderPlan contract; production Skia Preview uses it and parity rejects
different samples or invalid consumer pairs. The complete macOS Visual lane
passes 49/49, the updated iPhoneOS Graphics workflow builds the same contract,
and architecture checking reports 110 sources with zero active boundary debt.
This does not create a production Export claim. Formal plan closure still
requires Windows and Android execution evidence plus the prescribed semantic,
pixel, performance, recovery and device-loss profile receipts.

Version 17 records the bounded macOS Metal Preview profile. A corrected
10,000-frame exact-time loop completes with zero CPU pixel transfer, GPU
readback or unattributed copy; GPU observability remains inside the named
Apple-M1 640x360 ceilings and injected device loss rejects stale generations.
The initial soak's out-of-range timestamp was rejected by Runtime and fixed in
the test rather than hidden by clamping. The Core closure also passes 28/28
under ASan/UBSan. This advances macOS Preview evidence only; Export and all
missing Windows/Android/cross-backend receipts remain open.

Version 18 records XPF-WP02C. A valid Agent file update exposed synchronous Qt
model reentrancy while Application still held the admission mutex. Prepared
publication is now split into an atomic engine-state commit under exclusion and
immutable observer projection after unlock. A regression test re-enters the
accepted snapshot exactly like QML, the real live-reload fixture passes, and
the full macOS Visual lane remains 49/49 while sanitized Core remains 28/28.
Windows execution remains open.

Version 19 makes the two-host closure route explicit. Local work no longer
jumps directly from Windows source definition to a physical Windows run. A
named Pre-Windows Source Closure first finishes the portable SDR color contract,
Windows build entrance, bounded D3D12 failure behavior, offscreen qualification
consumer, shared corpus/evidence schema, repository checkpoint and macOS/iOS
regression receipts. The exact commit is then handed through GitHub to a clean
Windows host, which materializes its own official pinned Skia closure, builds
Core/Graphics/Visual in order, runs physical D3D12/DXGI and cross-backend
qualification, and returns machine/human receipts for reconciliation on macOS.
The section also preserves the honest boundary that Windows Media Foundation
video decode remains separate open G1/G4 work rather than being implied by a
successful Canvas Preview.

Version 20 implements the source side of that route. One explicit Desktop SDR
descriptor is bound into the common program/RenderPlan/compositor; output
consumer identity reaches the same common Skia executor without semantic
branching; and Metal proves exact Preview-versus-Offscreen pixels on separate
GPU targets. The cross-platform visual fixture now uses pinned packaged Noto
bytes and emits canonical project, command, Registry, font, color and
RenderPlan receipts plus a committed macOS PPM reference. Windows gains a
hardware D3D12 offscreen fixture/capture path, bounded DXGI waits, typed failure
diagnostics, a dependency-clean bring-up script and a schema-bound physical
receipt. GitHub runs compile-only and cannot promote physical evidence. The
proposed calibrated Metal/D3D12 policy and receipt schema are guarded by Repo
OS. ADR-0010 owner acceptance and final macOS/iPhoneOS gates still precede the
`XPF-PRE-WINDOWS-READY` evidence checkpoint; MSVC, Windows GPU and Android NDK
execution remain truthfully `not-run`.

Version 21 records the product owner's physical macOS visual acceptance of
ADR-0010 and the paired Desktop-v1 pixel bounds. This advances the candidate
contracts from `proposed` to `accepted` without promoting Windows, Android,
performance or Media Foundation evidence. The next operation is strictly the
commit-bound macOS qualification receipt and `XPF-PRE-WINDOWS-READY` review.

Version 22 closes the named Pre-Windows Source Closure. The accepted source
commit, machine-readable macOS qualification receipt, immutable Metal reference
and `XPF-PRE-WINDOWS-READY` review are now linked. The plan remains active only
for its truthfully external Windows/Android and full-profile qualification
inputs; the exact resume point moves to Phase B GitHub handoff and Phase C
Windows lock/build/device evidence.
