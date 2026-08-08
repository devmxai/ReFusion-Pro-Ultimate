---
id: EXP-001
kind: work-package
status: active
gate: pre-G2-experiment
owner_role: core-project
---

# Outcome

Deliver a runnable macOS experiment in which a real project-local `Project.rfx`
file is compiled to portable Core state and rendered by the existing native GPU
application. Prove that an agent can discover the project contract, validate a
candidate and receive precise failure diagnostics without introducing another
project/render authority.

# Dependencies

- RFC-0001 experiment authorization.
- Existing `ProjectDocument`, `ProjectAuthority`, `ProjectClock` and Skia walking
  product.
- Existing macOS-only physical runtime direction; Windows remains build-only or
  not-run evidence as applicable.

# Read first

- `AGENTS.md`
- `docs/status/CURRENT.md`
- `docs/architecture/INVARIANTS.md`
- `docs/decisions/rfcs/RFC-0001-project-rfx-authoring-experiment.md`
- `contracts/project/refusion-project-rfx-exp1.ebnf`

# Allowed paths

- `contracts/project/`
- `src/core/`
- `src/application/` only if candidate acceptance requires it
- `apps/cli/`
- `apps/studio/` only for the project file boundary and bounded live update
- `examples/projects/rfx-authoring-experiment-01/`
- `.agents/skills/` below that example project
- corresponding tests, evidence, status and decision register files

# Forbidden paths

- GPU/media/platform ownership changes
- new dependencies or network downloads
- Video/Audio import claims
- native project plugins or runtime C++ compilation
- QML/project/UI authority
- JSON as the new canonical experiment source

# Deliverables

- strict versioned RFX grammar and portable compiler/serializer;
- source-location diagnostics and semantic validation;
- CLI validate/describe commands;
- real project folder with `Project.rfx`, lock, instructions and Skill;
- Studio default open from `Project.rfx`;
- positive, negative, round-trip and Last-Known-Good tests;
- evidence and exact handoff.

# Verification

```bash
python3 tools/rfdev.py docs-doctor
python3 tools/rfdev.py architecture-check
cmake --workflow --preset macos-core
cmake --workflow --preset macos-studio
```

# Evidence path

`docs/evidence/experiments/EXP-001.md`

# Failure and rollback

The existing JSON seed and its last green commit remain the rollback point. An
invalid RFX candidate must not replace the active Core snapshot. If the grammar
requires UI ownership, frame-time interpretation or a second evaluator/render
path, stop the experiment.

# Exact handoff condition

Stop with the exact supported grammar, passing command/test receipts, known
unsupported capabilities and the next decision required. Do not promote the
experiment to an accepted ADR automatically.
