# Module boundaries

Allowed dependency direction:

```text
apps -> studio adapters -> application -> core contracts
                                      -> runtime -> platform ports
platform implementations --------------------^ 
Skia/SQLite adapters -------------------------^
```

Rules:

- `src/core`: standard C++ only; no UI, platform, Skia, media SDK, or filesystem
  watcher authority.
- `src/application`: commands, revisions, authoring, transport, AssetDB
  coordination; depends on engine-owned ports.
- `src/runtime`: semantic compiler and media/render/audio/export orchestration;
  never depends on Studio.
- `src/adapters`: Skia, SQLite, and other replaceable technology bindings.
- `src/platform`: native implementations only; no project semantics.
- `src/studio` and `apps/studio`: Qt/QML command and view adapters only.
- `services/live_authoring`: reconciliation/build/validation using application
  services; file watcher events are hints.

Platform conditionals are restricted to `src/platform`, `packaging`, toolchain,
and bootstrap code. Common semantic code must not use `_WIN32`, `__APPLE__`,
Android, or Qt platform branches.

## Visual execution path

```text
Core descriptor/evaluator
        -> Application candidate admission
        -> Runtime compiled render program / exact-time VisualRenderPlan
        -> common Skia scene compositor
        -> Metal | D3D12 | Vulkan target binding and presentation
```

The common compositor owns operation order, transforms, Shape/Text execution,
color, gradients, blend, masks, bounded effect isolation and FX lowering.
Native targets own only resource lifetime, target wrapping, native video-surface
import, synchronization, submission and presentation. Native targets must not
include `ProjectDocument` or inspect Composition/Layer/Mask/Effect/Blend/Text/
Shape/evaluator types. Conversely, the common compositor must not include a
native SDK, Qt, platform presenter or PlatformMedia surface.

Studio projects descriptor-provided controls and immutable accepted snapshots.
It may not create FX defaults, validate effect semantics or branch renderer
behavior by effect ID. Candidate runtime resources are prepared through
Application before the one accepted revision bundle is published.

## GPU ownership path

`src/runtime/gpu` owns the portable device identity and generation-bound native
lease contract. `src/platform/apple/metal` and `src/platform/windows/d3d12`
create the physical device and engine command queue. `src/adapters/skia`
borrows that lease and creates Skia contexts from the same handles. Skia may not
create a competing device or queue, and no native handle may enter project
serialization, UI state, Agent contracts, logs, or persistent files.

External sources and build products live only in ignored `out/deps-src` and
`out/deps-build`. Owned adapter/platform code remains reviewable in `src/`.

The authoritative migration/exception ledger is
`docs/plans/FIX_CROSS_PLATFORM_ARCHITECTURE.md`. Existing violations are
temporary ratcheted debt; touching them may only reduce the recorded surface.
