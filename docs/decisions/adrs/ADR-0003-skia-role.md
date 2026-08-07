---
id: ADR-0003
kind: adr
status: proposed
title: Skia as GPU-backed 2D and text producer
owner_role: render-architecture
decision_due: G1
last_verified: 2026-08-07
---

# Proposed decision

Use pinned Skia as a GPU-backed producer for text, vector, shape, image, and
custom 2D content inside the ReFusion render graph. Skia is not the project
model, media decoder, global scheduler, or cross-layer FX authority.

Select Ganesh, Graphite/Metal, Graphite/Dawn, or another supported binding only
after same-device texture import, synchronization, lifetime, quality, and
performance bake-offs on Apple and Windows.

