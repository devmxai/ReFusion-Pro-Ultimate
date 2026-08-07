---
id: EV-G0-WP04B
kind: evidence-record
gate: G0
work_package: G0-WP04B
status: local-controls-passed-external-gates-pending
source_commit: 24d5946e442de09ac9ccc798f9e7aeedeee04502
date: 2026-08-07
---

# Dependency and toolchain integrity evidence

## Delivered

- Exact macOS development toolchain qualification and explicit pending Windows
  qualification records.
- Tracked macOS Skia transitive lock containing all 46 dependency origins,
  revisions and clean-state requirements.
- Hydration schema 2 with atomic writes; full nested origin/revision/dirty and
  GN/Ninja digest verification before Skia build.
- Skia build record schema 2 binding root source, GN profile, generated inventory,
  tracked lock, host, compiler/tools, component archives and final artifact.
- CMake rejects dependency paths outside ReFusion and reruns full materialization
  verification before importing Skia.
- A separate `is_official_build=true` macOS release profile is defined but
  correctly unqualified and unbuilt.
- Release Studio configuration rejects missing/out-of-tree Qt Commercial SDK and
  missing or mismatched private entitlement receipt.
- Windows Skia configuration fails explicitly because a real same-device
  D3D/Dawn context implementation does not yet exist.

## Current verified materialization

- Skia revision: `294d31e0b1aa295d585836ab41bd2fba170e0c5d`.
- Tracked dependency count: 46.
- Generated dependency record SHA-256:
  `c8a42bbf008fae7e0d681dc092ad581ad0e0b4b502633823117a54ab6c874828`.
- Tracked macOS lock SHA-256:
  `81c192af6d5db893dbb5221b27254deccab91143936adc1aedf1ab667beaf56c`.
- Skia bundle SHA-256 remained:
  `f4bda2fdd752cece9983ee90863b1e1d9dac512b23f24948786cc67be95576f9`.

## Negative gates proved

- Qt release configure without admitted Commercial SDK authority: rejected.
- Skia source/build path outside ReFusion: rejected.
- Tampered Skia DEPS digest: rejected.
- Normal materialization verification and macOS graphics import: accepted.

## External gates still required

Qt Commercial SDK/entitlement, Windows transitive lock/build/link/runtime,
independent clean release rebuild, SBOM/license notices, signing and clean-machine
receipts. None is claimed or silently waived by this local evidence. The Qt
commercial receipt is explicitly deferred to G6 and does not block G0/G1
technical engineering.
