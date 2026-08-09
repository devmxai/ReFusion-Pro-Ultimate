---
id: DEP-FFMPEG-DEMUX-001
kind: dependency-intake
status: accepted-macos-materialized-windows-not-run
owner_role: media-release
decision: ADR-0012
last_verified: 2026-08-09
---

# FFmpeg demux-only intake

## Intended role

One shared ISO BMFF MP4/MOV container reader behind ReFusion's portable
`MediaDemuxPort`. This intake does not admit FFmpeg video/audio decoding,
encoding, filtering, scaling, resampling, playback, rendering or export.

## Immutable source

| Field | Accepted value |
|---|---|
| Official fetch mirror | `https://github.com/FFmpeg/FFmpeg.git` |
| Canonical upstream | `https://git.ffmpeg.org/ffmpeg.git` |
| Tag | `n8.0.3` |
| Tag object | `3b9813e76f3d2a6663de825ccebbef3968d7a576` |
| Peeled source commit | `8ae0b34901ba60a802f183ee75a250a9fc3e09a5` |
| License lane | LGPL-2.1-or-later candidate; GPL/nonfree/version3 features forbidden |
| Update policy | explicit intake review only; no floating branch/tag resolution |
| Source location | ReFusion-local `out/deps-src/ffmpeg`; never a system/Homebrew checkout |

The tag/commit values were verified read-only against both official remotes on
2026-08-09. They match exactly. The engineering direction is accepted and the
source is locked in `deps/manifest.lock.json`. The macOS materialization and
allowlist receipt passed; the Windows/MSVC materialization remains not run.

## Required build-tool admission

The official FFmpeg Windows/MSVC procedure uses an MSYS2 shell. ReFusion must
pin and receipt that shell before any reproducible-build claim. The demux-only
profile disables assembly and therefore does not require NASM. PATH discovery
or a developer-global MSYS2 install is not an accepted dependency source. This
tooling does not enter the packaged application.

## Build allowlist

The final platform profiles must be generated from the same semantic allowlist.
Toolchain syntax may differ, but the enabled-component inventory may not:

```text
--disable-everything
--enable-shared
--disable-static
--disable-programs
--disable-doc
--disable-network
--disable-autodetect
--disable-avdevice
--disable-avfilter
--disable-swscale
--disable-swresample
--disable-decoders
--disable-encoders
--disable-muxers
--disable-filters
--disable-protocols
--disable-bsfs
--disable-parsers
--disable-asm
--enable-avformat
--enable-avcodec
--enable-avutil
--enable-demuxer=mov
```

Custom `AVIOContext` callbacks read one immutable engine source lease, so no
FFmpeg file/network protocol is enabled. Configure may add only dependencies
that the allowlisted `mov` demuxer and the three named libraries require. The
generated report must prove zero decoders, encoders, muxers, filters, devices,
protocols and bitstream filters before the build is admitted.

If FFmpeg's configure system cannot produce this exact functional closure on
both AppleClang and MSVC-supported Windows tooling, ADR-0012 returns to review;
the allowlist is not weakened silently.

## Release and legal gates

Before any redistributable build:

1. legal owner reviews LGPL dynamic-linking obligations and H.264/AAC/container
   patent/distribution obligations for the target markets;
2. matching source archive, source diff, configure command/report and enabled
   component inventory are retained;
3. library notices, license texts, SBOM and source-offer/download link are
   packaged with the application and release page;
4. binaries are scanned to reject GPL/nonfree symbols/configuration;
5. macOS and Windows package tests prove the shared libraries can be replaced
   as required by the accepted compliance route.

Primary official references:

- `https://ffmpeg.org/doxygen/8.0/md_LICENSE.html`
- `https://ffmpeg.org/legal.html`
- `https://ffmpeg.org/doxygen/8.0/md_README.html`
- `https://www.ffmpeg.org/ffmpeg-all.html`

This document is accepted engineering intake evidence, not legal advice. The
component is present in the dependency manifest and materialized on macOS.
Redistribution remains blocked until the owner/legal gates above pass, and the
Windows build remains unqualified until its same-profile receipt is recorded.
