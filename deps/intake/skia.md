---
id: DEP-SKIA-001
kind: dependency-intake
status: macos-native-viewport-proved-windows-pending
owner_role: render-architecture
last_verified: 2026-08-07
---

# Skia intake

- Official origin: https://skia.googlesource.com/skia.git
- Candidate pin: `294d31e0b1aa295d585836ab41bd2fba170e0c5d`, resolved
  directly from the official `main` ref on 2026-08-07.
- Source policy: fresh qualification materializes below ReFusion's ignored
  `out/deps-src`. Ordinary development may use only the content-addressed,
  verifier-resolved ReFusion machine cache proposed by ADR-0012; arbitrary
  external checkouts, copies and junctions remain forbidden.
- License: BSD-3-Clause plus all enabled third-party components.
- Build: official GN/Ninja path; paired depot_tools pin
  `fa1fac8477c70532274e7244777a846537004750`; no normal-build network.
- Role: GPU-backed 2D/text/vector/custom-content producer inside Render Graph.
- Non-role: project truth, scheduler, decoder, cross-layer FX engine, presenter.

G1 must select and prove Apple and Windows backends with the engine-owned
physical device, imported targets, explicit fences/lifetimes, Arabic text
quality, package licenses and no CPU video-pixel path. The bounded Apple visual
slice is accepted; full Apple lifecycle and all Windows qualification remain open.

## Evidence reached

- Fresh Skia and depot_tools checkouts now exist only inside ReFusion `out/`.
- Skia's pinned `DEPS` graph resolved 46 Git dependencies and recorded their
  official origins and revisions.
- The tracked macOS arm64 GN profile built Ganesh, Graphite and Metal from
  source plus explicit targets `skia` and `modules/skshaper`; the resulting
  closure includes the pinned HarfBuzz, ICU, `skunicode` and image dependencies.
- ReFusion CMake rejected unverified source/build state, then imported the
  deterministic archive after origin, revision, profile and artifact checks.
- Metal device/queue ownership, both Skia context factories, drawable wrapping,
  Arabic/Latin shaping and CAMetalLayer presentation passed on macOS.

This is not Windows or video qualification. Those remain measured G1 decisions.

The current bundle explicitly contains `skshaper`, `skunicode_core`,
`skunicode_icu`, HarfBuzz and ICU; `skparagraph` remains excluded. The bundle is
1,191,880,024 bytes with SHA-256
`c432832b55ce7120916bf1eef17a5fb81b948a45ed11ac9c57e0130067fc73c9`.
Windows uses the same declared `skia` + `modules/skshaper` target closure but
remains unbuilt and `not-run` until a Windows runner/device exists.

`macos-arm64-metal-release` is a separate `is_official_build=true` definition.
It is deliberately `defined-not-built`: it receives no release claim until a
clean independent rebuild, typography/render fixture, SBOM/notices, packaging
and platform qualification pass.
