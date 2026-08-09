# ReFusion

ReFusion is a native, agent-first, cross-platform video and motion studio. The
repository is intentionally organized around one semantic project engine:
Qt/QML and external agents are clients that submit typed commands and consume
immutable accepted-revision snapshots.

## Current state

The repository is in **G0 — Product and Architecture Contract**. No production
renderer, media pipeline, or supported product capability is claimed yet.

Read in this order:

1. [`AGENTS.md`](AGENTS.md)
2. On the physical Windows test host, read and follow
   [`README_FOR_WINDOWS.md`](README_FOR_WINDOWS.md).
3. [`docs/status/CURRENT.md`](docs/status/CURRENT.md)
4. every active guardrail linked from `CURRENT.md`, currently
   [`Fix Cross-Platform Architecture`](docs/plans/FIX_CROSS_PLATFORM_ARCHITECTURE.md)
5. [`Cross-platform Git workflow`](docs/architecture/CROSS_PLATFORM_POLICY.md#canonical-two-host-git-workflow)
6. [`docs/plans/MASTER_PLAN.md`](docs/plans/MASTER_PLAN.md)
7. the current stage plan linked from `CURRENT.md`

Visual development is single-source: portable descriptors/evaluation compile
to one RenderPlan executed by one common Skia compositor. Metal, D3D12 and
Vulkan targets differ only in native device/target/import/sync/present mechanics.
The D3D12/DXGI source route has now compiled and physically run on hardware
Windows for the bounded Canvas/Studio profile. The final same-commit
Metal/D3D12 pixel comparison, performance qualification, the post-checkpoint UI
rerun and Windows Video import/decode remain open; see `CURRENT.md`.

The large foundation discussion is preserved in
`docs/research/foundation-screening-draft.md`; it is research evidence, not an
implementation authority.

## Foundation commands

```bash
python3 tools/rfdev.py status
python3 tools/rfdev.py context
python3 tools/rfdev.py docs-doctor
python3 tools/rfdev.py architecture-check
python3 tools/bootstrap.py doctor
cmake --workflow --preset macos-core
```

Build output and fetched dependency sources belong under `out/` and never in
Git. Release builds must use the pinned dependency manifest.

## Verified development machine cache

Ordinary development clones may reuse a content-addressed per-user dependency
cache after one verified publication. This is an acceleration path only; it is
not release or physical-qualification evidence.

```bash
python3 tools/bootstrap.py machine-cache publish-skia \
  --profile macos-arm64-metal --from-checkout /path/to/verified/checkout
python3 tools/bootstrap.py machine-cache publish-qt \
  --source /path/to/Qt/6.11.1/kit
python3 tools/bootstrap.py machine-cache status
```

When local `out/` dependencies are absent, CMake discovers a verified matching
entry automatically. A changed dependency pin, profile, patch, lock or host
architecture is a cache miss. Set `REFUSION_MACHINE_CACHE_ROOT` to use a
non-default per-user cache location. Windows uses the
`windows-x64-d3d12` profile and defaults to `%USERPROFILE%\.rfx\dc1`.

## Clean Skia materialization

Fresh qualification never copies Skia from another checkout or the development
cache. Its admitted source flow is:

```bash
python3 tools/bootstrap.py sync depot_tools --fresh
python3 tools/bootstrap.py sync skia --fresh
python3 tools/bootstrap.py hydrate-skia
python3 tools/bootstrap.py build-skia --profile macos-arm64-metal
cmake --workflow --preset macos-graphics
```

The first two commands clone exact revisions from their official origins into
the ignored `out/deps-src`. Hydration follows Skia's pinned `DEPS` graph and
records the resolved dependency inventory. Normal CMake builds remain offline
and fail closed if origin, source revision, build record, or artifact differs.
