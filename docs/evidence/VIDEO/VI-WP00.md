---
id: EVID-VI-WP00-PASS-2026-08-09
kind: implementation-evidence
plan: PLAN-VIDEO-VS-001
work_package: VI-WP00
status: passed
date: 2026-08-09
source_head_at_review: 718346cd88e835696d258533660ac5e7f7f48fa0
---

# VI-WP00 decision, corpus and dependency evidence

## Outcome

VI-WP00 is **passed** as the decision, dependency-profile and immutable-corpus
entry gate. Four ADRs and the Desktop v1 profile are accepted; six synthetic
payload/oracle pairs are materialized and verified; official FFmpeg `n8.0.3`
source is locked; and the macOS build receipt proves the demux-only allowlist.

No product C++/QML/platform import code was added by this package. Product
MP4/MOV import therefore remains unavailable until VI-WP01 and later packages.

## Decision package

| Decision | Proposed authority | State | Blocks |
|---|---|---|---|
| Shared ISO BMFF demux provider and canonical packet/index boundary | [ADR-0012](../../decisions/adrs/ADR-0012-shared-iso-bmff-demux-and-media-index.md) | accepted for engineering; release legal gate retained | VI-WP01–05 |
| Native AAC pipeline, PCM contract and audio clock correlation | [ADR-0013](../../decisions/adrs/ADR-0013-native-audio-pipeline-and-clock-correlation.md) | accepted | VI-WP01, VI-WP06–10 |
| Asset/MediaSource/linked clips, copy import and exact relink | [ADR-0014](../../decisions/adrs/ADR-0014-portable-media-assets-linked-clips-and-relink.md) | accepted | VI-WP01, VI-WP03, VI-WP07 |
| First Desktop profile, metadata, limits, failures and corpus | [ADR-0015](../../decisions/adrs/ADR-0015-first-desktop-video-import-profile-and-corpus.md) | accepted | every later VI package |

Machine-readable authorities:

- [`desktop-video-import-v1.json`](../../../contracts/media/desktop-video-import-v1.json)
- [`video-import-v1-fixtures.json`](../../../contracts/media/video-import-v1-fixtures.json)
- [`ffmpeg-demux.md`](../../../deps/intake/ffmpeg-demux.md)

## Technical screening result

The accepted demux provider is official FFmpeg `libavformat` tag `n8.0.3`,
peeled commit `8ae0b34901ba60a802f183ee75a250a9fc3e09a5`, behind a custom AVIO adapter
and a zero-decoder/encoder/filter allowlist. The canonical upstream and official
GitHub mirror returned the same immutable tag values:

```text
3b9813e76f3d2a6663de825ccebbef3968d7a576 refs/tags/n8.0.3
8ae0b34901ba60a802f183ee75a250a9fc3e09a5 refs/tags/n8.0.3^{}
```

Official FFmpeg documentation states that `libavformat` implements container
formats/basic I/O, decoders and demuxers can be disabled/allowlisted, default
FFmpeg is LGPL-2.1-or-later unless GPL/nonfree options are enabled, and dynamic
linking is its recommended compliance route. This is technical screening, not
legal acceptance.

The selected native audio route is AudioToolbox AudioConverter/Core Audio on
macOS and Media Foundation AAC Decoder/WASAPI on Windows. Both feed one planar
float32 PCM and exact-sample contract; endpoint clocks supply correlated ticks
to Core and never become a second ProjectClock authority.

## Fixture receipt

The required six-row corpus is committed under
`tests/fixtures/media/video-import-v1/` with CC0-1.0 ownership, a per-row
manifest and a normalized index or diagnostic oracle:

- positive VFR/B-frame/AAC-offset MP4;
- positive rotated portrait MOV;
- positive 1080p60 landscape performance MP4;
- unsupported HEVC MP4;
- corrupt/truncated MP4;
- encrypted CENC MP4.

`python3 tools/verify_video_import_v1_fixtures.py` verified all 6/6 payload
digests, byte sizes, oracle digests, IDs and expected results. The corpus totals
approximately 3.8 MiB. Homebrew FFmpeg 8.1.2 was used only by the explicit
offline generator; it is not a configure, build, test-runtime or product
dependency.

## Dependency receipt

`python3 tools/bootstrap.py sync ffmpeg --fresh` created a clean controlled
checkout at `out/deps-src/ffmpeg` with:

```text
origin: https://github.com/FFmpeg/FFmpeg.git
tag object: 3b9813e76f3d2a6663de825ccebbef3968d7a576
peeled/head: 8ae0b34901ba60a802f183ee75a250a9fc3e09a5
worktree: clean
```

`macos-arm64-demux-shared` then built shared `libavformat`, `libavcodec` packet
support and `libavutil` with `@rpath` install names. The build receipt reports:

```text
enabled demuxers: MOV
enabled decoders: 0
enabled encoders: 0
enabled muxers: 0
enabled filters: 0
enabled bitstream filters: 0
enabled parsers: 0
enabled protocols: 0
```

The Windows/MSVC profile is machine-readable but remains source-defined/not-run.
Release redistribution also remains blocked on the explicit legal/LGPL/patent
receipt required by ADR-0012.

## Branch and worktree integrity

The current worktree contains the owner-directed Unified Creative Authoring and
Video vertical-slice planning changes on
`research/agent-authoring-preset-recipes`. Creating
`feature/shared-video-import-v1` from this dirty research branch would violate
the clean-base workflow. The planning package must first be reviewed and
preserved on its correct branch; no reset, checkout or implicit discard is
authorized by this receipt.

## Acceptance checklist

The product owner accepted all four ADRs by the instruction to continue on
2026-08-09. VI-WP00 completion is:

1. **complete** — six immutable fixture rows and oracles;
2. **complete on macOS / source-defined on Windows** — official pinned demux
   source and profiles inside ReFusion;
3. **complete on macOS** — zero forbidden component allowlist receipt;
4. **complete** — documentation, architecture, fixture and Core checks;
5. **next governance action** — create the clean feature branch without
   discarding the current research worktree;
6. **next code package** — VI-WP01 portable media/project contracts.

The exact claim is: **VI-WP00 passed; ReFusion still has only the bounded
elementary-stream hardware proof, not product video import.**

## Verification of this preparation change

```text
python3 tools/rfdev.py docs-doctor
  137 documents, 0 problems

python3 tools/rfdev.py architecture-check
  116 source files, 0 problems, 0 visual-boundary debt

cmake --workflow --preset macos-core
  configured and built successfully; 31/31 tests passed

python3 tools/verify_video_import_v1_fixtures.py
  6 rows, all receipts verified

python3 tools/bootstrap.py build-ffmpeg --profile macos-arm64-demux-shared --fresh
  official source verified; MOV demuxer only; zero forbidden component categories

git diff --check
  passed
```

These checks prove the VI-WP00 entry package and macOS dependency materialization.
They do not prove the future ReFusion demux adapter, hardware playback, A/V sync,
Windows runtime qualification or product Video import.
