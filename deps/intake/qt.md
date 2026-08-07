---
id: DEP-QT-001
kind: dependency-intake
status: accepted-development-dependency
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
- License: Qt Commercial selected by ADR-0005. Commercial entitlement evidence
  remains mandatory before any redistributable artifact.
- G1 evidence: native engine viewport, clean package, runtime module census,
  license/SBOM bundle, macOS/Windows UI/accessibility smoke.

Qt 6.11 is currently supported on desktop/mobile configurations documented by
Qt, but ReFusion chooses a narrower product matrix and must qualify it itself.
