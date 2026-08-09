---
name: refusion-project-authoring
description: Author and repair this project's typed Project.rfx source. Use when inspecting the project, adding or editing supported Shape/Text layers and pass-through LayerGroups, changing exact frame ranges, anchors or transforms, authoring supported keyframes, validating a candidate, incrementing a revision, or repairing a Studio rejection.
---

# ReFusion Project Authoring

Treat `Project.rfx` as typed declarative source that compiles into the native
Core snapshot. It is not C++, JSON or free-form text.

## Workflow

1. Read `.refusion/agent-context.json` and use its absolute `cli_executable`;
   never guess an installation or repository path.
2. Read `references/supported-capabilities.md`,
   `references/agent-commands.md` and `references/semantic-authoring.md`.
3. Run `capabilities`, then `outline <Project.rfx>`. Use `inspect` for the
   target stable ID and `measure <Project.rfx> <time-ns> --json` for any
   spatial request. Never infer Canvas geometry from names or Timeline pixels.
4. Read `references/coordinates-and-time.md` for spatial/timeline edits,
   `references/language-v6.md` for portable linked media and
   `references/language-v5.md` for canonical visual declarations,
   `references/property-registry.md` for the generated Inspector/Agent property
   vocabulary, plus `references/visual-contributions.md` for registered Mask/FX
   parameters and bounds. RFX1–RFX4 references are migration documentation only.
5. For Group, static Glow or measured Align, use the typed `commit` operation
   from `agent-commands.md`; it performs revision CAS, validation and atomic
   publication through the same Core command type as UI authoring.
6. For another supported declaration, create a candidate outside
   `Project.rfx`, preserve stable IDs, increment revision exactly once, run
   `validate <candidate> --json`, then inspect `diff <active> <candidate>`.
7. Publish only a valid one-revision candidate atomically. A running Studio
   revalidates it through Application authority before changing accepted state.
8. On rejection, read the diagnostics path from agent-context and follow
   `references/diagnostics-and-repair.md`.

Use only descriptors reported by `capabilities`. Qualified assets/fonts/plugins
are referenced by stable ID, version and digest; never persist an OS path,
system-font assumption, native binary path or platform-specific replacement.
Preserve unresolved contribution state and fail closed instead of approximating
an unavailable FX.

`outline`, `inspect` and `measure --json` are the authoritative digital-eye
projection. `describe` is human-readable convenience only.
