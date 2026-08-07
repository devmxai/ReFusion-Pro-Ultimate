---
id: ADR-0004
kind: adr
status: proposed
title: Strict hardware video and zero CPU pixel transfer
owner_role: media-architecture
decision_due: G0-WP02
last_verified: 2026-08-07
---

# Proposed decision

Production preview/composite/export supports only qualified hardware profiles.
Decoded video pixels may not enter a CPU software path. Unsupported profiles
fail closed with typed diagnostics; there is no hidden software fallback.

The measurable contract is zero CPU pixel map/readback/conversion/upload in the
admitted steady-state path. It does not prohibit CPU command logic, compressed
packet parsing, metadata, diagnostics, glyph work, or audio DSP.

