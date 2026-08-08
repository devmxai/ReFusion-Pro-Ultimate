---
id: EVID-XPF-WP02B-LOCAL-2026-08-08
kind: architecture-implementation-evidence
status: passed
plan: PLAN-XPLAT-FIX-001
work_package: XPF-WP02B-local
platform: portable-corpus-plus-macos-metal
date: 2026-08-08
---

# XPF-WP02B local conformance evidence

## Portable corpus

`tests/fixtures/render-plan/xplat-visual-v1/Project.rfx` passes through the real
RFX4 compiler, Core exact-time evaluator and Runtime RenderPlan compiler. Its
checked-in receipts cover frames 0, 30, 60 and 119 and fail on changes to:

- root/Group traversal and animated transforms;
- Solid, Linear Gradient, Radial Gradient and Text operations;
- Normal, Multiply, Screen and Overlay blends;
- normal and inverted ordered rounded Masks;
- strokes, corners and bounded isolation;
- Blur -> Shadow -> Glow ordering.

The digest encodes primitives explicitly in little-endian order, never native
object padding. A deterministic test TextLayout result isolates RenderPlan
semantics from the still-open packaged-font qualification package.

## macOS visual qualification

The Metal fixture test renders the same RFX project through `SkiaCommon` at
frame 60. A test-only shared target and same-queue completion barrier permit a
bounded pixel read. It verifies full opacity, minimum non-background coverage,
minimum colorful/highlight coverage and an exact unaffected background corner.
No production target, presenter or observability counter gains a readback path.

## Verification

```text
cmake --build --preset macos-visual             PASS
ctest --preset macos-visual                     PASS; 42/42
refusion.xplat_render_plan_conformance          PASS
refusion.skia_fixture_renderer                  PASS; Apple M1 Metal
python3 tools/rfdev.py docs-doctor              PASS; 93 documents
python3 tools/rfdev.py architecture-check       PASS; 81 source files
python3 tests/tools/rfdev_policy_test.py         PASS
git diff --check                                PASS
```

## Claim boundary

The portable corpus is ready to run under MSVC but Windows is `not-run` in this
environment. This evidence does not qualify D3D12/DXGI pixels, packaged fonts,
mobile backends or preview/export parity. It closes only the locally executable
WP02B slice.
