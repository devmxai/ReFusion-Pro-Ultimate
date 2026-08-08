---
id: EVID-XPF-WP03A-LOCAL-2026-08-08
kind: implementation-evidence
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP03
scope: macos-local
status: passed-local-not-cross-platform-qualified
date: 2026-08-08
---

# XPF-WP03A local deterministic-font receipt

## Outcome

ReFusion's qualified text path no longer depends on a host-installed font.
Project workspaces carry official immutable Latin and Arabic baseline font
bytes, and the common Skia layout path constructs typefaces from those exact
digest-verified bytes. Platform font managers remain available only through an
explicitly unqualified adapter boundary.

This receipt proves the local AppleClang/macOS implementation. It does not
claim formal cross-platform WP03 exit because the exact same receipt has not
yet run under MSVC/Windows.

## Pinned inputs

| Input | Official release | Archive SHA-256 | Selected font SHA-256 |
|---|---|---|---|
| Noto Sans | `NotoSans-v2.015` | `0c34df072a3fa7efbb7cbf34950e1f971a4447cffe365d3a359e2d4089b958f5` | `f5f552c8c5edb61fe6efb824baf4d4de47b1a8689ab4925ff43f7bd6a4ebece5` |
| Noto Sans Arabic | `NotoSansArabic-v2.013` | `1301aceaea84c501cf2e6dcfb3182e2328c8eae5725817fcb239672bda7154f1` | `7ed3fe069312aceac454f17cf613a30f95271d6ed7ce58005ed4d016bd3823d7` |

Both inputs use OFL-1.1 notices whose selected-member digests are recorded in
`deps/manifest.lock.json`. Bootstrap verified the extracted members before the
offline configure/build accepted them.

## Implemented contract

- Core owns a dependency-free `sha256:` content digest and path-free font asset
  request/result port.
- Studio resolves only canonical project-relative
  `Assets/Fonts/<AssetId>/font.ttf` bytes and validates their digest.
- New workspaces transactionally include both fonts, both license notices and a
  digest-bound font catalog.
- Qualified Skia layout uses a byte-backed custom embedded FreeType manager,
  HarfBuzz shaping, ICU line breaking, cluster-safe spacing and no hinting.
- The layout receipt binds font digest, face, style, fixed shaping inputs,
  primary-only fallback policy and engine configuration.
- System font providers are compiled as separate OS adapters and are explicitly
  unqualified; missing or mismatched assets fail closed.
- The common GPU composition receives the same resolver as Inspector/CLI text
  measurement, so Canvas and measurement cannot silently select different font
  bytes.

## Verification

Commands executed from the repository root:

```text
python3 tools/rfdev.py architecture-check
  checked_source_files: 97
  problems: []

cmake --preset macos-visual
cmake --build --preset macos-visual --parallel 8
  passed

ctest --preset macos-visual --output-on-failure
  47/47 passed
```

The exact fixture at `tests/fixtures/fonts/xplat-noto-v1/expected-layout.txt`
covers Latin, Arabic, RTL, mixed text, diacritics and wrapping with fixed glyph
IDs/positions, line metrics and baselines.

## Remaining formal exit work

1. Run the exact font/layout receipt under MSVC/Windows and compare it without
   regenerating expected output.
2. When a general authored fallback chain is introduced, store every fallback
   identity/digest and shaping input in portable project truth and the layout
   digest. Never consult an implicit host fallback.
3. Add the same corpus to mobile contract canaries; physical mobile product
   qualification remains a later gate.

Therefore the truthful state is: local WP03A passed; formal XPF-WP03 remains
open and Windows/mobile evidence remains `not-run`.
