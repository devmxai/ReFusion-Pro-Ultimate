# Module boundaries

Allowed dependency direction:

```text
apps -> studio adapters -> application -> core contracts
                                      -> runtime -> platform ports
platform implementations --------------------^ 
Skia/SQLite adapters -------------------------^
```

Rules:

- `src/core`: standard C++ only; no UI, platform, Skia, media SDK, or filesystem
  watcher authority.
- `src/application`: commands, revisions, authoring, transport, AssetDB
  coordination; depends on engine-owned ports.
- `src/runtime`: semantic compiler and media/render/audio/export orchestration;
  never depends on Studio.
- `src/adapters`: Skia, SQLite, and other replaceable technology bindings.
- `src/platform`: native implementations only; no project semantics.
- `src/studio` and `apps/studio`: Qt/QML command and view adapters only.
- `services/live_authoring`: reconciliation/build/validation using application
  services; file watcher events are hints.

Platform conditionals are restricted to `src/platform`, `packaging`, toolchain,
and bootstrap code. Common semantic code must not use `_WIN32`, `__APPLE__`,
Android, or Qt platform branches.

