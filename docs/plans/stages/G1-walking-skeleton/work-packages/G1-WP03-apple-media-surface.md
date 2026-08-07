---
id: G1-WP03
kind: work-package
status: active
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
