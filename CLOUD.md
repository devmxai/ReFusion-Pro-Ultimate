# Cloud and remote runner policy

Cloud agents may edit portable code, contracts, docs, deterministic tests, and
build artifacts for toolchains available in their environment. They must not
claim evidence they cannot physically observe.

## Runner classes

- **Portable CI:** core contracts, unit/property/replay tests, docs and boundary
  checks, schema/code generation.
- **Native build CI:** macOS, Windows, iOS-simulator, and Android toolchains.
- **GPU/device lab:** native surface interop, decode/encode, presentation,
  audio-clock, device-loss, performance, thermal, and golden evidence.
- **Protected release runner:** signing, notarization/store upload, provenance,
  SBOM, and promotion. Secrets never enter PR runners or repository files.

If required hardware, signing credentials, codec support, or store access is
missing, record `CODE_COMPLETE_AWAITING_NATIVE_GATE`; do not mark the gate
passed and do not substitute a software path.

