# B-frame, VFR and long-GOP hardware-decode fixture

This directory is a repository-owned synthetic `G1-WP03` corpus row. It has no
third-party audiovisual content. The 16-frame moving test pattern is encoded
offline by Apple's H.264 hardware encoder with B-frames, two sync-rooted GOPs,
access-unit delimiters and explicit SDR Rec.709 metadata.

`fixture.json` records each access unit in decode order with its exact
presentation time, decode time, duration, source-frame identity and sync flag.
The timestamps are shifted to a non-zero three-second source origin. ReFusion
tests pass those timestamps as packet metadata; the elementary stream is not a
container or a demux qualification claim.

FFmpeg is used only as an offline wrapper to create this committed synthetic
fixture. It is not fetched, linked, invoked or admitted by ReFusion configure,
build, test or runtime.
