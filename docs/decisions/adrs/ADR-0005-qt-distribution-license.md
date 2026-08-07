---
id: ADR-0005
kind: adr
status: accepted
title: Qt distribution license and release line
owner_role: product-owner
decision_due: G0-WP02
blocking: false
last_verified: 2026-08-07
---

# Context

ReFusion is intended to be proprietary and commercially distributed on desktop
and later through mobile stores. The Qt Company offers commercial and open-source
licensing, and warns that LGPL obligations include source availability,
relinkability, notices, reverse-engineering rights for debugging modifications,
and possible store-rule conflicts. Some modules are GPL-only for open-source use.

Qt 6.11.1 is installed and supported through 2027-03-17. Qt 6.8 LTS commercial
updates are listed through 2029-10-08. The release line and legal lane are related
but distinct decisions.

# Decision

The product owner selected **Qt Commercial** on 2026-08-07 for proprietary
Desktop+iOS+Android distribution. Compare the supported LTS line with the
current feature line at the G6 release-candidate toolchain freeze. Continue
engineering on exact Qt 6.11.1 without claiming redistribution rights.

Selection of the commercial lane is not proof that a commercial subscription,
the required developer seats, CI rights, or product/store coverage have been
purchased. Release packaging remains fail-closed until that entitlement evidence
is attached to the release gate.

The product owner explicitly deferred SDK/entitlement verification during G0/G1.
This changes timing only: development may continue, while redistributable release
admission remains fail closed.

# Rejected alternative: LGPLv3 compliance program

LGPL is technically possible only if the owner accepts and implements all legal
obligations, including dynamic linking where applicable, controlled Qt source
delivery, relink/install instructions, notices, module-by-module license and
third-party review, and store compatibility review. No GPL-only module may enter
the proprietary product. This requires legal counsel and continuous compliance.

# Prohibited outcome

Do not mix open-source and commercial Qt artifacts in one application/device,
and do not ship while the lane is `unselected`.

# Consequences

- The Qt module allowlist remains narrow and independently enforced.
- No open-source Qt artifact may be mixed into the commercial release lineage.
- A later version-line decision does not reopen the licensing lane; changing the
  lane requires a superseding ADR and a new compliance review.
