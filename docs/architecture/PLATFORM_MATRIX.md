# Platform qualification matrix

This matrix records intended lanes, not current support claims.

| Lane | Product priority | GPU/presenter candidate | Hardware media candidate | Audio candidate | Current state |
|---|---:|---|---|---|---|
| macOS arm64 | Desktop v1 | Metal/CAMetalLayer | VideoToolbox/CoreVideo | CoreAudio | Core, presenter and same-device Skia green; H.264 capability plus NV12 Metal-surface interop green, compressed-sample decode pending |
| Windows x64 | Desktop v1 | D3D12/DXGI | Media Foundation hardware MFT | WASAPI | Core/D3D12/Skia and fail-closed media lane defined; Windows execution pending |
| iOS arm64 | post-desktop product; early canary | Metal/CAMetalLayer | VideoToolbox | AVAudioSession/CoreAudio | contract canary |
| Android arm64 | post-desktop product; early canary | Vulkan/ANativeWindow | MediaCodec Surface/AHardwareBuffer | AAudio/Oboe candidate | contract canary |

Every shipping row requires device tiers, OS minimum, GPU/driver matrix, codec
profiles, performance budgets, unsupported behavior, and real-device evidence.
