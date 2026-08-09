---
id: ADR-0012
kind: adr
status: accepted
title: Shared allowlisted ISO BMFF demux and canonical media index
owner_role: shared-media-architecture
decision_due: VI-WP00
last_verified: 2026-08-09
accepted_by: product-owner-user-instruction-2026-08-09
---

# Context

The existing G1 proof receives repository-authored Annex-B access units and
timestamps. It does not parse MP4/MOV, own a production media index or prove
that macOS and Windows interpret one container identically.

Using AVFoundation/AVAsset on Apple and Media Foundation Source Reader on
Windows as two independent demux authorities would allow edit lists, packet
ordering, missing timestamps, rotation and malformed-container behavior to
diverge before the common decoder contract is reached. Writing a new ISO BMFF
parser in ReFusion would create a large security-sensitive container project
outside the product's differentiating scope.

# Decision

Use one shared, allowlisted `libavformat` adapter for the first local seekable
ISO BMFF MP4/MOV subset. Pin official FFmpeg tag `n8.0.3`, peeled source commit
`8ae0b34901ba60a802f183ee75a250a9fc3e09a5`, fetched from the official
`https://github.com/FFmpeg/FFmpeg.git` mirror. Its tag object and peeled commit
must match the canonical `https://git.ffmpeg.org/ffmpeg.git` upstream exactly.

The reproducible Windows/MSVC profile requires pinned MSYS2 build tooling,
following FFmpeg's official Windows platform procedure. The demux-only profile
disables assembly, so NASM is deliberately unnecessary. MSYS2 is build-time
tooling only: it may not be discovered implicitly from a developer machine,
and Windows materialization remains unqualified until its version, hash and
receipt are admitted by ReFusion dependency tooling.

FFmpeg is admitted as a **container demux dependency only**:

- build shared libraries, not static copies, for Desktop v1;
- compile `libavformat`, the minimum `libavcodec` packet/codec-parameter
  support and `libavutil` only;
- begin from `--disable-everything`; enable only the `mov` demuxer and the
  library features transitively required by that demuxer;
- disable programs, network, devices, muxers, encoders, decoders, filters,
  post-processing, software scaling and software resampling;
- never pass `--enable-gpl`, `--enable-nonfree` or `--enable-version3`;
- use a custom bounded seekable `AVIOContext` over an engine-owned immutable
  compressed-source lease, so host paths and FFmpeg file protocols do not enter
  the adapter contract;
- produce and retain the exact configure report, enabled-component inventory,
  source diff, license/notices, SBOM and matching source archive for release;
- block redistributable packaging until an explicit LGPL/patent/legal receipt
  accepts the final linkage and notice bundle.

No FFmpeg type crosses `src/adapters/media/ffmpeg`. In particular, `AVFormatContext`,
`AVStream`, `AVPacket`, `AVRational`, codec IDs and error codes are converted at
that boundary into one portable canonical contract:

```text
MediaIndex
  source_digest + source_byte_size + container_profile
  ordered StreamDescriptor[]
    StreamId + container_track_id + kind + codec_configuration_digest
    exact signed time_base + start + duration
    video/audio/color/aperture/orientation metadata
  ordered CompressedSample[] in decode order
    stream_id + sample_index + byte_offset + byte_size
    signed PTS + signed DTS + positive duration + exact time_base
    sync/dependency flags + description_index
```

The adapter must reject missing required PTS/DTS/duration, arithmetic overflow,
out-of-file byte ranges, inconsistent sample-description changes, unsupported
edit lists, encryption, external data references and malformed tables with
stable ReFusion diagnostic codes. It never decodes video, converts pixels,
selects project time or publishes a project revision.

The canonical index is derived and rebuildable. Project truth stores the
content-addressed asset, selected stream identities and the accepted index
contract version/digest; it does not serialize FFmpeg objects or make the cache
a second project authority.

# Enforcement

- The dependency profile must report exactly zero enabled video/audio decoders,
  encoders, filters and muxers.
- Architecture checks reject FFmpeg headers outside the demux adapter and
  reject `avcodec_send_packet`, `avcodec_receive_frame`, `sws_*` and `swr_*` in
  all ReFusion production targets.
- The same committed fixtures must emit byte-identical stream/sample/index
  receipts under AppleClang and MSVC.
- Video packets flow only to the platform `HardwareVideoDecoder`; AAC packets
  flow only to the accepted native audio-decoder port.

# Alternatives rejected

- **Independent AVFoundation and Media Foundation demux semantics:** rejected
  because two container authorities can generate different project/index
  meaning.
- **A ReFusion-authored ISO BMFF parser:** rejected for security, maintenance
  and format-correctness risk.
- **Full FFmpeg runtime or FFmpeg software decode:** rejected by ADR-0004 and
  the zero-CPU-video-pixel invariant.
- **Static LGPL linkage without a reviewed compliance route:** rejected for the
  first profile; the official FFmpeg legal checklist recommends dynamic linking
  as the simplest compliance path.

# Consequences

This adds a non-trivial dependency and release-compliance obligation, but only
one container meaning. The dependency is not admitted into
`deps/manifest.lock.json` and no source is downloaded until this ADR is accepted
and the dependency intake is approved. A later container or streaming protocol
requires a separate allowlist and qualification; it cannot silently enter
through FFmpeg's default configuration.

# Primary references

- `https://ffmpeg.org/doxygen/8.0/md_LICENSE.html`
- `https://ffmpeg.org/legal.html`
- `https://ffmpeg.org/doxygen/8.0/md_README.html`
- `https://www.ffmpeg.org/ffmpeg-all.html`
