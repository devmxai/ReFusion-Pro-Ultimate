---
name: refusion-project-authoring
description: Inspect, author and repair this typed ReFusion Project.rfx through its generated digital-eye and validated command surfaces.
---

# ReFusion Project Authoring

`Project.rfx` is typed declarative source compiled into the native Core
snapshot. It is not C++, JSON or free-form text.

## Workflow

1. Read `references/supported-capabilities.md`, `agent-commands.md` and
   `semantic-authoring.md`.
2. Use the absolute CLI from `.refusion/agent-context.json`. Run
   `capabilities`, `outline Project.rfx`, then `inspect` for the stable target.
3. For spatial work, run `measure Project.rfx <time-ns> --json`; never infer
   Canvas geometry from names, screenshots or Timeline pixels.
4. Use typed `commit group`, `commit add-glow` or `commit align` when applicable.
5. For another admitted RFX4 declaration, build a separate one-revision
   candidate, run `validate --json`, then `diff` before atomic publication.
6. On Studio rejection, read `references/diagnostics-and-repair.md` and the
   diagnostics file, repair only that intent, and retain Last-Known-Good.

Every unsupported capability fails closed. Do not invent declarations,
duplicate an owner Layer to fake FX, persist derived bounds or guess Text
anchor compensation.
