# ReFusion project instructions

`Project.rfx` is the only authored project truth in this folder. Before editing
it, use the project-local `$refusion-project-authoring` Skill at
`.agents/skills/refusion-project-authoring/SKILL.md`.

## Mandatory protocol

1. Read `.refusion/agent-context.json` for the exact CLI and diagnostics paths.
2. Run CLI `capabilities` and `outline Project.rfx`; use `inspect` and
   `measure --json` before changing a target or its geometry.
3. Preserve the project ID and every unchanged semantic object's stable ID.
4. Use the typed CLI `commit` for Group, static Glow and measured Align intents.
5. For other supported declarations, build a separate one-revision candidate,
   run `validate --json` and review `diff` before atomic publication.
6. Studio revalidates every file candidate through Application authority; a
   CLI success is publication, not permission for UI/file state to bypass it.
7. If Studio rejects it, read `.refusion/Diagnostics/session.jsonl`, repair only
   the rejected intent and keep Last-Known-Good as the base.

`.refusion` is generated host-local state, never portable project truth. When a
workspace is copied or moved, Studio replaces copied context, lock, cache,
journal and diagnostics for the new canonical path; do not preserve their old
absolute paths in `Project.rfx`.

Use only capability/descriptor IDs returned by this engine build. Never make an
absolute OS path, local system font, `.dll`/`.dylib`, native handle or
platform-specific plugin declaration project truth. A missing/incompatible
contribution remains unresolved with its state preserved; do not replace it
with a visually similar effect.

Qualified text must use an AssetId, family and digest copied from
`Assets/Fonts/catalog.lock`. Never infer qualified typography from the host
operating system; missing or changed packaged bytes are a blocking diagnostic.

Never create `project.cpp`, a second JSON project, UI state or renderer state.
Never invent unsupported Video, Audio, Image, SVG, Glass/backdrop, Motion Blur,
procedural texture, isolated-group opacity or nested-composition declarations.
