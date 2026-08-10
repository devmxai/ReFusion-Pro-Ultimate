---
id: EVID-VIDEO-VI-WP05
kind: apple-production-video-playback-evidence
plan: PLAN-VIDEO-VS-001
work_package: VI-WP05
status: macos-implementation-checkpoint-qualification-pending
source_branch: feature/shared-video-import-v1
last_verified: 2026-08-10
---

# VI-WP05 — VideoToolbox/Metal production checkpoint

## Honest claim

The accepted imported MP4 is indexed through the pinned demux-only FFmpeg
adapter, decoded by one required-hardware VideoToolbox session and delivered as
GPU-resident NV12 CoreVideo/Metal leases to common Skia. No software video
decoder, CPU pixel conversion, CPU upload or GPU readback is admitted.

## Playback residency

- the decoder consumes exact accepted MP4 sample byte ranges and `avcC`;
- future B-frame outputs produced by a dependency range remain resident;
- bounded queues use 14 maximum published surfaces, an 8-surface published low
  watermark and a 12-surface decoder high watermark;
- consecutive queues reuse existing Skia YUVA wrappers by lease ID;
- the tested complete forward presentation sequence retains one hardware
  decoder session.

The final macOS Visual aggregate passes 64/64. Architecture inspection covers
135 source files with zero problems and zero active visual-boundary debt; the
documentation check covers 152 documents with zero problems.

## Physical source fact

`wv` contains a 2160x3840 H.264 source at exactly 30/1 fps and a 60/1 project.
The correct non-interpolated result is an even 2:1 repeat cadence. True 60 fps
motion requires either a native 60 fps source or a separately admitted,
cross-platform optical-flow feature.

## Remaining exit evidence

Product pause/seek/loop performance receipts and Windows Media Foundation/D3D
parity remain pending. Audio decode/output is owned by VI-WP06.
