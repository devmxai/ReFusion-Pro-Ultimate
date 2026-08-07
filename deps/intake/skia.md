---
id: DEP-SKIA-001
kind: dependency-intake
status: candidate
owner_role: render-architecture
last_verified: 2026-08-07
---

# Skia intake

- Official origin: https://skia.googlesource.com/skia.git
- Candidate pin: `76941bbcaf48f2927878038412a6a69db0f45449`.
- Local verification: origin and HEAD matched on 2026-08-07.
- License: BSD-3-Clause plus all enabled third-party components.
- Build: official GN/Ninja path; paired depot_tools pin; no normal-build network.
- Role: GPU-backed 2D/text/vector/custom-content producer inside Render Graph.
- Non-role: project truth, scheduler, decoder, cross-layer FX engine, presenter.

G1 must select and prove an Apple and Windows backend with the engine-owned
physical device, imported targets, explicit fences/lifetimes, Arabic text
quality, package licenses and no CPU video-pixel path. No backend is accepted yet.

