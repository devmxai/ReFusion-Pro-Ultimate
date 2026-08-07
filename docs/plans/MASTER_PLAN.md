---
id: MP-001
kind: master-plan
status: active
version: 1
owner_role: program-architecture
canonical_for: delivery-order
last_verified: 2026-08-07
research_basis: docs/research/foundation-screening-draft.md
activated_by: user-instruction-2026-08-07
---

# ReFusion Master Plan

## Mission

Deliver a native cross-platform ReFusion product through installable vertical
slices, while preserving one project/command/revision truth and proving GPU,
media, timing, persistence, agent parity, and release claims before expansion.

The execution pattern is:

```text
Walking Product Skeleton
-> Kill-Risk Proofs
-> Semantic/Revision Spine
-> Live Visual Authoring
-> Hardware Media/Audio
-> Complete Creator Loop
-> Product Hardening
-> Paid Beta
-> Stable Desktop v1
-> Mobile and SDK Expansion
```

There is no stage called “build the whole engine.” From G1 onward, every gate
must preserve an installable trunk and add an end-to-end user outcome.

## Meaning of professional cross-platform

ReFusion guarantees semantic parity only for an explicit, qualified capability
and device/media profile. It does not promise identical native APIs, bit-exact
floating-point pixels on every GPU, hardware support for every codec, or a
defect-free program. Failures are contained, diagnosed, and fail closed.

macOS arm64 and Windows x64 are Desktop v1 product lanes. iOS and Android enter
portable-core and adapter-contract CI early, but full mobile product work does
not delay Desktop v1.

## Program gates

| Gate | Product outcome | Installable evidence | Status |
|---|---|---|---|
| G0 | Product, architecture, dependency, legal, and qualification contract | Core/CLI build baseline | active |
| G1 | Signed walking skeleton and GPU/media kill-risk proof | macOS + Windows clean-machine artifacts | planned |
| G2 | Command/Revision/Live-Authoring vertical slice | Qt Studio with Text/Shape/Image round-trip | planned |
| G3 | Unified visual authoring | Complete Image/Text/Shape/Group/Adjustment slice | planned |
| G4 | Hardware media, audio, and exact transport | Video+Audio import/play/seek/edit/export slice | planned |
| G5 | Complete Desktop Creator Loop | Internal alpha installers | planned |
| G6 | Reliability and release hardening | Signed RC with recovery/update evidence | planned |
| G7 | Paid Founder Beta | Externally installable paid beta | planned |
| G8 | Stable Desktop v1 | Promoted stable artifact | planned |
| G9 | iOS and Android productization | TestFlight/Play internal products | planned |
| G10 | Public extension and SDK expansion | Versioned conformance-qualified SDK | planned |

Future gate detail is intentionally outcome-level. Only the active gate and the
next risk gate receive detailed work packages, preventing stale speculative plans.

## G0 — Product and architecture contract

### Outcome

Turn research into a reviewable operating system: product scope, invariants,
platform/media matrix, repo boundaries, dependency/legal intake, exact toolchain
baseline, architecture enforcement, and G1 proof design.

### User-visible demonstration

A reproducible C++ core/CLI build proves typed command acceptance, stale edit
rejection, and preservation of Last-Known-Good state. It is not a renderer claim.

### Exit criteria

- Product Contract, Invariants, Master Plan, G1 plan, status, risks, and ADR
  register are reviewed.
- C++20 portable core builds and tests on macOS and Windows CI.
- Dependency manifest has official immutable origins and licensing status.
- Qt commercial/LGPL path, Skia role, strict GPU policy, and initial media matrix
  have explicit ADR decisions or blocking owners.
- Architecture/docs checks run locally and in CI.
- G1 spikes have metrics, kill criteria, platform devices, and evidence format.

Detailed plan: `stages/G0-foundation/PLAN.md`.

## G1 — Signed walking skeleton and kill-risk proofs

### Outcome

Native Qt Studio installers on macOS and Windows host an engine-owned native GPU
viewport and render a GPU-backed Skia Text/Shape fixture. Separate spikes prove
hardware decoded surface admission without CPU pixel transfer.

### Required proofs

- one physical GPU device/queue/fence authority;
- Qt window/surface to engine presenter without Qt Canvas/media ownership;
- Skia native GPU producer using borrowed engine resources;
- Apple Metal/VideoToolbox and Windows D3D/Media Foundation surface routes;
- fail-closed unsupported behavior and runtime zero-CPU-pixel counters;
- preview/export semantic path seed;
- clean-machine install/uninstall and signing/notarization skeleton;
- iOS Metal and Android Vulkan portable contract/build canaries.

### Kill criteria

If a candidate requires a permanent CPU pixel bridge, competing GPU owner, UI
presentation authority, hidden software decoder, or unredistributable license,
stop feature expansion and replace or narrow that candidate.

Detailed plan: `stages/G1-walking-skeleton/PLAN.md`.

## G2 — Command, revision, and Live Authoring spine

### Outcome

The Unity-like authoring mechanism becomes real: UI, MCP/CLI, and canonical file
edits produce transactional ChangeSets; valid candidates atomically publish one
revision to Timeline/Canvas/Inspector, while invalid candidates retain LKG and
emit structured diagnostics.

### Scope

Stable typed IDs, exact rational time, pixel-true coordinates, schemas and
registry, CAS revision authority, EvaluationStamp/PresentationGate, persistence
and journal, Source/Dependency/Artifact DBs, watcher-as-hint reconciliation,
minimal MCP/CLI, Text/Shape/Image, Console, and deterministic round-trip tests.

### Exit criteria

- equivalent UI/agent commands produce the same semantic digest;
- no mixed revision under stress;
- rename/reorder/save/reopen preserves IDs;
- partial, stale, and invalid edits never replace active state;
- exact time and canvas probes pass; diagnostics match across Console/CLI/MCP.

## G3 — Unified visual authoring

### Outcome

Image/Text/Shape/Group/bounded Adjustment share one Layer/Descriptor/Typed-Port,
Property/Animation, Mask, and FX system. Inspector is schema-driven; preview and
offline render use one evaluator.

### Exit criteria

- a new visual descriptor requires no Inspector or command-family switch;
- compatible transforms/animation/masks/FX operate across visual types;
- invalid port combinations fail with typed diagnostics;
- Arabic/RTL/font fixtures and macOS/Windows semantic goldens pass;
- no backend/UI types enter project state.

## G4 — Hardware media, audio, and exact transport

### Outcome

Importing video creates linked but independently editable Video and Audio clips,
real waveform, exact playback/seek/trim/split, audio controls, and hardware export
inside the declared matrix.

### Exit criteria

- VFR, B-frames, non-zero PTS and sample offsets use presentation/sample truth;
- Project Transport owns ProjectTime; audio endpoint may provide realtime pulse;
- supported profiles pass drift/seek/performance budgets on real hardware;
- production path contains no Qt Multimedia, software video decode, CPU video
  pixel transfer, or silent fallback;
- unsupported media fails early without project corruption.

## G5 — Complete Desktop Creator Loop

### Outcome

An internal user completes all three reference projects from install through
export on both Desktop platforms, manually or with the agent.

### Exit criteria

- every v1 capability satisfies Capability Definition of Done;
- save/reopen/export works on both platforms;
- relink, Undo/Redo, agent describe/measure/time/validate/commit/render-probe work;
- Internal Extension Registry backs built-ins; native developer extensions use
  an isolated shadow host without a public ABI promise;
- a new installer is emitted at gate exit.

## G6 — Reliability and release hardening

### Outcome

Turn the alpha into a supportable product through recovery, migrations, device
loss, budgets, updater rollback, diagnostics, symbols, SBOM, provenance,
security/privacy/licensing audits, accessibility, and clean-machine labs.

### Exit criteria

- fault corpus causes no project loss;
- N-2/N-1 projects migrate or open safely;
- interrupted updates yield old or new complete versions;
- performance and thermal budgets pass declared device tiers;
- no P0/P1 blockers; one signed/notarized RC is promotable without rebuild.

## G7 — Paid Founder Beta

### Outcome

External users can pay, install, activate, create, export, diagnose, update, and
recover without developer intervention. Telemetry is opt-in and excludes media,
project, and prompt content by default.

## G8 — Stable Desktop v1

### Outcome

Publish the supported media/device matrix, limitations, migrations, rollback,
support and security posture, then promote the qualified RC digest to stable.

## G9 — Mobile productization

### Outcome

Use the same project/command/evaluator semantics with separate adaptive UI,
lifecycle, sandbox, thermal, signing, and store policies. iOS uses Metal/native
media; Android uses Vulkan/MediaCodec surface paths. Downloadable native plugins
are not a mobile contract.

## G10 — Extension and SDK expansion

### Order

1. Internal Registry/Descriptors from G2.
2. Declarative presets and graphs after G5.
3. Sandboxed non-realtime workers when needed.
4. Stable C ABI in an out-of-process Plugin Host after v1 contracts stabilize.
5. Certified GPU/audio extensions and marketplace only after conformance,
   signing, compatibility, crash containment, and revocation exist.

## Capability Definition of Done

```text
Descriptor -> Typed Command -> Validation -> Accepted Revision
-> Serialization/Migration -> Inspector/Timeline -> Canvas Preview
-> Export -> Undo/Redo/Replay -> Agent Introspection
-> Platform Qualification -> Diagnostics/Documentation
```

## Gate Definition of Done

A gate passes only when all exit criteria, required platform artifacts,
evidence, failure/rollback tests, risks, and status updates are complete. A type,
skeleton, inactive backend, compile check, or demo inside an IDE is not a gate.

## Release spine

- Every PR: desktop core builds, tests, boundary/docs checks, schema migrations,
  shader/contract checks where applicable, dependency/license/SBOM drift.
- Nightly native lab: media corpus, seek/scrub/export soak, device loss, zero-CPU
  counters, GPU captures, visual/color/text tolerances.
- RC: clean protected runners, exact tag/lock/toolchain, symbols, SBOM,
  provenance, signing, clean-machine install/update/rollback.
- Promote one payload digest through Internal -> Alpha -> Paid Beta -> Stable;
  signing/store wrappers must retain traceable provenance.

## Program stop conditions

Stop and request a decision when a path violates an invariant, creates a second
truth, requires an unapproved license or destructive migration, lacks required
hardware/signing authority, or materially expands scope. Ordinary build/test
failures inside an authorized work package are not stop conditions.
