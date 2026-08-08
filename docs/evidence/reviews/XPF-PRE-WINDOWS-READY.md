---
id: XPF-PRE-WINDOWS-READY
kind: implementation-evidence
plan: PLAN-XPLAT-FIX-001
work_package: pre-windows-source-closure
scope: accepted-desktop-v1-common-source-and-macos-reference
status: passed-source-ready-windows-and-android-execution-not-run
date: 2026-08-09
source_commit: 57d000fc51b7156e08c362f8b04979b4aee5b3fe
---

# Pre-Windows source readiness review

## Outcome

Phase A of the two-host cross-platform route is closed. Commit
`57d000fc51b7156e08c362f8b04979b4aee5b3fe` contains the accepted Desktop-v1
SDR contract, one portable Core/RenderPlan/Skia semantic route, thin Metal and
D3D12 mechanics, the fixed cross-toolchain corpus, the macOS reference capture
and the Windows build/qualification entrance. The product owner physically
reviewed the macOS application and accepted ADR-0010 on 2026-08-09.

This record means **source-ready for the Windows host**. It does not claim that
MSVC compiled the source, that the Windows Skia closure linked, or that a
physical D3D12 GPU produced matching pixels.

## Accepted contracts and immutable inputs

| Input | Bound identity |
|---|---|
| Source commit | `57d000fc51b7156e08c362f8b04979b4aee5b3fe` |
| Desktop color contract | `refusion.color.desktop-v1-sdr.v1` / semantic SHA-256 `50a4acc6cc8d7092b5aa10d4f70bc24aa93aaf4e71413617c5ec297e5547af78` |
| Pixel comparison policy | `refusion.xplat-pixel-tolerance.desktop-v1.v1` |
| Canonical project | SHA-256 `d0181f7a9a3399dbf7827942de6d52f7d2f5a7da213a26959b8a0e37202e575b` |
| Command receipt | SHA-256 `42627f982c8cd6134c6c220abe2d1591fe919801625e6237aa7438d2874ebfaf` |
| macOS reference capture | SHA-256 `042200df6dee015c4065a1556049bf8a52798a8f78e9a617e47111bc38bc8d8b` |
| macOS Skia closure | revision `294d31e0b1aa295d585836ab41bd2fba170e0c5d` |
| macOS transitive lock | SHA-256 `81c192af6d5db893dbb5221b27254deccab91143936adc1aedf1ab667beaf56c` |
| Machine receipt | SHA-256 `a514903866d6e887d460ecf88bf73731d7cdd8d79c936787e9113684c5995f86` |

The machine-readable receipt is
[`xpf-pre-windows-macos-metal-qualification.json`](artifacts/xpf-pre-windows-macos-metal-qualification.json).
The RGB8 reference is
[`xplat-visual-v1-macos-metal-640x360.ppm`](artifacts/xplat-visual-v1-macos-metal-640x360.ppm).

## Final local gates

The accepted commit was built and exercised on macOS 26.5.1, Apple M1/Metal 4,
AppleClang 21.0.0, CMake 4.3.3 and Ninja 1.13.2:

```text
macos-core             30/30 passed
macos-core-sanitized   30/30 passed under ASan/UBSan
macos-visual           51/51 passed
skia fixture capture   1/1 passed; Preview == Offline bytes
ios-core-canary        BUILD SUCCEEDED (iPhoneOS arm64)
ios-graphics-canary    BUILD SUCCEEDED (iPhoneOS arm64 + common Skia)
docs-doctor            106 documents, 0 problems
architecture-check     112 source files, 0 problems, 0 boundary debt
```

The capture was regenerated from the accepted commit and retained the expected
SHA-256. The machine receipt binds canonical RFX, command, Registry, font,
color and RenderPlan identities to this host and explicitly keeps full
performance and production Offline Export unqualified.

## Windows handoff protocol

The Windows host must check out the exact source commit or this evidence-only
descendant. Its first `CompileOnly` run may generate
`deps/locks/skia-transitive-windows-x64.lock.json`; that run is non-qualifying.
The lock must be reviewed and committed. A second clean run then builds Core,
Graphics and Visual in order, exercises a named non-WARP GPU, emits the D3D12
capture and compares it to the committed Metal reference. Any shared-source
correction invalidates this receipt and requires the affected macOS gates to be
rerun.

## Claim boundary

- macOS common visual source: compiled and physically run;
- macOS semantic/capture reference: passed the accepted bounded contract;
- iOS: compile canary only, not a product runtime;
- Windows MSVC, D3D12 physical pixels, performance and device recovery:
  `not-run`;
- Android official-NDK compile: `not-run`;
- Windows Media Foundation video decode and production Offline Export: outside
  this closure and still open in their owning gates.

The full Fix Cross-Platform Architecture plan therefore remains active until
the external Windows and Android receipts and the required profile
qualification evidence are reconciled. Only its Pre-Windows Source Closure is
closed by this record.
