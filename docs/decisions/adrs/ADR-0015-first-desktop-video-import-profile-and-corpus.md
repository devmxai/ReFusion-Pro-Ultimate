---
id: ADR-0015
kind: adr
status: accepted
title: First Desktop video import profile, metadata policy and fixture corpus
owner_role: product-media
decision_due: VI-WP00
last_verified: 2026-08-09
accepted_by: product-owner-user-instruction-2026-08-09
---

# Context

“MP4 support” or “H.264 support” is not a testable capability. A professional
first slice needs an explicit container subset, codec limits, time/metadata
rules, negative corpus and device/resource budget. A profile can widen later;
silent inference or software fallback cannot.

# Decision

Adopt `refusion.media.desktop-video-import.v1` for the first same-commit macOS
and Windows qualification.

## Container and track subset

- local, seekable, unencrypted, non-fragmented ISO BMFF MP4 or QuickTime MOV;
- `moov` before or after media data is legal; external data references are not;
- exactly one selected H.264 video track and zero or one selected AAC-LC audio
  track; multiple alternate/video tracks require a later selection contract;
- one normal-rate media edit plus an optional leading empty edit is legal for
  explicit start offset. Repeats, reverse/rate edits and unsupported edit-list
  structures fail closed;
- every selected sample has bounded in-file byte range, signed PTS and DTS,
  positive duration, positive exact time base and stable decode order;
- VFR, B-frames, long GOP and non-zero or negative source starts are required
  corpus cases rather than normalized away.

## Video subset

- H.264/AVC Baseline, Main or High, 8-bit 4:2:0 progressive, level no greater
  than 4.2;
- maximum coded pixel count `1920 * 1080`, with either landscape 1920x1080 or
  portrait 1080x1920 display orientation, maximum 60 presentation frames/s and
  maximum declared bitrate 50 Mbit/s;
- hardware decode is mandatory and output must be same-adapter native two-plane
  NV12 video-range;
- explicit BT.709 primaries, BT.709 transfer and BT.709 matrix are mandatory;
  missing/ambiguous/HDR/wide-gamut color metadata fails instead of being
  guessed;
- square pixels in v1. Clean aperture must be positive, contained by coded
  extent and map exactly to the declared display extent;
- track orientation may be identity or an orthonormal 90/180/270-degree
  rotation. Mirroring, shear, arbitrary rotation and perspective are rejected.

Orientation, clean aperture and source-to-display mapping are normalized once
into portable metadata and the future `DrawVideoFrame` operation. Metal and D3D
adapters do not reinterpret the QuickTime matrix.

## Audio subset

- MPEG-4 AAC-LC only, mono or stereo, 44,100 or 48,000 Hz;
- exact AudioSpecificConfig, source start, priming/roll/end padding and sample
  duration are required when applicable;
- HE-AAC/SBR/PS, surround, dynamic format changes and malformed gapless metadata
  are rejected in v1.

## Bounded source and runtime profile

- maximum qualified source duration: 10 minutes;
- maximum source byte size: 8 GiB;
- bounded decode queue: at most 12 native video surfaces and 32 compressed
  samples per active stream;
- active video-surface budget: at most 128 MiB for the declared 1080p stream;
- 60 fps frame deadline: 16.67 ms; after one-second warm-up, missed/repeated
  presents must remain below 0.1% over 10,000 requested frames;
- first visible frame p95 no greater than 250 ms after admitted preroll;
- exact seek-to-visible-frame p95 no greater than 200 ms and maximum 350 ms on
  the named qualification device tier;
- A/V bounds are those in ADR-0013;
- software decoder, CPU video-pixel map/conversion/upload, GPU readback,
  cross-adapter and hidden-fallback counters remain exactly zero.

The macOS tier is `MAC-LAB-001` Apple Silicon M1 or stronger with VideoToolbox
and Metal. The Windows tier is a named Windows 11 x64 non-WARP D3D12 adapter
with a hardware H.264 MFT and same-adapter surface interop; its exact GPU/driver
identity is recorded by VI-WP09 and cannot be fabricated on macOS.

## Immutable corpus

VI-WP00 must materialize and SHA-256 lock repository-owned synthetic assets:

1. `mp4-vfr-bframes-aac-offset`: VFR/long-GOP/B-frames, non-zero video origin,
   AAC start offset, explicit Rec.709;
2. `mov-portrait-rotation-aac`: quarter-turn track matrix, portrait display,
   clean aperture and AAC-LC;
3. `mp4-landscape-1080p60`: physical performance/reference row;
4. `mp4-unsupported-hevc`: valid container rejected by codec profile;
5. `mp4-corrupt-truncated`: malformed table/range rejection;
6. `mp4-encrypted-cenc`: encrypted-track rejection without key handling.

All audiovisual content is a ReFusion-generated test pattern/tone dedicated to
CC0-1.0. Each row records generator source, exact command/tool version, payload
SHA-256, byte size, expected stream/sample/index digest and stable diagnostic or
admission result. The offline generation tool is not a runtime dependency.

# Stable failure behavior

Unsupported, encrypted, malformed, oversized or hardware-unavailable media is
rejected before project publication with typed diagnostics. Import staging is
removed or recoverable, the old accepted Revision and Canvas stay active, and
the application never tries another decoder/profile silently.

# Alternatives rejected

- **Accept arbitrary MP4/MOV and rely on device behavior:** not testable and not
  cross-platform.
- **Infer Rec.709 from resolution:** rejected because color is semantic input.
- **Normalize all input to CFR or zero start:** rejected because it destroys VFR
  frame selection and A/V offset truth.
- **Qualify 4K/HDR/HEVC in the first slice:** deferred until the first Creator
  Loop remains green.

# Consequences

The first supported row is intentionally narrow but real: it includes Reels,
landscape, VFR, B-frames, AAC and rotation. Files outside it remain readable
only after a later profile is explicitly admitted; they do not activate a
software fallback or platform-only approximation.

## Amendment

[`ADR-0018`](ADR-0018-desktop-sdr-h264-4k-intake-amendment.md) widens only the
shared SDR H.264 intake bounds to portrait/landscape 4K through Level 5.2 and
defines one receipt-bearing missing-transfer normalization. All other
fail-closed and zero-CPU-video-pixel decisions in this ADR remain active.
