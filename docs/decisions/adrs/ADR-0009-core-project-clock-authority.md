---
id: ADR-0009
kind: adr
status: accepted
title: Core-owned canonical Project Clock authority
owner_role: core-transport
decision_due: G1-WP03
last_verified: 2026-08-07
accepted_by: product-owner-user-instruction-2026-08-07
---

# Context

Project playback must not acquire competing notions of time from the UI,
viewport, video decoder, audio clip, render loop, or a platform callback. A
physical clock is still required during realtime playback, but a device clock
cannot own project semantics and offline rendering has no realtime device clock.

The earlier walking slice stored mutable playback position, epoch and running
state in `ViewportRenderSession`. That was sufficient for a bounded visual
fixture but did not physically enforce the documented single-authority rule.

# Decision

`refusion::core::ProjectClock` is the sole mutable authority for canonical
`ProjectTime` in an engine session. It owns transport state, position, loop
index, epoch ID, source generation and sample sequence. Play, Pause, Stop, Seek
and realtime samples are validated state transitions on this object.

Runtime and platform code may provide `ClockTick` pulses. A host-monotonic clock
is the bounded source now; a qualified audio-endpoint clock becomes the preferred
forward-playback source in G4. A source pulse has no authority to set Timeline,
video, animation or audio state directly. Source-generation changes and backward
ticks fail closed until an explicit re-correlation policy is accepted.

Every scheduled visual/video evaluation consumes a snapshot from the Core clock
and carries its `epoch_id`. The current PTS-indexed decoded-surface queue selects
video with that exact snapshot time. UI controls submit transport commands and
display snapshots; they own no timer. Offline export will drive the same Core
authority/evaluator with deterministic requested timestamps rather than a wall
clock.

The portable Core API contains no Qt, Skia, codec, OS, audio-device, or native
GPU type. Clock sources remain Runtime/platform adapters.

# Options considered

- Keep time inside `ViewportRenderSession`: rejected because presentation would
  become a second project-time owner.
- Make the audio engine or active clip the master: rejected because silence,
  route changes, scrubbing and offline export would change project authority.
- Let each subsystem synchronize its own clock: rejected because drift and
  stale work cannot be invalidated under one epoch.
- Put platform clock APIs in Core: rejected because Core would stop being
  portable and device ticks would be confused with project-time authority.

# Consequences

- Runtime presentation is a scheduler and clock-source adapter, not a time
  owner.
- Timeline, Canvas, decoded-video selection and future audio/FX evaluation must
  consume one `ProjectClockSnapshot`/compatible evaluation stamp.
- Route changes require explicit source-generation correlation; silent clock
  switching is forbidden.
- Frame-rate conversion is direct checked integer/rational mapping with no
  cumulative frame-duration addition and no floating-point seconds.
- Final audio-master drift, latency, route-change and offline equivalence remain
  G4 qualification work; this ADR fixes authority and ownership now, not those
  future hardware measurements.

# Qualification evidence

- Core unit tests cover Play/Pause/Seek/Stop, loop continuity, non-monotonic and
  wrong-generation rejection, NTSC frame mapping and immutable snapshots.
- Runtime tests prove the presenter receives the Core epoch and time, with no
  local mutable playback position.
- The G1-WP03 integration proof seeks through Core and selects decoded GPU frames
  `[3, 7, 1]` by exact PTS while all forbidden media-path counters remain zero.

# Supersedes

No earlier ADR. It converts the transport-authority rule in the architecture
invariants and Master Plan G4 from documentation-only intent into executable
Core ownership.
