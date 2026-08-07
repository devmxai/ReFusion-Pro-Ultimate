---
id: DEP-SKIA-001
kind: dependency-intake
status: macos-foundation-proved-windows-pending
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

## Evidence reached

- Fresh Skia and depot_tools checkouts now exist only inside ReFusion `out/`.
- Skia's pinned `DEPS` graph resolved 46 Git dependencies and recorded their
  official origins and revisions.
- The tracked macOS arm64 GN profile built Ganesh, Graphite and Metal from
  source with the HarfBuzz and image dependencies required by target `skia`.
- ReFusion CMake rejected unverified source/build state, then imported the
  deterministic archive after origin, revision, profile and artifact checks.
- Metal device/queue ownership and both Skia context factories passed on macOS.

This is not backend selection or a Windows/presenter/video qualification. Those
remain measured G1 decisions.

The current bundle does not contain `skshaper`, `skparagraph`, or
`skunicode_icu`; setting `skia_use_icu=true` makes those targets available but
does not cause target `skia` to build them. G1/G3 text work must name, build and
test its exact shaping/unicode targets rather than inferring them from GN flags.

`macos-arm64-metal-release` is a separate `is_official_build=true` definition.
It is deliberately `defined-not-built`: it receives no release claim until a
clean independent rebuild, typography/render fixture, SBOM/notices, packaging
and platform qualification pass.
