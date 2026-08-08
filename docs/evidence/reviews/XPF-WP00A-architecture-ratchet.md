---
id: EVID-XPF-WP00A-2026-08-08
kind: architecture-governance-evidence
status: passed
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP00A
platform: repository-policy
date: 2026-08-08
---

# XPF-WP00A architecture-ratchet evidence

## Delivered boundary

- `CURRENT.md` exposes `PLAN-XPLAT-FIX-001` through `active_guardrails`.
- `rfdev.py context` includes the guardrail path and digest in its read set.
- `docs-doctor` rejects unknown, inactive, duplicate or wrong-master guardrails.
- `architecture-check` scans native project semantics, common platform branches/
  includes/links and Studio FX/Mask vocabulary.
- The baseline contains 45 exact signatures and 85 occurrences. It is digest-
  pinned; active allowances are shrink-only and carry owner, removal package and
  expiry. New/wildcard/growing allowances fail.
- Negative policy fixtures prove native FX, common platform leakage, Studio FX
  dispatch, baseline mutation, new allowance and count growth are rejected.

## Verification

```text
python3 tools/rfdev.py context                         PASS; active guardrail loaded
python3 tools/rfdev.py docs-doctor                     PASS; 91 docs, 0 problems
python3 tools/rfdev.py architecture-check              PASS; 75 files, 0 problems
python3 tests/tools/rfdev_policy_test.py                PASS
cmake --workflow --preset macos-core                   PASS; 19/19 tests
```

## Claim boundary

This evidence changes repository governance only. It does not move renderer
code, change application pixels/runtime behavior or qualify Windows/iOS/Android.
`XPF-WP02` remains the next behavior-preserving renderer transition.
