# Runtime

Runtime owns render, GPU, media, audio and export orchestration contracts. It
depends only on portable engine types and ports. Native implementations and
Skia adapters depend on Runtime; Runtime never imports Qt or platform SDKs.

Runtime compiles the accepted semantic scene into an immutable backend-neutral
render program and exact-time `VisualRenderPlan`. The common Skia compositor
executes that plan; Runtime never asks Metal, D3D12 or Vulkan to reinterpret a
Layer, Mask, FX, text, color or project clock.

`render/VisualOutputContract` is the mandatory semantic entrance for both the
interactive Preview and future Offline Export. Consumer identity cannot change
lowering: the same accepted program at the same exact `ProjectTime` must yield
the same canvas, operation order and RenderPlan digest. Preview/export clocks
may use independent epochs, but scheduling never redefines project pixels.

`gpu/` contains the engine-owned device/lease contract. A native handle is
ephemeral process state carried only by a generation-bound lease; it is never a
project value and must never be serialized.
