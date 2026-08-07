---
id: STATUS-001
kind: program-status
master_plan: MP-001
master_plan_status: active
current_gate: G0
gate_status: active
active_work_packages:
  - G0-WP03
  - G0-WP04B
  - G0-WP05
  - G1-WP03
baseline_commit: 1a70f5a
last_green_commit: ac8de22c329a64a9f351796ae5ded280e701984c
last_checkpoint: CP-G1-0004
blocking_risks:
  - RISK-002
  - RISK-003
last_updated: 2026-08-07
---

# Current program status

## Exact resume point

`G0-WP02` passed after the product owner selected Qt Commercial in ADR-0005.
`G0-WP03` Core code and CI definition remain complete but still need a real
Windows x64 run. By explicit owner direction, checkpoint `CP-G0-0004` also
materialized a fresh official Skia dependency graph inside ReFusion and proved
the engine-owned Metal plus same-device Ganesh/Graphite contract on macOS.
Checkpoint `CP-G0-0005` passed `G0-WP04A`: one private Application Host now
owns mutable project authority, while Studio and CLI submit commands through the
same service. The local controls in `G0-WP04B` are code complete and fail closed;
Windows materialization/build/runtime still requires external evidence. By the
product owner's direction in `CP-G0-0006`, Qt Commercial SDK and entitlement
verification is explicitly deferred to the redistributable release-candidate
gate and does not block G0/G1 engineering. Checkpoint `CP-G0-0007` accepts
cross-platform-first implementation with macOS-only physical runtime evidence
while no Windows device is available. `G1-WP01` is active for the first real
visual experience on macOS; Windows remains `not-run`, and neither G0 nor G1 may
pass from the macOS evidence alone.

Source commit `4c91df7f6abf0734a47cfb549a726d15ef35281e`
delivered the first real visual application experience: the Qt shell embeds an
engine-owned CAMetalLayer, Skia renders a GPU-backed Arabic/Latin Text/Shape
fixture on the same Metal device/queue, and a 10,000-frame presenter soak
retained zero CPU pixel maps, uploads, readbacks, or unattributed GPU copies.
`G1-WP01` remains active for physical sleep/wake, occlusion and device-loss
qualification; this visual slice does not pass Windows or G1.

By explicit product-owner direction, source commit
`b850a84ce8aff860bf8aac786440b396eb77024e` made the macOS experience a
file-backed project-open test instead of a hard-coded visual mock. The app now
opens a validated 30-second 1080x1920 Reels Composition with stable Layer IDs,
exact integer time and keyframed Shape/Text content; Runtime owns continuous
30 fps looping and the UI only displays telemetry/snapshot metadata. Proposed
ADR-0008 and the experimental schema record the boundary. This is a G2 seed,
not G2 activation or a Video Layer/media/save/export claim.

Source commit `73d59a7960bae38ba7e2e9c41ae2df4898c8f565` hardened the
GPU lifecycle contract across the declared Metal and D3D12 lanes. macOS now
observes native system sleep/wake and window occlusion, presentation reports
typed health/generation telemetry, loss advances the generation, and Runtime
stops fail-closed on its first rejected frame. Physical full-window
occlusion/resume and injected device-loss receipts pass with zero CPU pixel
transfer. The product owner then reported a successful manual sleep/wake test
and explicitly made retention of the automated physical receipt non-blocking
for current development. `G1-WP01` is therefore accepted for the bounded macOS
walking runtime, while its automated sleep/wake receipt remains deferred and
must not be represented as lab-qualified evidence.

By the same cross-platform-first/macOS-runtime-only direction, `G1-WP03` is now
active for the Apple H.264 hardware-media surface proof. Its semantic contracts,
strict counters and failure model must remain portable and a matching Windows
build lane must exist from the first slice. Windows runtime qualification and
G1 exit remain blocked until a physical Windows device exists.

Source commit `ac8de22c329a64a9f351796ae5ded280e701984c` delivered the
first G1-WP03 slice. Portable Runtime now owns exact source-frame timing, the
narrow H.264/NV12/SDR Rec.709 profile, typed capability outcomes and strict
zero-CPU/fallback/cross-adapter counters. The Apple adapter proved that H.264
hardware decode capability exists and that a two-plane NV12 native surface can
bind to Metal textures on the engine-owned adapter/generation. The matching
Windows media lane exists but deliberately returns not-qualified until G1-WP04
runs on Windows. This slice does not yet decode a compressed H.264 sample.

## Read next

1. `docs/plans/stages/G0-foundation/PLAN.md`
2. `docs/architecture/INVARIANTS.md`
3. `docs/product/PRODUCT_CONTRACT.md`

## Next actions

1. Continue `G1-WP03` with a repository-owned bounded H.264 sample corpus and a
   hardware-required VideoToolbox decompression session. Prove output PTS/color,
   engine surface lease/lifetime and two-plane same-device Metal binding without
   CPU pixel mapping.
2. Keep every new semantic/media contract portable and define the matching
   Windows lane/fixture; native code may enter Apple/Windows adapters only.
3. Retain an automated physical `G1-WP01` sleep/wake receipt when convenient;
   it is deferred, not silently converted into lab-qualified evidence.
4. When a Windows x64 runner/device becomes available, run G0-WP03, create the
   repository-local Windows Skia lock, then execute G1-WP02 D3D12/DXGI proof.
5. Complete G0-WP05 and the G0/G1 cross-platform exit reviews only after the
   missing Windows receipts exist.
6. Keep the experimental project schema bounded until G2 formally decides
   persistence, migrations, journals, ChangeSets and save/reopen semantics.


## Do not repeat

- Do not turn the research draft into an implementation plan.
- Do not reuse, search for, or copy any external Skia checkout or machine cache;
  only the ReFusion-local clean bootstrap path is admitted.
- Do not expand beyond the explicitly admitted macOS `G1-WP03` hardware-media
  proof while G0 Windows evidence is pending.
- Do not repeat Qt/Skia/media/font intake research; use ADR-0005 through ADR-0007.
- Do not reopen the Qt licensing lane without a superseding ADR.
- Do not request or claim Qt Commercial SDK/entitlement evidence during G0/G1;
  preserve the fail-closed release gate for the later redistributable RC.
- Do not re-implement the Application Host boundary or the macOS Skia lock; use
  source commit `24d5946e442de09ac9ccc798f9e7aeedeee04502`.
- Do not interpret macOS runtime success as Windows/iOS/Android qualification;
  their state remains `not-run` until platform evidence exists.
- Do not call the 30-second Shape/Text project a decoded video or stable project
  format; actual Video/Audio import and transport remain G4.
- Do not call the G1-WP03 capability/native-surface probe an actual decoded
  frame; no compressed sample has entered VideoToolbox yet.
