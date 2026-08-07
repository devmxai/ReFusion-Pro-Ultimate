---
id: ADR-0007
kind: adr
status: proposed
title: Packaged portable font baseline
owner_role: text-rendering
decision_due: G1
last_verified: 2026-08-07
---

# Proposed decision

Qualify pinned release binaries of Noto Sans and Noto Sans Arabic as reference
fonts under SIL OFL 1.1. Store font asset identity, version, content hash,
license and project-relative reference. System-font fallback may be convenient
but cannot establish cross-platform layout parity.

# Conditions

- Pin official tagged release assets and SHA-256 before download.
- Preserve OFL notices and reserved-name obligations.
- Test Arabic joining, RTL, diacritics, mixed Arabic/Latin, multiline layout,
  font fallback and export parity through the same Skia text stack.
- Emoji, CJK and additional scripts require separate size/license/quality intake.

