---
id: ADR-0011
kind: adr
status: proposed
title: Full-resolution Canvas raster and Fit presentation
owner_role: principal-cross-platform-architecture
decision_due: XPF-WINDOWS-DESKTOP-V1
last_verified: 2026-08-09
---

# Context

The viewport previously scaled the common Skia Canvas independently in X and Y
directly into the native swapchain. A small Fit viewport therefore changed the
rasterization resolution, distorted non-matching aspect ratios and allowed
backend sampling defaults to determine text, gradient and edge quality. A UI
percentage also conflated presentation zoom with render quality.

Composition coordinates already have one portable meaning:
`1 Composition Unit = 1 Composition Pixel`. DPI, Fit, zoom and pan are
presentation state and must not enter Project.rfx, exact-time evaluation or a
VisualRenderPlan semantic digest.

# Proposed decision

Adopt one common Canvas-preview policy:

- Studio defaults to Fit while always requesting full-resolution raster
  quality; Fit preserves the Composition aspect ratio and only determines the
  destination rectangle;
- when Fit reduces the image, the common Skia executor first renders the exact
  accepted VisualRenderPlan into a GPU-resident RGBA F16 Composition surface at
  the declared project dimensions;
- reduction uses reusable GPU surfaces, linear stages with no stage reducing
  either dimension by more than two, then one Mitchell cubic presentation pass
  through the shared final dither defined by ADR-0013;
- Canvas text uses device-independent font rasterization with unknown subpixel
  geometry, preventing platform display geometry from changing project text;
- Metal, D3D12 and future Vulkan bindings call the same executor and supply only
  native target mechanics; they do not select scaling, text, gradient, color or
  sampling semantics;
- native presenters bind one complete storage/transfer pair admitted by
  ADR-0013 and expose the selected profile through portable telemetry.

The Desktop-v1 SDR project and compositing contract in ADR-0010 remains
unchanged. The floating-point intermediate prevents early 8-bit quantization;
the interactive display prefers a linear-sRGB RGBA16F carrier and falls back
to BGRA8/sRGB without changing project semantics.

# Failure behavior

Composition and staged surfaces must remain GPU-resident. Invalid dimensions,
unsupported output color space, allocation failure or snapshot failure rejects
the frame with a typed diagnostic. There is no CPU project-pixel fallback,
readback, WARP route or platform-specific approximation.

Skia default mipmap generation is not part of this policy. The physical Intel
D3D12 candidate failed fast while creating that backend-dependent path. The
explicit staged reduction uses only the shared surface and sampling APIs and
passed the same D3D12 render fixture on the Intel adapter.

# Qualification

- Portable tests must prove aspect-preserving Fit, separation of raster quality
  from presentation mapping and Actual Pixels zoom 1.0 as a true centered
  one-source-pixel to one-physical-target-pixel crop.
- D3D12 and Metal integration fixtures must render the same semantic program at
  native and reduced target sizes and retain opaque output and foreground
  coverage.
- Windows qualification must run on a named non-WARP hardware adapter with no
  CPU project-pixel transfer.
- The shared Metal source and fixture must be rebuilt and physically rerun on
  macOS after this candidate lands. Windows success does not qualify Metal or
  preserve an older pixel reference automatically.

# Consequences

Fit cannot display all source pixels when a Composition is physically larger
than its on-screen destination; information loss is mathematically unavoidable.
This policy makes that loss deterministic and high quality, removes aspect
distortion and prevents low-resolution direct text rasterization. Studio exposes
Fit and Actual Pixels as presentation-only state; 100% means pixel mapping and
does not select a different project quality or backend renderer.
