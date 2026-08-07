# Bounded H.264 hardware-decode fixture

This directory is a repository-owned synthetic media fixture for `G1-WP03`.
It contains no third-party audiovisual content. The payload is a short moving
test pattern encoded by Apple's hardware H.264 encoder and locked by SHA-256 in
`fixture.json`.

The elementary stream intentionally includes access-unit delimiters and uses
eight independent IDR frames. This keeps the first decompression proof small
and makes packet PTS/duration explicit. It does not qualify MP4 demux, B-frame
reordering, VFR, long-GOP seek, audio, or product import; those corpus rows
remain later work.

FFmpeg was used only as an offline command-line wrapper around the Apple
hardware encoder to materialize this committed fixture. It is not fetched,
linked, invoked, or admitted by ReFusion configure, build, test, or runtime.
