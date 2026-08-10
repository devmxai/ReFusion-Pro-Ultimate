# ReFusion Web professional spike

This is the Web shell and adapter surface for `PLAN-WEB-VS-001`. It is not a
second project evaluator and it does not contain a fake project/video renderer.

The current slice proves:

- QML-derived Launcher/Studio layout and tokens;
- a real browser WebGPU adapter probe and clear-only target submission;
- the existing Core project creation/RFX compiler built as an Emscripten WASM
  module and executed behind a typed Engine Worker boundary;
- desktop-folder project creation/opening through the File System Access API,
  with an explicit OPFS fallback for browsers that do not expose a writable
  desktop-folder picker;
- real video-file copy into `Media/Originals` and a browser-decoded frame path
  copied directly into a WebGPU texture for the first insertion milestone;
- responsive, keyboard-addressable DOM controls and accepted project
  projections.

The first media milestone deliberately stops before semantic `VideoClip`
admission into the common RenderPlan and before the Skia WebGPU compositor is
qualified. The video path is real and GPU-presented, but it is not yet the
final WebCodecs/Skia 2K/4K qualification claim.

## Development

```bash
npm install
npm run dev
npm run build
```

The Vite dev/preview server emits the COOP/COEP headers needed for future
WebAssembly threads. Production hosting must preserve the same headers and
serve the WASM/Skia artifacts from the same isolated origin.

`npm run build` invokes the local Emscripten toolchain to materialize the Core
WASM artifact under `src/wasm`. A checked-in artifact is used only when
Emscripten is unavailable; semantic code remains in the shared C++ Core.
