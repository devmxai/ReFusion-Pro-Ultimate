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

### G0-WP01 — Repository operating system

- Outcome: safe Git root, canonical docs, status, templates, Agent guidance and
  progressive-disclosure skill.
- Allowed paths: root docs/config, `docs/`, `.agents/`, `tools/rfdev.py`.
- Verification: docs doctor, skill validator, context manifest.
- Evidence: `docs/evidence/G0/G0-WP01.md`.

### G0-WP02 — Product, matrix, and legal decisions

- Outcome: review Product Contract and initial media/device matrix; decide Qt
  licensing lane and module allowlist; record codec/font/Skia obligations.
- Deliverables: ADRs and Dependency Intake records.
- Stop condition: commercial distribution obligations remain ownerless.

### G0-WP03 — Portable command/revision baseline

- Outcome: C++20 typed project snapshot, expected-revision command acceptance,
  deterministic rejection, and Last-Known-Good test/CLI proof.
- Allowed paths: `src/core`, `apps/cli`, `tests/unit`, CMake files.
- Verification: `cmake --workflow --preset macos-core` and Windows CI equivalent.

### G0-WP04 — Architecture and dependency enforcement

- Outcome: official pinned manifest, offline-normal-build rule, forbidden include
  scanner, dependency direction checks, docs freshness checks, CI design.
- Stop condition: scanner is treated as runtime evidence; it is structural only.

### G0-WP05 — G1 experiment readiness review

- Outcome: approve G1 devices, metrics, traces, fixtures, kill criteria,
  packaging lanes, and evidence schema.
- Exit: every G0 Master Plan criterion has evidence or an explicit rejection.

## Platform matrix

Portable core must build on macOS arm64 and Windows x64. iOS/Android compile
canaries are designed in G0 and activated in G1. No runtime platform claim arises
from compile-only evidence.

## Stage exit decision

An owner reviews evidence and marks G0 `passed`, `failed`, or `waived` criterion
by criterion. An Agent may prepare the record but cannot silently approve legal
or product decisions.

