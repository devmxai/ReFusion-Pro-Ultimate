---
id: ADR-0013
kind: adr
status: proposed
title: High-precision SDR display presentation carrier
owner_role: principal-cross-platform-architecture
decision_due: XPF-WINDOWS-DESKTOP-V1
last_verified: 2026-08-09
---

# Context

ADR-0010 defines the accepted Desktop-v1 SDR project, compositing and BGRA8
qualification meaning. That semantic contract does not require the interactive
desktop swapchain to quantize the already-composited image to eight bits before
the operating-system display pipeline. On dark gradients, early quantization
and a fixed ordered dither can expose bands or a repeating screen-space pattern,
especially on an internal 6-bit-plus-FRC panel.

The storage and transfer function of a native presentation target are runtime
display capabilities. They must not enter Project.rfx, exact-time evaluation,
the VisualRenderPlan digest, gradient interpolation or blend semantics.

# Proposed decision

Admit two portable SDR presentation-target profiles:

- preferred: RGBA16 float storage carrying linear sRGB/Rec.709 values;
- fallback: BGRA8 UNORM storage carrying sRGB-encoded values.

The native presenter probes and binds the first complete profile supported by
the device, swapchain and output color-space API. D3D12 uses
`R16G16B16A16_FLOAT` with
`DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709`; Metal uses `RGBA16Float` with an
extended-linear-sRGB CAMetalLayer color space. If the complete high-precision
pair is unavailable, both use the admitted BGRA8/sRGB fallback. The chosen
profile is carried in the backend target lease and exposed in presentation
telemetry so the common renderer cannot guess from a native pointer.

The common Skia executor always rasterizes the Composition into a GPU-resident
RGBA F16 surface at project resolution. Its final presentation pass preserves
the accepted sRGB-encoded Desktop-v1 compositing result, applies one shared
non-periodic temporal dither with a peak amplitude below one-half of an 8-bit
encoded step, and then writes either linear-sRGB F16 or encoded-sRGB BGRA8.
Scaling, dither and transfer conversion remain common Skia behavior; Metal and
D3D12 supply only native target mechanics.

If accepted, this decision narrows ADR-0010's BGRA8 Preview-target sentence to
the semantic qualification target. It supersedes only the storage and transfer
carrier used by the interactive desktop display; the project color contract,
RenderPlan digest, reference corpus and offline semantic graph remain unchanged.

# Failure behavior

An unsupported storage/transfer pair is invalid even if each enum value is
known independently. A presenter that cannot create either complete profile
rejects the frame. There is no CPU pixel copy, software color conversion, WARP
route or backend-specific scaling/dither fallback.

# Qualification

- Portable tests validate the two admitted profile pairs, their byte sizes and
  rejection of mixed pairs.
- D3D12 and Metal fixtures create real RGBA16F render targets and execute the
  same common Skia program used by BGRA8 qualification.
- Presenter tests require a valid selected profile, hardware GPU submission,
  resize and safe target retirement without device loss.
- Fit and Actual Pixels tests prove that 100% is a centered one-source-pixel to
  one-physical-target-pixel crop, while Fit retains full-resolution raster and
  aspect-preserving reduction.
- Physical qualification records the selected profile and checks that the
  final dither has no fixed 8x8 or directional correlation peak. Metal source
  changes must still be built and physically rerun on macOS.

# Consequences

The operating system receives more precision before its monitor transform and
final output quantization. This removes application-side 8-bit band boundaries
and fixed ordered patterns from the preferred path, while keeping a deterministic
cross-platform fallback.

This cannot turn a 6-bit/FRC panel into a native high-bit-depth display, remove
all panel electronics artifacts or guarantee identical physical appearance on
uncalibrated monitors. It does make the application's raster, scaling, transfer
and quantization boundaries explicit, observable and shared across platforms.
