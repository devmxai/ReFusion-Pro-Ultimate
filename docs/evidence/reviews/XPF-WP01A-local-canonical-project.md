---
id: EVID-XPF-WP01A-LOCAL-2026-08-08
kind: architecture-implementation-evidence
status: passed
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP01A-local
platform: portable-core-plus-appleclang
date: 2026-08-08
---

# XPF-WP01A local canonical-project evidence

## Implemented contract

Core now owns one locale-independent textual primitive contract used by RFX,
schema/project/text/render-plan digests and Agent JSON floating-point output:

- finite `binary64` uses `to_chars(general, max_digits10)` and both IEEE zero
  encodings serialize as `0`;
- the existing registry schema digest keeps its six-decimal compatibility
  spelling through explicit `to_chars(fixed, 6)`;
- unsigned and fixed-width hexadecimal receipts use direct ASCII conversion;
- entity IDs use a case-sensitive portable ASCII grammar with no host paths;
- project strings must be well-formed UTF-8, reject NUL/C0/C1/DEL except
  TAB/LF/CR, and preserve authored NFC/NFD bytes without silent normalization;
- locale-sensitive whitespace/case helpers in the touched Core paths were
  replaced by explicit ASCII rules.

RFX rejects malformed encoding and prohibited controls with typed diagnostics
before accepting a snapshot. Programmatic project validation applies the same
contract before publication, and serialization fails closed.

## Cross-toolchain receipt

`tests/fixtures/render-plan/xplat-visual-v1/expected-project.txt` binds the
existing Arabic/Latin visual corpus to:

```text
snapshot_digest=rfx-project-fnv1a64:3a78631fbbbf20c9
registry_digest=rfx-vp-fnv1a64:f6f8d1b88fe2d101
revision=12
```

The `refusion.xplat_project_conformance` test compiles the real RFX4 fixture,
checks these receipts, canonical-reserializes and recompiles it, then repeats
serialization and all digests under a synthetic decimal-comma/grouped-number
global C++ locale. The bytes and receipts remain identical.

## Verification

```text
cmake --build --preset macos-visual             PASS
ctest --preset macos-visual                     PASS; 44/44
refusion.canonical_text                         PASS
refusion.xplat_project_conformance              PASS
refusion.xplat_render_plan_conformance          PASS
python3 tools/rfdev.py docs-doctor              PASS; 94 documents
python3 tools/rfdev.py architecture-check       PASS; 83 source files
python3 tests/tools/rfdev_policy_test.py         PASS
git diff --check                                PASS
```

## Claim boundary

This closes the locally executable WP01A slice only. The same receipt has not
run under MSVC, and canonical authored-coordinate quantization, command-output
receipts, Mac/Windows canonical resave, copied-workspace regeneration and
mobile compile canaries remain open. No cross-platform qualification is
inferred from AppleClang.
