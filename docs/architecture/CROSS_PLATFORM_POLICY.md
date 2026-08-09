---
id: ARCH-XPLAT-001
kind: architecture-policy
status: accepted
owner_role: principal-architecture
canonical_for: cross-platform-build-and-evidence
last_verified: 2026-08-09
accepted_by: product-owner-user-instruction-2026-08-07
---

# Cross-platform build and evidence policy

## Product lanes

- Desktop v1: macOS Apple Silicon and Windows x64 are equal source and product
  lanes. Neither platform may redefine project, layer, timing, command, render,
  media, audio, or export semantics.
- Mobile: iOS and Android receive portable-contract compile canaries during G1;
  full product/runtime qualification remains G9.
- The current physical runtime lab is macOS only. Windows, iOS, and Android
  runtime state is `not-run`, never `passed`, `unsupported`, or silently waived.

## Canonical two-host Git workflow

This section is the single operating policy for synchronized macOS and Windows
development. A pending remote branch is a proposed change or evidence handoff;
only the reviewed commit reachable from `origin/main` is shared product truth.

### Branch authority

| Branch class | Purpose | May contain | Integration destination |
|---|---|---|---|
| `main` | Protected, always-buildable integration trunk | Reviewed shared product source, thin platform adapters, accepted contracts and reconciled evidence | N/A |
| `feature/shared-*` or `feature/<capability>` | Portable UI/Core/Application/Runtime/RenderPlan/SkiaCommon product work | One bounded feature and its tests/docs | `main` after the affected lanes pass |
| `fix/shared-*` | Correction to portable behavior observed on either host | Common correction, regression test and evidence invalidation note | `main` after macOS and Windows affected checks pass |
| `fix/macos-*`, `fix/windows-*` | Native mechanics only | Metal/AppKit or D3D12/DXGI/MSVC mechanics; no project/FX/text/timeline meaning | `main` after common checks and the named physical lane pass |
| `evidence/<platform>-*` | Immutable diagnostic/qualification handoff | Receipts, report updates and generated-artifact hashes; no unrelated feature development | Reconciled evidence/docs, plus separately reviewed fixes, into `main` |

No macOS or Windows Agent may perform concurrent feature development directly
on `main`, continue product development on an old evidence branch, force-push a
shared branch, or rewrite a commit already used for physical evidence. One
integration owner promotes one reviewed commit at a time.

### Change classification

- Qt/QML layouts, Inspector, Timeline projection and shared Studio command
  surfaces are shared product work. They go to a feature branch and ultimately
  to `main`; they never go to a Windows-evidence branch merely because Windows
  last qualified the base commit.
- Project, command, time, animation, Registry, RenderPlan, compositor, color,
  font, FX and plugin meaning is shared work even when a defect is first seen on
  only one platform.
- Metal, D3D12, Vulkan, window-host, swapchain, fence, device-loss and native
  media mechanics use a platform-fix branch. A native branch may not compensate
  for a shared semantic defect.
- Logs, screenshots, captures and machine receipts are evidence. Build trees,
  dependency materializations, Qt/Skia binaries and `out/` remain untracked.

### Required sequence

Every human or Agent change follows this order:

1. Finish or discard no work implicitly; confirm the existing worktree is
   understood and preserved.
2. Fetch, start from the latest accepted trunk and record the base commit:

   ```bash
   git fetch --prune origin
   git switch main
   git pull --ff-only origin main
   git status --short
   git rev-parse HEAD
   ```

3. Create exactly one correctly classified branch. Never recycle an evidence
   branch for later product work.
4. Implement and run the smallest required checks plus every affected physical
   platform lane. Record `not-run` honestly where a host is unavailable.
5. Commit source, tests and required documentation together; push the feature,
   fix or evidence branch, not `main`.
6. Reconcile the branch against current `origin/main`. If it is not a clean
   descendant or a fast-forward candidate, stop and review the divergence on an
   integration branch or pull request. Never resolve semantic conflicts by
   choosing one whole platform copy.
7. The integration owner promotes the reviewed commit to `main`, preferably by
   fast-forward. A non-fast-forward merge requires an explicit review receipt.
8. After the `main` push, every other host fetches and checks out that exact
   commit before starting its next change or evidence run.

When a clean fast-forward is possible, the integration operation is:

```bash
git switch main
git pull --ff-only origin main
git merge --ff-only <reviewed-branch>
git push origin main
```

Failure of `--ff-only`, a dirty integration worktree, missing evidence or a red
check is a stop condition. Force push, `reset --hard`, copying files between
platform branches and weakening a check are not recovery procedures.

### Handoff and evidence invalidation

Every pushed branch handoff records: base commit, head commit, branch purpose,
host/toolchain/GPU, exact changed files, checks with pass/fail/not-run, first
meaningful diagnostic, remaining limitations and the recommended next action.

A change to shared project, RenderPlan, font, color, compositor, UI projection
or qualification-fixture behavior invalidates every affected older platform
receipt. macOS and Windows rerun against the same new `main` commit; a receipt
from an ancestor cannot qualify its descendant. Platform-specific evidence may
advance independently only when the shared source and fixture digests are
unchanged.

For example, a portrait Timeline/UI change is developed on
`feature/portrait-workspace-layout`, qualified on macOS, pushed for review and
then promoted to `main`. The Windows host subsequently pulls that exact `main`
commit and records its Windows result. The UI commit is never appended to the
historical Windows evidence branch.

## Merge contract

Every shared feature must:

1. place its versioned descriptor, state, migration, validation and evaluator
   semantics in portable C++20;
2. compile exact-time state into one immutable backend-neutral RenderPlan;
3. execute visual meaning through the common Skia compositor;
4. keep OS/GPU/media/window implementation in an explicit native adapter that
   owns mechanics only and does not switch on feature/effect kinds;
5. prepare semantic/runtime capability before atomic accepted-bundle publish;
6. keep Studio commands/snapshots descriptor-driven and platform-neutral;
7. define corresponding macOS and Windows build lanes before integration;
8. use the same fixture, IDs, time domains, semantic/plan digest, diagnostics,
   migrations and failure model;
9. serialize project/Agent truth with locale-free canonical numbers, portable
   case-sensitive ASCII IDs and validated byte-preserved UTF-8;
10. record separately whether each platform is defined, compiled, run and
   qualified.

The repository architecture check rejects platform conditionals in common
semantic code, protects declared macOS/Windows lanes and applies the
`PLAN-XPLAT-FIX-001` visual-boundary ratchet. Existing exact debt is frozen in a
digest-pinned, shrink-only manifest; new signatures, wildcard allowances and
count growth fail. This source check is not physical pixel/runtime qualification.

An FX/plugin is cross-platform only when one contribution identity and semantic
contract passes the same state/migration/plan/diagnostic/conformance corpus on
every required lane. Native C++ extensions are post-v1, out of process and
separately built/signed per target triple; mobile admits no downloaded native
executable plugin. Missing support fails closed without semantic substitution.

## Current execution rule

G1-WP01 may produce the first real visual experience and G1-WP03 may execute the
Apple hardware-media proof on `MAC-LAB-001` because that is the available
physical device. This is a runtime-evidence choice only. Presenter/media
contracts, fixtures, counters and failure semantics remain portable; Metal and
VideoToolbox code stay in Apple adapters, while matching Windows lanes remain
tracked by G1-WP02/G1-WP04 until a Windows device becomes available.

No stage or feature may claim cross-platform qualification from macOS evidence
alone. G1 cannot pass until the required Windows evidence exists.

The `windows-visual` source lane now binds the shared visual-program executor to
Skia D3D12 and an engine-owned DXGI presenter. This is a declared future build
and physical-test route, not passed Windows evidence. Native Metal/D3D/Vulkan
bindings may not include project/compiler headers or own FX-specific lowering.

The authoritative remediation, relocation and plugin admission contract is
[`PLAN-XPLAT-FIX-001`](../plans/FIX_CROSS_PLATFORM_ARCHITECTURE.md).

The admitted G1 mobile compile surface is fixed by `G1-WP07`: iPhoneOS arm64
uses UIKit/Metal without AppKit, and Android arm64-v8a/API 28 uses
ANativeWindow/Vulkan through the pinned official NDK. Both compile the same
Core/RFX/RenderPlan/SkiaCommon semantics and deliberately reject product
presentation. Compile evidence advances only the `compiled` matrix state;
physical run, semantic match, visual tolerance, performance and qualification
remain false until their own device receipts exist.
