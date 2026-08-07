# Active risks

| ID | Severity | Risk | Required response | Gate |
|---|---|---|---|---|
| RISK-001 | P0 | Qt commercial/LGPL/store obligations unresolved | Legal/module ADR before distribution commitment | G0 |
| RISK-002 | P0 | GPU ownership and Qt/Skia/native surface interop unproven | G1 kill-risk bake-off; no feature expansion before proof | G1 |
| RISK-003 | P0 | Windows hardware decode surface path may require explicit GPU bridge | Compare MF D3D surfaces/direct D3D12 with counters | G1 |
| RISK-004 | P1 | Strict hardware-only policy narrows usable media/device matrix | Publish matrix and fail closed; never hide software fallback | G1/G4 |
| RISK-005 | P1 | Mobile lifecycle/store rules conflict with desktop native extensions | Keep mobile declarative/packaged; separate product gate | G9 |
| RISK-006 | P1 | Early breadth delays install/export creator loop | Freeze Not-v1 and enforce gate outcomes | all |

