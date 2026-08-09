---
id: ADR-0012
kind: adr
status: proposed
title: Verified development machine dependency cache
owner_role: build-and-supply-chain
decision_due: G0-WP04B
last_verified: 2026-08-09
---

# Context

Each isolated checkout currently materializes the same pinned Skia dependency
graph, builds the same multi-gigabyte Skia profile and locates the same Qt SDK.
That is appropriate for independent qualification, but unnecessarily repeats
network and build work during ordinary feature development on one physical
host. Copying another checkout's mutable `out/`, using directory junctions, or
committing SDK binaries would weaken origin and artifact controls.

# Proposed decision

Add one development-only, content-addressed machine cache:

- the default root is a per-user cache directory and can be overridden by
  `REFUSION_MACHINE_CACHE_ROOT`; the Windows default is deliberately short to
  accommodate Skia/Dawn paths near the legacy `MAX_PATH` boundary;
- `tools/bootstrap.py machine-cache publish-skia` accepts only a compatible
  ReFusion checkout, re-verifies official Git origins, exact revisions, the
  tracked transitive lock, dependency inventory, profile, patch, GN arguments,
  host architecture and final artifact hash, then publishes an immutable cache
  identity;
- `publish-qt` verifies exact Qt version and the required module metadata before
  publishing a development SDK identity;
- CMake may consume external Skia/Qt paths only when the repository verifier
  resolves them from the cache index and re-verifies their identities;
- local `out/` materialization remains the first choice when present;
- Windows `-UseMachineCache` runs are explicitly non-qualifying. Fresh physical
  qualification and release gates retain their existing source, entitlement and
  evidence requirements.

The cache index stores only content identities and paths beneath its own root.
Path traversal, arbitrary external paths, stale schemas, source dirtiness,
revision drift, lock drift, profile drift and artifact tampering fail closed.

# Consequences

A dependency is downloaded and built once per compatible host/cache identity.
Later clones can configure and build without downloading or rebuilding Qt or
Skia. ReFusion source build trees remain checkout-local, and a changed pin,
profile, patch, transitive lock, architecture or artifact creates a cache miss
rather than silently reusing an incompatible object.

The machine cache is an acceleration mechanism, not repository truth, release
evidence or a substitute for reproducibility. `out/`, Qt/Skia binaries and
machine-cache receipts remain untracked. Promotion of this ADR requires the
same cache-miss, tamper-rejection and clean-clone reuse tests on macOS and
Windows.
