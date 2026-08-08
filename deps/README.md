# Dependency intake

`manifest.lock.json` is the only foundation dependency inventory. Candidate
entries are not product qualification.

Every production dependency must record official origin, immutable revision or
archive digest, license/SPDX obligations, build options, supported triples,
patch digests, security owner, update policy, and acceptance evidence.

Host toolchain qualifications live in `deps/toolchains`. A `qualified-local`
record is exact and `bootstrap.py doctor` fails when the active host drifts. A
platform marked `awaiting-runner` is not qualified and cannot close its native
gate; it remains usable only to capture the first explicit CI fingerprint.

Normal CMake configure/build is offline. `tools/bootstrap.py` is the only
foundation network-fetch entry point and writes to the ignored ReFusion-local
`out/deps-src`. For production-intake dependencies, external machine caches and
another project's checkout are forbidden. `sync --fresh` may remove only the
validated direct child for the named dependency. Skia remains an official
GN/Ninja external build; do not replace it with the development-only
GN-to-CMake output.

The reproducible Skia flow is:

1. clone the pinned official `depot_tools` and Skia revisions;
2. execute Skia's own `tools/git-sync-deps` and `bin/fetch-ninja`;
3. record `DEPS` SHA-256, fetched tool SHA-256 values, and every Git dependency
   Origin/HEAD in `out/deps-src/skia-dependencies.lock.json`;
4. build a tracked GN profile under `out/deps-build/skia`;
5. record artifact size/SHA-256 and source/profile identity;
6. let CMake import the artifact only after rechecking official Origin and HEAD.

The deterministic baseline-font flow is also explicit and repository-local:

```bash
python3 tools/bootstrap.py sync-font noto_sans_latin_baseline --fresh
python3 tools/bootstrap.py sync-font noto_sans_arabic_baseline --fresh
python3 tools/bootstrap.py verify-font noto_sans_latin_baseline
python3 tools/bootstrap.py verify-font noto_sans_arabic_baseline
```

`sync-font` downloads only the pinned official release archive, checks its
SHA-256, extracts only the allowlisted font/license members into
`out/deps-src`, and checks every selected-member digest. Normal configure/build
remains offline and accepts no system font as qualified project input.

Qt is not auto-downloaded. Its commercial/LGPL lane and module allowlist must be
decided before a redistributable Studio build. FFmpeg, Dawn/wgpu, OpenColorIO,
JUCE, plugin SDKs, updater, cloud telemetry, and AI models are intentionally not
in the foundation lock until their stage intake/qualification.

The G1 mobile compile canaries use two additional tracked Skia profiles:
`ios-arm64-metal-canary` and `android-arm64-vulkan-canary`. The Android profile
requires the exact `android_ndk` version recorded by `manifest.lock.json`
through `ANDROID_NDK_HOME`; it never searches for an arbitrary host NDK. These
artifacts remain contract-canary inputs and do not qualify a mobile runtime.
