---
id: DEP-MEDIA-001
kind: dependency-intake
status: candidate-native-only
owner_role: media-release
last_verified: 2026-08-07
---

# Media and codec intake

Initial product code does not bundle a general software codec stack. It uses
native hardware APIs behind engine ports:

- Apple: VideoToolbox/CoreVideo/Metal and native audio APIs.
- Windows: Media Foundation hardware MFT/D3D and WASAPI.
- Android canary: MediaCodec Surface/AHardwareBuffer/Vulkan and native audio.

H.264/AAC capability in an OS does not by itself settle patent, distribution,
content, regional, or royalty obligations. Before beta, obtain legal review for
the product's encoding/decoding/distribution business model and document any
required pool/vendor terms.

FFmpeg is not a foundation dependency. If later admitted for container/probe
work, it receives its own exact source/config/license/SBOM intake and may not
enable software video fallback in the production path.

