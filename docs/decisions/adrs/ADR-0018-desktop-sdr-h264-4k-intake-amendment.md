---
id: ADR-0018
kind: adr
status: accepted
title: Desktop SDR H.264 4K intake and bounded color-metadata normalization
owner_role: product-media
decision_due: VI-WP03-correction
last_verified: 2026-08-10
accepted_by: product-owner-user-instruction-2026-08-10
---

# Context

The first bounded profile in ADR-0015 rejected common professional and phone
SDR sources whose H.264 level is 5.0/5.1 or whose coded raster is 4K. A physical
2160x3840 High Level 5.1 source therefore failed before ImportVideo admission,
even though its 8-bit 4:2:0, rate, bitrate, range, primaries and matrix were
inside the intended Desktop hardware path. The same source explicitly declares
BT.709 primaries and matrix but leaves transfer unspecified.

Removing admission checks or adding a macOS-only exception would destroy the
cross-platform contract. Guessing color from resolution also remains rejected.

# Decision

Amend the shared Desktop SDR H.264 intake profile as
`refusion.media.desktop-video-import.v1.1`:

- admit Baseline, Main and High H.264 through Level 5.2;
- admit at most 3840 pixels on either coded axis and at most 3840x2160 coded
  pixels, covering both landscape and portrait 4K;
- retain 8-bit 4:2:0 progressive, square-pixel, 60 fps, 50 Mbit/s, local
  seekable MP4/MOV and all zero-software-decode requirements;
- ignore an ancillary `tmcd` Timecode track after encryption checks; it is not
  selected project media and must not cause a valid single-Video source to be
  rejected;
- retain explicit video range, BT.709 primaries and BT.709 matrix;
- when and only when transfer is unspecified while every preceding SDR
  condition is explicit, normalize transfer to BT.709 and bind the
  `bt709_transfer_defaulted` notice into the canonical MediaIndex digest;
- reject any different, HDR, wide-gamut, full-range, ambiguous primaries/matrix
  or non-8-bit case rather than inferring it;
- expand the bounded active native-surface budget to 192 MiB. Runtime admission
  must still prove hardware decode and same-adapter GPU residency.

This is one portable semantic contract. FFmpeg, VideoToolbox and Media
Foundation adapters do not independently choose supported levels or color
defaults.

# Evidence

The immutable corpus adds a generated 2160x3840, 30 fps, High Level 5.1,
video-only MP4 with BT.709 primaries/matrix and unspecified transfer. Its index
must carry the normalization notice. A physical user source with the same
admission characteristics is tested separately and is never committed.

# Consequences

- Import may accept a 4K source before Canvas playback exists; hardware runtime
  admission remains a later fail-closed gate.
- Windows must reproduce the same fixture/index digest under MSVC before the
  shared media package closes.
- This decision does not add HEVC, HDR, 10-bit, 4:2:2/4:4:4, software decode,
  arbitrary color inference or a platform-only fallback.
