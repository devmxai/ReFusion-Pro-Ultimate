---
id: ADR-0006
kind: adr
status: proposed
title: Initial qualification OS and media profile
owner_role: product-media
decision_due: G1-exit
last_verified: 2026-08-07
---

# Proposed decision

Use a deliberately narrow G1/G4 qualification target:

- macOS 14+ on Apple Silicon;
- Windows 11 25H2 x64 for the strict Source Reader/Sink Writer hardware-only
  enforcement candidate;
- MP4 with H.264/AVC 8-bit 4:2:0 SDR Rec.709 and AAC-LC stereo;
- PNG/JPEG still images and PCM WAV audio;
- 1920x1080 reference composition and the exact frame/sample-rate matrix in
  `docs/product/MEDIA_MATRIX.md`.

# Reasoning

Windows 11 25H2 is the first documented client for
`MF_READWRITE_USE_ONLY_HARDWARE_TRANSFORMS`, which can fail when no matching
hardware MFT exists. Earlier Windows support may be widened only if manual MFT,
D3D device, surface and no-software evidence proves equivalent fail-closed
behavior.

Apple VideoToolbox exposes an explicit hardware-decode capability query; actual
surface/codec/profile support remains a runtime gate.

# Excluded from initial qualification

HEVC, AV1, VP9, ProRes, HDR, 10/12-bit, RAW, alpha video, multichannel audio,
arbitrary containers, Windows 10, and Intel Mac. Exclusion means “not yet
qualified,” not a permanent product rejection.

