---
id: PROD-001
kind: product-contract
status: accepted
owner_role: product-architecture
canonical_for: product-scope
last_verified: 2026-08-07
---

# ReFusion product contract

## Product promise

ReFusion v1 is an agent-native desktop studio for creating professional short
video and motion compositions through the same project whether the user edits
with the native UI or an external agent.

## First paid creator loop

```text
Install -> Create/Open -> Import Video/Image/Audio
-> Add Text/Shape -> Animate/Mask/FX -> Preview
-> Save/Close/Reopen -> Export shareable MP4 -> Update/Recover
```

## Desktop v1 candidate scope

- macOS Apple Silicon and Windows x64 from the first installable trunk;
- SDR Rec.709, 1080p reference profile until higher profiles qualify;
- narrow published media matrix, initially targeting hardware-supported
  H.264/AAC MP4 plus PNG/JPEG and WAV, subject to licensing and hardware gates;
- Video, Audio, Image, Text, Shape, Group, and bounded Adjustment layers;
- trim/split/move, Transform2D, crop, opacity, compatible blend modes;
- one typed property/keyframe system with Step/Linear/Cubic interpolation;
- typed masks and a small complete FX set such as blur, color controls, and
  drop shadow;
- real waveform, gain, pan, fades, save/reopen, undo/redo, recovery, relink;
- one semantic evaluator for preview/export and one command path for UI/agent.

## Explicitly not v1

HDR/RAW/multicam, 3D, particles, full tracking/rotoscoping, public native
plugin marketplace, cloud collaboration, arbitrary expressions, a general node
editor, a broad codec matrix, and full mobile Studio.

## Reference projects

1. Short video with animated Arabic/Latin text and linked audio.
2. Image/Shape animation with mask and complete FX chain.
3. Short advertisement combining Video/Image/Text/Audio and exporting MP4.

This contract was accepted as the v1 planning baseline when execution of MP-001
was explicitly started on 2026-08-07. Media/device rows remain qualification
targets rather than shipping claims until their gates pass.
