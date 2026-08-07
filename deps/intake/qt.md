---
id: DEP-QT-001
kind: dependency-intake
status: conditional
owner_role: studio-release
last_verified: 2026-08-07
---

# Qt 6 intake

- Official origin: https://download.qt.io/official_releases/qt/
- Development pin: 6.11.1.
- Role: native Studio window/control/accessibility/input/view-model shell only.
- Allowed runtime modules: `deps/policies/qt-modules.json`.
- Forbidden role: project model, media pipeline, transport, project Canvas,
  GPU ownership, video frames, export, or accepted-revision authority.
- License: blocked by ADR-0005 owner selection.
- G1 evidence: native engine viewport, clean package, runtime module census,
  license/SBOM bundle, macOS/Windows UI/accessibility smoke.

Qt 6.11 is currently supported on desktop/mobile configurations documented by
Qt, but ReFusion chooses a narrower product matrix and must qualify it itself.

