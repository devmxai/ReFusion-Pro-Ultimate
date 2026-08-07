---
id: PROD-MEDIA-001
kind: qualification-matrix
status: proposed
owner_role: product-media
canonical_for: initial-media-profile
last_verified: 2026-08-07
---

# Initial media and OS qualification matrix

This is a test target, not a support claim. A row becomes supported only after
decode, surface interop, timing, preview, export, error and clean-machine gates
pass on the declared device profile.

## Desktop G1/G4 profile

| Domain | Initial target |
|---|---|
| macOS | macOS 14+, Apple Silicon arm64 |
| Windows | Windows 11 25H2+, x64 |
| Composition | 1920×1080, square pixels, SDR Rec.709 |
| Container | MP4 |
| Video | H.264/AVC Main/High, 8-bit 4:2:0, hardware capability required |
| Audio in MP4 | AAC-LC, mono/stereo, 44.1 or 48 kHz |
| Image | PNG and JPEG; original source remains immutable |
| Standalone audio | PCM WAV, mono/stereo, 44.1 or 48 kHz |
| Export | H.264/AAC MP4 through qualified hardware encoder only |
| Frame rates | 24/1, 25/1, 30/1, 50/1, 60/1, 24000/1001, 30000/1001, 60000/1001 |
| Timestamp corpus | CFR and VFR, B-frames, long GOP, non-zero/negative source starts |

## Required runtime admission

An import records codec/profile/level, coded and display size, chroma/bit depth,
color metadata, frame presentation index, audio sample format/rate/layout, and
native hardware capability. Admission fails before playback when the profile,
device, same-adapter surface interop, or hardware encoder is unavailable.

No decoder or container name alone proves support. Every admitted path publishes
its physical adapter/device identity, native surface format, copies/conversions,
fences, and zero-CPU-pixel counters.

## Mobile contract canary

- iOS: Qt-supported iOS baseline, arm64, VideoToolbox+Metal capability probe.
- Android: Qt-supported Android baseline, arm64-v8a, hardware MediaCodec Surface
  to AHardwareBuffer/Vulkan proof.

Mobile rows remain compile/contract canaries until G9.

## Explicitly unqualified initially

HEVC/H.265, AV1, VP9, ProRes, HDR, 10/12-bit, RAW, alpha video, image sequences,
MXF/MKV/WebM, Dolby formats, multichannel audio, Windows 10, Windows ARM64,
Intel Mac, 4K/8K, and unrestricted frame rates.

The matrix may widen only after the existing Creator Loop remains green; it may
narrow when hardware/licensing evidence fails. It never enables software fallback.

