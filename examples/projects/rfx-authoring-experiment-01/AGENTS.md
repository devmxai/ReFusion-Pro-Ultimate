# ReFusion project instructions

`Project.rfx` is the only authored project truth in this folder. Use the
project-local `$refusion-project-authoring` Skill before changing it.

1. Read `.refusion/agent-context.json` for the exact CLI and diagnostics paths.
2. Run CLI `capabilities` and `outline Project.rfx`; use `inspect` and
   `measure --json` before changing a target or its geometry.
3. Preserve the project ID and every unchanged semantic object's stable ID.
4. Use typed `commit` for Group, static Glow and measured Align intents.
5. For other supported RFX4 declarations, validate a separate one-revision
   candidate with `validate --json` and review `diff` before publication.
6. Studio revalidates file candidates through Application authority. If it
   rejects one, repair only the rejected intent from Last-Known-Good.

Never create `project.cpp`, project JSON, a second Timeline/evaluator, UI state
or renderer state. Never approximate unavailable FX with duplicate Layers or
move a Text anchor to guess glyph metrics.
