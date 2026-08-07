---
id: G0-WP03
kind: work-package
status: code-complete-awaiting-external-gate
gate: G0
owner_role: portable-core
evidence: docs/evidence/G0/G0-WP03.md
last_verified: 2026-08-07
---

# Outcome

Prove the portable C++20 command/revision/LKG baseline on macOS arm64 and
Windows x64.

# Dependencies

G0-WP01 and G0-WP02 passed; a CI-capable remote or Windows x64 runner is
required for the remaining native gate.

# Allowed paths

`src/core`, `src/application`, `apps/cli`, `tests/unit`, portable CMake/CI,
evidence and status files.

# Deliverables

Typed identity/envelope/receipt contracts, deterministic rejection/replay,
one Application command-service boundary, and macOS/Windows Core evidence.

# Verification

`cmake --workflow --preset macos-core` and the committed Windows CI equivalent.

# Failure and handoff

Mac code is green. Do not close this package until a real Windows run URL,
runner image, compiler fingerprint, and passing test receipt are retained.
