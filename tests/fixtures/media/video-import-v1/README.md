# Desktop video-import v1 corpus

This directory contains only repository-generated synthetic audiovisual test
patterns for `VI-WP00`. It contains no third-party creative content. Every
payload and normalized index/diagnostic oracle is locked by SHA-256 in its
fixture manifest and in `contracts/media/video-import-v1-fixtures.json`.

The corpus covers:

- admitted H.264 VFR/B-frame MP4 with non-zero source starts and AAC offset;
- admitted H.264/AAC MOV with a 90-degree display matrix;
- admitted H.264/AAC 1920x1080 at 60 fps performance input;
- valid HEVC MP4 rejected by the first profile;
- a deliberately tail-truncated MP4 rejected as corrupt;
- a deliberately CENC-encrypted MP4 rejected before decode.

`tools/generate_video_import_v1_fixtures.py` records the exact offline commands
and tool version. FFmpeg is used here only to create immutable test data; the
Homebrew binary is not a ReFusion dependency and is never invoked by configure,
build, product tests or runtime. The CENC muxer generates fresh encryption
material when explicitly regenerating that row, so its newly reviewed digest
must replace the old one as a deliberate corpus revision. Ordinary builds never
regenerate fixtures.

The normalized packet oracle is provider-assisted preparation data. The future
ReFusion demux adapter must reproduce its portable stream/sample facts without
serializing or exposing FFmpeg types.

All payloads and metadata in this directory are dedicated under CC0-1.0. See
`LICENSE.md`.
