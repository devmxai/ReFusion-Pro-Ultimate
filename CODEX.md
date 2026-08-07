# Codex operating guide

`AGENTS.md` is the automatically discovered instruction surface. This file is a
linked operator guide and does not override the Master Plan or current status.

## Normal workflow

```text
status -> context -> active work package -> baseline checks
       -> bounded change -> verification -> evidence -> checkpoint/handoff
```

Useful commands:

```bash
python3 tools/rfdev.py status
python3 tools/rfdev.py context
python3 tools/rfdev.py docs-doctor
python3 tools/rfdev.py architecture-check
python3 tools/bootstrap.py doctor
```

Use the repo-local `$refusion-engineering` skill when executing or handing off a
ReFusion work package. It performs progressive disclosure: load the current
status and stage first, then only linked contracts/decisions.

## Diagnostics discipline

Prefer structured diagnostics over raw logs. A useful diagnostic carries a
stable code, run ID, revision/change-set IDs, entity/property references,
source location, severity, fingerprint, and suggested recovery. Do not repair
by guessing names, IDs, frames, coordinates, or capabilities.

## Agent/project integration target

The product will expose one service core through both CLI and local MCP. MCP is
the preferred transactional authoring surface; canonical project files remain
portable and externally editable. Both paths must produce the same semantic
revision digest and diagnostics.

