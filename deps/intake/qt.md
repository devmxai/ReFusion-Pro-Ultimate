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
- `Qt6::QuickDialogs2` is limited to native project-folder and `Project.rfx`
  selection in the Studio shell; it receives no project, render, transport or
  media authority.
- Forbidden role: project model, media pipeline, transport, project Canvas,
  GPU ownership, video frames, export, or accepted-revision authority.
- License: Qt Commercial selected by ADR-0005. Commercial entitlement evidence
  remains mandatory before any redistributable artifact.
- G1 evidence: native engine viewport, clean package, runtime module census,
  license/SBOM bundle, macOS/Windows UI/accessibility smoke.

## Materialization policy

Development may use exact Qt 6.11.1 from the host and carries no distribution
claim. A verified machine-cache copy is still a host development SDK and carries
no distribution claim. A build with `REFUSION_RELEASE_BUILD=ON` fails closed
unless:

- the exact Commercial SDK is materialized below the ignored ReFusion-local
  `out/toolchains/qt-commercial` path;
- `REFUSION_QT_COMMERCIAL_SDK_ROOT` resolves to that SDK and CMake resolves Qt
  from the same prefix; and
- `REFUSION_QT_COMMERCIAL_RECEIPT` points to a private JSON receipt whose
  `status`, `license_lane`, and `qt_version` admit this exact build.

The receipt is an engineering gate, not legal proof by itself. Entitlement,
seats, CI, product/store coverage, SBOM and module census remain release evidence.

Qt 6.11 is currently supported on desktop/mobile configurations documented by
Qt, but ReFusion chooses a narrower product matrix and must qualify it itself.
