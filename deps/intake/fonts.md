---
id: DEP-FONT-001
kind: dependency-intake
status: candidate
owner_role: text-rendering
last_verified: 2026-08-07
---

# Reference font intake

- Candidate families: Noto Sans and Noto Sans Arabic.
- Official distribution organization: https://github.com/notofonts
- License: SIL Open Font License 1.1 for the font binaries.
- No font binary is downloaded until a tagged release asset and SHA-256 are
  recorded in the dependency lock.

Packaged reference fonts provide reproducible layout; arbitrary user/system
fonts remain assets with explicit availability/relink behavior. G1 must test
Arabic/RTL shaping, mixed scripts, diacritics, multiline, weight axes, fallback,
missing glyphs, preview/export parity and license notice packaging.

