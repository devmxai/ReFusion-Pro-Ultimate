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
2. [`docs/status/CURRENT.md`](docs/status/CURRENT.md)
3. [`docs/plans/MASTER_PLAN.md`](docs/plans/MASTER_PLAN.md)
4. the current stage plan linked from `CURRENT.md`

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

