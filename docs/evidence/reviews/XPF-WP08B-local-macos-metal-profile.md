---
id: EVID-XPF-WP08B-MACOS-2026-08-09
kind: implementation-evidence
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP08
scope: bounded-macos-metal-preview-soak-observability-recovery
status: passed-bounded-preview-profile-export-and-xplat-not-qualified
date: 2026-08-09
---

# XPF-WP08B bounded macOS Metal profile receipt

## Outcome

The physical macOS Metal Preview path completed a 10,000-frame presenter soak
on the existing `MACOS-METAL-INTERACTIVE-640X360/Apple M1` tier:

```json
{"requested_frames":10000,"present_submissions":10000,"cpu_pixel_maps":0,"cpu_pixel_uploads":0,"gpu_readbacks":0,"unattributed_gpu_copies":0}
```

The separate joint GPU-observability fixture exercised hardware media
surfaces, the common Skia executor, Metal presentation, real completion fences,
thermal sampling, injected device loss and stale-generation rejection:

```json
{"device_tier":"MACOS-METAL-INTERACTIVE-640X360/Apple M1","resource_leases":15,"attributed_submissions":15,"attributed_copies":0,"attributed_conversions":0,"peak_resident_bytes":2089728,"fence_latency_max_ns":89967083,"thermal_state":"nominal","device_loss_events":1,"stale_generation_resources_accepted":0,"strict_path_clean":true}
```

The measured peak remained below the bounded 64 MiB resident-extent ceiling,
maximum fence latency remained below 2 seconds, and thermal state remained
better than the admitted `serious` ceiling. All 49 macOS Visual tests then
passed after the soak. The complete Core contract closure also passed 28/28
under AddressSanitizer and UndefinedBehaviorSanitizer, including the new
VisualOutput contract and cross-platform project/command/RenderPlan receipts.

## Exact-time defect found and corrected

The first 10,000-frame attempt exposed that the qualification executable sent
monotonically increasing timestamps beyond its 30-second Composition after the
first loop. Runtime correctly rejected the illegal sample with
`RFX-RENDER-PLAN-002`; it did not clamp, guess or render outside project time.
The test now wraps its requested exact time inside the declared 30-second
loop. No production clock or renderer relaxation was introduced.

## Claim boundary

This receipt qualifies only the bounded macOS Metal interactive Preview tier
and its current recovery/observability path. It does not qualify Offline
Export, encoded output, arbitrary canvas sizes, sustained thermal operation,
Windows/D3D12, Android/Vulkan, iOS product presentation or cross-backend pixel
equivalence. It cannot close XPF-WP08 or the cross-platform remediation plan.
