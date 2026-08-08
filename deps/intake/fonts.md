---
id: DEP-FONT-001
kind: dependency-intake
status: pinned-baseline-local-qualification
owner_role: text-rendering
last_verified: 2026-08-07
---

# Deterministic packaged-font intake

- Baseline Latin family: Noto Sans Regular 2.015 from official tag
  `NotoSans-v2.015` in `notofonts/latin-greek-cyrillic`.
- Baseline Arabic family: Noto Sans Arabic Regular 2.013 from official tag
  `NotoSansArabic-v2.013` in `notofonts/arabic`.
- Official distribution organization: https://github.com/notofonts
- License: SIL Open Font License 1.1. Each admitted archive's `OFL.txt` is
  extracted beside its font bytes and verified independently.
- Archive URLs, archive SHA-256 values, selected member paths and selected
  member SHA-256 values are frozen in `deps/manifest.lock.json`.
- `python3 tools/bootstrap.py sync-font <component> --fresh` is the only
  production materialization route. It downloads inside ReFusion, verifies the
  archive before extraction, admits only the named font/license members and
  then re-verifies their bytes.
- Skia profiles use the same bundled FreeType custom font manager on macOS and
  Windows for qualified packaged fonts. CoreText/DirectWrite/Android discovery
  remains available only for explicit unqualified system-font convenience.

These two regular faces are only the first qualification corpus, not the final
typography catalog. Weight/width/variation-axis intake remains a later explicit
addition. Arbitrary user/system fonts remain assets with availability/relink
behavior and may not qualify cross-platform layout. WP03 must test Arabic/RTL,
mixed scripts, diacritics, multiline, deterministic byte-backed fallback,
missing glyphs, preview/export parity and license-notice packaging.
