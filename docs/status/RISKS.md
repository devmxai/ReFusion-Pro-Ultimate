# Active risks

| ID | Severity | Risk | Required response | Gate |
|---|---|---|---|---|
| RISK-001 | closed | Qt distribution lane selected as Commercial | ADR-0005 accepted; prove commercial entitlement only at the redistributable RC gate | G6 |
| RISK-002 | P0 | macOS same-device Skia contexts proved; Windows, presentation and media-surface interop remain unproven | Complete G1 Windows/presenter/media bake-off before feature expansion | G1 |
| RISK-003 | P0 | Windows hardware decode surface path may require explicit GPU bridge | Compare MF D3D surfaces/direct D3D12 with counters | G1 |
| RISK-004 | P1 | Strict hardware-only policy narrows usable media/device matrix | Publish matrix and fail closed; never hide software fallback | G1/G4 |
| RISK-005 | P1 | Mobile lifecycle/store rules conflict with desktop native extensions | Keep mobile declarative/packaged; separate product gate | G9 |
| RISK-006 | P1 | Early breadth delays install/export creator loop | Freeze Not-v1 and enforce gate outcomes | all |
| RISK-007 | closed | Qt/CLI adapters could own mutable project authority and diverge from the accepted revision | Application Host boundary and negative architecture policy passed at `24d5946e442de09ac9ccc798f9e7aeedeee04502` | G0 |
| RISK-008 | P1 | Qt Commercial SDK/entitlement is intentionally unverified during engineering | Keep redistributable release configuration fail closed; request evidence only at G6 RC admission | G6 |
| RISK-009 | P1 | Local dependency/build materialization can exhaust the development disk | Keep sources/builds repository-local, measure before hydration, and prune only reproducible ignored outputs | all |
