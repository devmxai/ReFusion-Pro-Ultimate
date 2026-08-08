---
id: G1-WP05
kind: work-package
status: passed-bounded-macos
gate: G1
owner_role: gpu-runtime
evidence: docs/evidence/G1/G1-WP05.md
---

# Outcome

Introduce typed device generation, presenter/resource/fence leases, device-loss
state, and runtime counters/traces shared by Skia, media and presentation proofs.

# Delivered bounded slice

- Portable Runtime owns one process-local `GpuObservabilityService`; it owns no
  device, queue, clock, scheduling, revision or presentation authority.
- Typed RAII resource/fence leases track native video surfaces, Skia render
  contexts, CAMetalLayer/drawables and real asynchronous completion fences.
- Every VideoToolbox, Skia flush and Metal presentation submission carries a
  stable non-zero attribution ID. Copy and conversion observations are explicit
  operations rather than inferred from frame counts.
- The service rejects mismatched adapter/generation admission, advances only on
  a typed device-loss generation, and records stale-generation rejection.
- A named qualification budget records actual resident-byte peak, fence
  latency and platform thermal state. Budget results are valid only for the
  named tier; they cannot qualify a different device tier.
- One shared observer is injected through portable factories into the Apple
  hardware decoder, Skia Metal context and native Metal presenter. The physical
  proof uses the real B-frame/VFR fixture and real GPU completion handlers.

# Acceptance result

The bounded macOS Metal lane passes all absolute thresholds and the named
`MACOS-METAL-INTERACTIVE-640X360/Apple M1` qualification budget. Portable
contracts remain backend-neutral; Windows runtime evidence is `not-run` and
still blocks cross-platform G1 exit.

# Absolute admission thresholds

- CPU video-pixel map/readback/conversion/upload: exactly zero.
- Software decoder, WARP, silent fallback and cross-adapter events: exactly zero.
- Unattributed GPU copy/conversion or submission: exactly zero.
- Stale-generation resource accepted after device loss: exactly zero.

Latency, memory and thermal budgets are recorded per named device tier; no
aggregate cross-device number can qualify a platform.

# Claim boundary

This closes G1-WP05 only for the bounded macOS walking proof. It does not
qualify Windows/D3D12, iOS, Android, production media import, general memory
budgeting, sustained thermal soak, export or G1 exit.
