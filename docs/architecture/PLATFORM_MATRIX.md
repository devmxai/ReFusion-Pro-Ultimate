# Platform qualification matrix

This matrix records intended lanes, not current support claims.

| Lane | Product priority | GPU/presenter candidate | Hardware media candidate | Audio candidate | Current state |
|---|---:|---|---|---|---|
| macOS arm64 | Desktop v1 | Metal/CAMetalLayer | VideoToolbox/CoreVideo | CoreAudio | G0 portable Core locally green; G1 runtime proposed |
| Windows x64 | Desktop v1 | D3D12/DXGI | Media Foundation hardware MFT | WASAPI | G0 portable Core CI defined; execution pending; G1 runtime proposed |
| iOS arm64 | post-desktop product; early canary | Metal/CAMetalLayer | VideoToolbox | AVAudioSession/CoreAudio | contract canary |
| Android arm64 | post-desktop product; early canary | Vulkan/ANativeWindow | MediaCodec Surface/AHardwareBuffer | AAudio/Oboe candidate | contract canary |

Every shipping row requires device tiers, OS minimum, GPU/driver matrix, codec
profiles, performance budgets, unsupported behavior, and real-device evidence.
