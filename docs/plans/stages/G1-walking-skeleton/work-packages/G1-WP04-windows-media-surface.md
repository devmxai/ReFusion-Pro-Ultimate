---
id: G1-WP04
kind: work-package
status: blocked-awaiting-windows-device
gate: G1
owner_role: windows-media
evidence: docs/evidence/G1/G1-WP04.md
---

# Outcome

Admit the initial H.264 hardware Media Foundation profile on the same physical
adapter as the engine and compare the bounded D3D11-on-12/direct D3D12 surface
routes before selection.

# Required proof

Hardware-only MFT admission, adapter LUID equality, native surface and color
metadata, explicit bridge/copy/fence accounting, seek corpus, unsupported
diagnostics, and zero CPU-video-pixel counters.

# Kill criteria

Software MFT, WARP, cross-adapter route, CPU lock/map/conversion/upload, silent
fallback, or a D3D11/D3D12 bridge without bounded synchronization evidence.
