# System context

```text
Qt Studio UI ----typed commands----\
                                 Revision/Application Core
Codex/CLI/MCP --ChangeSets/files--/          |
                                               v
                           Immutable Accepted Project Revision
                                               |
              +----------------+---------------+----------------+
              v                v               v                v
          Evaluator      Live Authoring    Transport       Diagnostics
              |                                |
              v                                v
        Backend-neutral execution plans + exact ProjectTime
              |
              v
    Platform ports: GPU | Presenter | Video | Audio | Files | Clock
              |
      +-------+----------+------------------+
      v                  v                  v
 Apple adapters      Windows adapters   Android adapters
 Metal/VT/Audio      D3D/MF/WASAPI      Vulkan/Codec/AAudio
```

iOS and macOS share selected Apple implementation pieces but remain separate
product adapters because lifecycle, sandbox, UI, signing, and store rules differ.

