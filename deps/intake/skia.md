---
id: DEP-SKIA-001
kind: dependency-intake
status: candidate
owner_role: render-architecture
last_verified: 2026-08-07
---

# Skia intake

- Official origin: https://skia.googlesource.com/skia.git
- Candidate pin: `294d31e0b1aa295d585836ab41bd2fba170e0c5d`, resolved
  directly from the official `main` ref on 2026-08-07.
- Source policy: materialize a fresh checkout only under ReFusion's ignored
  `out/deps-src`; never copy or reuse another project's checkout or machine cache.
- License: BSD-3-Clause plus all enabled third-party components.
- Build: official GN/Ninja path; paired depot_tools pin
  `fa1fac8477c70532274e7244777a846537004750`; no normal-build network.
- Role: GPU-backed 2D/text/vector/custom-content producer inside Render Graph.
- Non-role: project truth, scheduler, decoder, cross-layer FX engine, presenter.

G1 must select and prove an Apple and Windows backend with the engine-owned
physical device, imported targets, explicit fences/lifetimes, Arabic text
quality, package licenses and no CPU video-pixel path. No backend is accepted yet.
