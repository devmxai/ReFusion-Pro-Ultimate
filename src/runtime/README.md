# Runtime

Runtime owns render, GPU, media, audio and export orchestration contracts. It
depends only on portable engine types and ports. Native implementations and
Skia adapters depend on Runtime; Runtime never imports Qt or platform SDKs.

`gpu/` contains the engine-owned device/lease contract. A native handle is
ephemeral process state carried only by a generation-bound lease; it is never a
project value and must never be serialized.
