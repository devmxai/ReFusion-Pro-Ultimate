---
id: G1-WP03
kind: work-package
status: passed-bounded-macos
gate: G1
owner_role: apple-media
evidence: docs/evidence/G1/G1-WP03.md
---

# Outcome

Admit the initial H.264 hardware decode profile through
VideoToolbox/CoreVideo to a Metal-compatible native surface without CPU pixel
mapping, then composite a bounded frame fixture in the engine render path.

# Required proof

Hardware-capability query, exact codec/color/PTS metadata, CVMetalTexture path,
explicit copies/conversions/fences, seek corpus, unsupported fail-closed result,
and counters showing zero CPU maps/readbacks/conversions/uploads.

# Kill criteria

Software decoder selection, buffer lock/map, CPU YUV/RGB conversion, silent
fallback, missing PTS truth, or surface lifetime outside an engine lease.

# Current delivery

Source commit `ac8de22c329a64a9f351796ae5ded280e701984c` adds the
portable H.264/NV12/SDR Rec.709 capability, exact source-time and strict counter
contracts, plus separate macOS/Windows media build lanes. On `MAC-LAB-001`,
VideoToolbox reports H.264 hardware decode support and CoreVideo produces two
NV12 Metal texture planes on the engine device/generation with every forbidden
counter at zero. The Windows lane fails closed as not-qualified pending
G1-WP04. This is capability and native-surface interop proof only; actual
compressed-sample decode, PTS output, seek corpus and surface lease are next.

Source commit `79aafb072ad0f254ef64726231b2e7c7e50b5fe8` also supplies
the portable frame-addressed Play/Pause/Seek substrate and Timeline control
needed to exercise later decoded media. It is qualified here only against the
Shape/Text walking Composition; decoded output has not yet entered transport.

Source commit `1433a576e75f47fa4377259da9c095eab20291a9` advances the
proof from capability to one actual compressed all-IDR H.264 frame. A
hardware-required and hardware-confirmed VideoToolbox session preserves exact
PTS/duration and Rec.709 NV12 output in an opaque engine surface lease; both
planes bind to the engine Metal adapter and Skia composites them as YUVA without
CPU pixel transfer. Multi-frame session scheduling and transport-driven PTS
selection remain next; this is not decoded Timeline playback.

Source commit `a697c6a873760366bb957590038cbf3416e644d0` adds the
Core-owned canonical Project Clock and removes mutable playback position from
the viewport scheduler. One hardware-required VideoToolbox session now decodes
the eight all-IDR access units into one immutable exact-PTS-indexed GPU surface
queue. Core Transport seeks `[3, 7, 1]` select those exact source frames in Skia
at epochs `[1, 2, 3]`, with all forbidden counters zero. B-frame/VFR/long-GOP
dependency-aware seek remained the final bounded proof at that checkpoint.

Checkpoint `CP-G1-0008` closes the Apple proof with a repository-owned
16-frame H.264 High fixture containing B-frames, two GOP roots, variable frame
durations and a non-zero three-second source origin. Portable Runtime now plans
the decode-order dependency window from the nearest sync sample, bounds the
published PTS queue, and rejects publication after a Core transport-epoch or
GPU-device-generation change. Physical VideoToolbox sessions decoded and
flushed three non-trivial windows, released all 19 native leases, and Skia
selected/composited source frames `[9, 10, 9]` from a two-surface GPU queue.
Every forbidden path counter remains zero. MP4/MOV demux, import and production
Timeline Video Layer playback remain G4 rather than being inferred here.
