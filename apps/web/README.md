# ReFusion Web professional spike

This is the Web shell and adapter surface for `PLAN-WEB-VS-001`. It is not a
second project evaluator and it does not contain a fake project/video renderer.

The current slice proves:

- QML-derived Launcher/Studio layout and tokens;
- a real browser WebGPU adapter probe and clear-only target submission;
- a typed Engine Worker boundary with fail-closed WASM-unavailable diagnostics;
- responsive, keyboard-addressable DOM controls and empty accepted-state
  projections.

The next implementation gate materializes the digest-pinned C++ WASM engine and
Skia WebGPU backend. Until then the UI deliberately shows no accepted project,
Timeline clip or video frame.

## Development

```bash
npm install
npm run dev
npm run build
```

The Vite dev/preview server emits the COOP/COEP headers needed for future
WebAssembly threads. Production hosting must preserve the same headers and
serve the WASM/Skia artifacts from the same isolated origin.
