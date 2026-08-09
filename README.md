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
5. [`docs/plans/MASTER_PLAN.md`](docs/plans/MASTER_PLAN.md)
6. the current stage plan linked from `CURRENT.md`

Visual development is single-source: portable descriptors/evaluation compile
to one RenderPlan executed by one common Skia compositor. Metal, D3D12 and
Vulkan targets differ only in native device/target/import/sync/present mechanics.
The D3D12/DXGI source route and `windows-visual` lane are defined, but Windows
build/device/pixel evidence remains `not-run` until a real Windows lab exists.

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

## Clean Skia materialization

Skia is never copied from another checkout. The only admitted source flow is:

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
