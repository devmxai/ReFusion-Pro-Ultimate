# Dependency intake

`manifest.lock.json` is the only foundation dependency inventory. Candidate
entries are not product qualification.

Every production dependency must record official origin, immutable revision or
archive digest, license/SPDX obligations, build options, supported triples,
patch digests, security owner, update policy, and acceptance evidence.

Normal CMake configure/build is offline. `tools/bootstrap.py` is the only
foundation network-fetch entry point and writes to ignored `out/deps-src` or an
explicit cache. Skia remains a GN/Ninja external build; do not replace its
official build with the development-only GN-to-CMake output.

Qt is not auto-downloaded. Its commercial/LGPL lane and module allowlist must be
decided before a redistributable Studio build. FFmpeg, Dawn/wgpu, OpenColorIO,
JUCE, plugin SDKs, updater, cloud telemetry, and AI models are intentionally not
in the foundation lock until their stage intake/qualification.

