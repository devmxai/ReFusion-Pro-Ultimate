---
id: EVID-VIDEO-VI-WP02
kind: shared-container-demux-media-index-evidence
plan: PLAN-VIDEO-VS-001
work_package: VI-WP02
status: macos-passed-msvc-pending
source_branch: feature/shared-video-import-v1
last_verified: 2026-08-09
---

# VI-WP02 — Container demux and exact media index

## Honest claim

The shared demux/index implementation is complete and physically verified with
AppleClang on macOS for the accepted six-row corpus. The matching FFmpeg n8.0.3
Windows profile and the same fixed canonical receipts are source-defined but
have not run under MSVC. VI-WP02 is therefore locally passed, not cross-Desktop
closed.

This package does not publish an imported Project revision, decode video, draw
video pixels, output audio or expose Studio Import UI. Those remain VI-WP03 and
later work.

## Implemented shared boundary

- immutable path-free random-access compressed-source lease;
- one pinned FFmpeg n8.0.3 libavformat adapter using bounded custom AVIO;
- MOV/MP4 demux only, with zero decoders, encoders, parsers, filters, muxers,
  bitstream filters or protocols in the admitted dependency profile;
- exact selected Stream identity, codec configuration bytes/digests, signed
  start, rational time base, duration and typed Video/Audio metadata;
- decode-order compressed samples with immutable byte ranges, PTS, DTS,
  duration, sync/discard/dependency flags and sample-description identity;
- deterministic validation, canonical bytes and SHA-256 index digest;
- bounded asynchronous indexing with 1–4 jobs, cancellation and a validated
  content-addressed derived cache that invalidates corrupt entries;
- one common MediaIndex-to-hardware-scheduler projection that preserves decode
  order, derives stable presentation-frame identity from exact PTS and carries
  codec configuration and byte ranges without a platform container parser;
- fail-closed unsupported-container, encrypted, corrupt, unsupported-profile,
  duplicate-PTS and unrepresentable-time diagnostics.

CPU container/metadata parsing is used. No Video decoder is linked or called,
and no decoded Video pixel, native surface, renderer operation, UI object or
Project mutation is present in this package.

## Fixed receipts

```text
mp4-vfr-bframes-aac-offset
  sha256:bebd0bf5ad5e88563ab81ea63662ddc643180db520375b962371790ee151c91a
mov-portrait-rotation-aac
  sha256:0f53e5900bb1d93fc6d45866dd369817cccca2c8d4a47e4e5149b5c33e8a4a7a
mp4-landscape-1080p60
  sha256:70c092e5ca8c75830c4ccfa561eec97ae376b2189898e899a8fab3bd12b4b1c2
synthetic MediaIndex contract
  sha256:c85d1ba60f7109f9447d455a19e9d8d3fd1df22fc57ae4083a358e3cb7585ab7
```

The positive fixtures also pass the shared hardware-scheduler projection. The
HEVC, truncated and CENC fixtures produce their fixed typed rejections; random
non-BMFF bytes reject as unsupported container; pre-cancelled work publishes no
index.

## macOS verification

```text
cmake --preset macos-demux
cmake --build out/build/macos-demux
ctest --test-dir out/build/macos-demux --output-on-failure
python3 tools/verify_video_import_v1_fixtures.py
python3 tools/rfdev.py architecture-check
python3 tools/rfdev.py docs-doctor
```

Checkpoint result:

```text
macos-demux CTest: 36/36 passed
architecture-check: 123 source files, 0 problems, 0 visual-boundary debt
docs-doctor: 139 documents, 0 problems
fixture verification: 6/6 rows passed
git diff --check: passed
```

## Remaining exit evidence

On Windows, materialize the pinned `windows-x64-demux-shared` profile, compile
the same targets with MSVC and reproduce all four canonical digests. No Windows
container parser, alternate MediaIndex schema or platform timestamp correction
is permitted. A mismatch is repaired in shared code and requalified on macOS.

## Next local package

VI-WP03 may build the atomic ImportVideo transaction against this boundary on
the macOS feature branch. It must not bypass the pending MSVC receipt or claim
cross-Desktop closure.
