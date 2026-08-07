---
id: LEGAL-QT-001
kind: legal-gate
status: selected-commercial
owner_role: product-owner
canonical_for: qt-license-lane
last_verified: 2026-08-07
---

# Qt distribution gate

This is an engineering compliance record, not legal advice.

## Selected lane: Commercial

Selected by the product owner on 2026-08-07 for the intended proprietary
Desktop+iOS+Android product because it
avoids relying on LGPL relinking/store compatibility and provides access to
commercial LTS updates. Purchase terms, developer seats, product coverage,
mobile/store rights, CI usage and renewal remain owner/vendor matters.

No redistributable artifact may pass the release gate until commercial
entitlement evidence covers the exact Qt version, developers, CI, target
products and stores used by that artifact.

## Rejected alternative: LGPLv3 compliance

Before any distribution, all of the following must have named evidence:

- dynamically linked eligible Qt runtime where the platform permits;
- no GPL-only Qt modules or incompatible third-party parts;
- exact Qt source/modification package under ReFusion control;
- notices and complete LGPL text;
- relink and installation mechanism/instructions;
- user rights and reverse-engineering terms preserved;
- App Store, mobile static-link, signing and DRM compatibility reviewed;
- SBOM and module-level license audit for every shipped artifact;
- legal counsel sign-off.

## Version lanes

- Development: exact locally installed Qt 6.11.1, no shipping claim.
- Release candidate: choose at G1 toolchain freeze between a commercial LTS line
  and the then-supported feature line; rebuild and qualify all target platforms.
- Never mix open-source and commercial artifacts in one release lineage.

## Official references

- https://doc.qt.io/qt-6/licensing.html
- https://www.qt.io/development/open-source-lgpl-obligations
- https://doc.qt.io/qt-6/qt-releases.html
- https://doc.qt.io/qt-6/supported-platforms.html
