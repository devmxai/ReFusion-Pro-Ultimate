---
id: ADR-0003
kind: adr
status: accepted
title: Skia as GPU-backed 2D and text producer
owner_role: render-architecture
decision_due: G1
last_verified: 2026-08-07
---

# Decision

Use pinned Skia as a GPU-backed producer for text, vector, shape, image, and
custom 2D content inside the ReFusion render graph. Skia is not the project
model, media decoder, global scheduler, or cross-layer FX authority.

Select Ganesh, Graphite/Metal, Graphite/Dawn, or another supported binding only
after same-device texture import, synchronization, lifetime, quality, and
performance bake-offs on Apple and Windows.

# Explicit non-decision

This ADR does not select Ganesh, Graphite, Dawn, or a Windows interop route.
Those are G1 measured candidates. Failure of a backend candidate does not
invalidate Skia's accepted semantic role unless every qualified route fails.

# Preflight evidence

On macOS arm64, both Ganesh/Metal and Graphite/Metal contexts were created from
one generation-bound lease of the engine-owned Metal device and command queue.
This proves the contract and source/build intake on one host; it does not choose
between Ganesh and Graphite or qualify presentation, media surfaces, Windows,
device loss, synchronization, performance, or packaging.
