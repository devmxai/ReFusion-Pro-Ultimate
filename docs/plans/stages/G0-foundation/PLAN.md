---
id: G0
kind: stage-plan
status: active
master_plan: MP-001
owner_role: foundation-lead
last_verified: 2026-08-07
---

# G0 — Product and architecture contract

## Outcome

Create a reproducible repo operating system and portable core baseline that
makes the next GPU/media risk gate measurable, reviewable, and safe.

## Included

Product scope, invariants, repo boundaries, Master Plan, status/checkpoints,
decision templates, dependency intake/lock, CMake presets, portable command and
revision proof, docs/architecture enforcement, and G1 experiment design.

## Excluded

Production renderer/media/audio, shipping capability claims, broad UI work,
public plugin ABI, full dependency ecosystem, and signed public release.

## Work packages

Each package is a machine-checkable authority file under `work-packages/`:

1. [`G0-WP01`](work-packages/G0-WP01-repo-os.md) — Repository operating system
   — passed.
2. [`G0-WP02`](work-packages/G0-WP02-decisions.md) — Product, matrix, and legal
   decisions — passed.
3. [`G0-WP03`](work-packages/G0-WP03-portable-core.md) — Portable command and
   revision baseline — code complete, awaiting Windows.
4. [`G0-WP04A`](work-packages/G0-WP04A-authority-integrity.md) — Authority and
   boundary integrity — passed.
5. [`G0-WP04B`](work-packages/G0-WP04B-supply-chain.md) — Dependency/toolchain
   integrity — code complete, awaiting external gates.
6. [`G0-WP05`](work-packages/G0-WP05-g1-readiness.md) — G1 experiment readiness
   review — active.

## Platform matrix

Portable core must build on macOS arm64 and Windows x64. iOS/Android compile
canaries are designed in G0 and activated in G1. No runtime platform claim arises
from compile-only evidence.

## Stage exit decision

An owner reviews evidence and marks G0 `passed`, `failed`, or `waived` criterion
by criterion. An Agent may prepare the record but cannot silently approve legal
or product decisions.
