---
id: ADR-0010
kind: adr
status: proposed
title: Desktop v1 SDR color and compositing contract
owner_role: principal-cross-platform-architecture
decision_due: XPF-PRE-WINDOWS-READY
last_verified: 2026-08-09
---

# Context

The common compositor already uses BGRA8 targets, sRGB-tagged Skia surfaces,
straight-sRGB gradient interpolation and premultiplied Skia compositing. Those
choices were previously distributed across implementation calls. Metal and
D3D12 could therefore retain the same project and RenderPlan digest while an
API default changed gradient, alpha, filter edge or output meaning.

Desktop v1 needs one explicit bounded SDR profile before macOS reference
captures can qualify Windows. HDR, wide gamut and linear-light authoring remain
later versioned profiles rather than implicit changes to existing projects.

# Proposed decision

Adopt `refusion.color.desktop-v1-sdr.v1` with these exact semantics:

- project colors are straight-alpha RGBA8 values encoded with the sRGB transfer
  function and Rec.709/sRGB primaries at D65;
- the common compositor uses premultiplied alpha and an sRGB-encoded blend and
  image-filter working space for this compatibility profile;
- gradients interpolate straight sRGB components with clamp tiling;
- effect/filter samples outside admitted isolation bounds use transparent-decal
  edges; bounds/crops remain explicit RenderPlan semantics;
- Preview/qualification targets are BGRA8 UNORM carrying sRGB output meaning;
- Metal and D3D12 create/wrap native targets only and do not choose or mutate
  these semantics.

The paired qualification policy is
`refusion.xplat-pixel-tolerance.desktop-v1.v1`. A Windows candidate must have
the same 640x360 dimensions, maximum RGB channel delta no greater than 8,
mean absolute channel delta no greater than 0.75, no more than 0.5% of pixels
with any channel delta above 3, and global luminance SSIM of at least 0.995.
These bounds are a versioned qualification decision, not a backend hint:
exceeding one fails the profile and cannot be overridden by a platform-only
semantic patch.

The canonical contract bytes have SHA-256
`50a4acc6cc8d7092b5aa10d4f70bc24aa93aaf4e71413617c5ec297e5547af78`.
Core owns the descriptor and digest, `VisualRenderProgram` binds them during
candidate preparation, every `VisualRenderPlan` carries and hashes them, and
the common Skia compositor rejects an unknown or changed contract.

# Qualification

- AppleClang and MSVC must emit the same color-contract and RenderPlan digests.
- The shared visual corpus must bind the exact contract digest and compare
  Metal/D3D12 captures using declared channel, alpha/edge and structural
  tolerances.
- Production Preview retains zero readback; pixel capture remains test-only.
- Preview and Offline qualification render separate GPU targets through the
  same accepted `VisualRenderProgram`, exact time, output-frame preparation and
  common Skia compositor; their pixels must be exact on one backend.
- A new HDR, linear-light or wide-gamut profile requires another descriptor,
  migration/capability policy, conformance corpus and ADR revision.

# Consequences

The profile intentionally preserves the current bounded SDR appearance rather
than silently switching existing content to linear-light blending. Professional
linear/HDR work can be added later as a distinct capability. Until this ADR is
accepted by the product owner, the implementation is a source-complete
candidate and cannot close `XPF-PRE-WINDOWS-READY` or advance either Desktop
profile to color-qualified.
