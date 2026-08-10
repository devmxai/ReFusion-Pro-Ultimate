import { defineConfig } from "vite";

const strictContentSecurityPolicy =
  "default-src 'self'; script-src 'self' 'wasm-unsafe-eval'; style-src 'self' 'unsafe-inline'; img-src 'self' blob: data:; worker-src 'self' blob:; connect-src 'self'; media-src 'self' blob:; object-src 'none'; base-uri 'none'; frame-ancestors 'none'";

const strictHeaders = {
  "Cross-Origin-Opener-Policy": "same-origin",
  "Cross-Origin-Embedder-Policy": "require-corp",
  "Content-Security-Policy": strictContentSecurityPolicy,
};

export default defineConfig({
  // React JSX is transformed by Vite's built-in esbuild pipeline. Keeping the
  // same headers in dev and preview prevents a dev-only inline preamble from
  // diverging from the production security boundary.
  server: { headers: strictHeaders },
  preview: { headers: strictHeaders },
  build: {
    target: "es2022",
    sourcemap: true,
  },
});
