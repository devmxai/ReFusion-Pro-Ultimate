---
id: G0-WP04B
kind: work-package
status: active
gate: G0
owner_role: dependency-release
evidence: docs/evidence/G0/G0-WP04B.md
last_verified: 2026-08-07
---

# Outcome

Make dependency and toolchain admission fail closed: verify the full Skia graph,
bind hydration to build records, distinguish spike/release profiles, and prevent
unapproved Qt artifacts from entering a redistributable build.

# Allowed paths

`deps/`, `tools/bootstrap.py`, `cmake/deps`, CMake presets/options, dependency
tests, G0 plan/status/evidence/checkpoints.

# Deliverables

- Full nested Skia origin/revision/clean verification before build/import.
- Dependency inventory digest and host/toolchain fingerprint in build records.
- ReFusion-local path enforcement for dependency sources/builds.
- Separate Skia release profile definition, unqualified until rebuilt/tested.
- Qt Commercial release gate requiring local SDK root and private entitlement receipt.

# Verification

Materialization verification, tamper-negative tests, current macOS graphics
workflow, and explicit rejection tests for missing release authority.

# Failure and handoff

Local provenance hardening may pass; Qt entitlement, clean independent rebuild,
Windows build, SBOM/notices, and signing remain external G1/release gates and may
not be fabricated or waived here.
