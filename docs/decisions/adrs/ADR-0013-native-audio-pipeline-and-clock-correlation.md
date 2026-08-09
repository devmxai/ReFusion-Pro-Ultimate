---
id: ADR-0013
kind: adr
status: accepted
title: Native AAC audio pipeline and Core clock correlation
owner_role: audio-runtime-architecture
decision_due: VI-WP00
last_verified: 2026-08-09
accepted_by: product-owner-user-instruction-2026-08-09
---

# Context

The first imported MP4/MOV must create an independently editable AudioClip,
real waveform and synchronized playback without making an audio callback, clip
or device the owner of project time. ADR-0009 already establishes
`core::ProjectClock` as the sole mutable project-time authority and permits a
qualified audio endpoint to become the preferred forward-playback tick source.

Audio samples may be decoded and processed on the CPU; ADR-0004's zero-pixel
rule applies to decoded **video pixels**, not PCM audio. Using FFmpeg to decode
AAC would nevertheless broaden the new demux dependency and create a second
software-codec policy that is unnecessary on the declared Desktop platforms.

# Decision

Adopt one portable audio packet/PCM/scheduling contract with thin native codec
and endpoint adapters:

- macOS decodes AAC-LC packets through AudioToolbox `AudioConverter` and renders
  through a Core Audio output unit/HAL endpoint;
- Windows decodes AAC-LC packets through the Microsoft Media Foundation AAC
  decoder MFT and renders through WASAPI;
- both adapters output engine-owned, bounded, planar IEEE float32 PCM blocks at
  the source rate (44.1 or 48 kHz), with one or two explicitly ordered channels;
- platform endpoint adapters may interleave and resample PCM to the physical
  mix format, but must report the conversion, latency, source generation and
  exact sample-position correlation;
- the portable layer owns `AudioStreamDescriptor`, signed start offset,
  integer source-sample ranges, `AudioPcmBlock`, gain/mute/solo seed,
  discontinuity and underflow diagnostics;
- waveforms are derived, versioned cache artifacts from the admitted PCM
  stream. They are not project truth and can be rebuilt without changing the
  accepted Revision.

During forward playback with an admitted AudioClip, the endpoint supplies a
correlated tick:

```text
AudioClockTick
  endpoint_generation
  device_position + device_frequency
  correlated_host_monotonic_time
  submitted/heard sample ranges
  measured output latency
```

Core validates that tick and advances `ProjectClock`; the endpoint never writes
project time directly. Pause and seek increment the transport epoch, flush
decoder/output queues, seek by exact source sample, preroll, establish a new
correlation and only then resume. Endpoint removal or generation change pauses
and fails closed until an explicit new correlation succeeds. Muting a clip
does not remove its admitted endpoint clock; a project with no audio uses the
host-monotonic source defined by ADR-0009.

# Exact audio rules

- AAC profile is LC only; HE-AAC, SBR/PS, surround and dynamic format change are
  rejected in this slice.
- Source sample time is signed integer samples plus a positive sample rate.
- Priming, roll distance, end padding and MP4 edit-list offsets are explicit
  metadata; they are never guessed or accumulated as floating-point seconds.
- PCM values are finite float32. NaN/Inf and invalid channel layouts fail.
- Bounded queues apply backpressure; no UI callback or file watcher can feed
  realtime audio directly.
- Audio device selection, interleaving and endpoint buffer mechanics are native
  responsibilities; clip timing, gain meaning and A/V synchronization are not.

# Qualification target

- steady-state absolute A/V drift: p95 no greater than 10 ms and maximum no
  greater than 20 ms after warm-up;
- start/seek correlation settles within 500 ms and no later than one video
  frame plus 20 ms from the requested project sample;
- no unreported discontinuity, unbounded queue or stale-epoch PCM publication;
- macOS and Windows persist the same source sample ranges and clip offsets even
  when endpoint mix formats differ.

# Alternatives rejected

- **Audio device or AudioClip as master project clock:** rejected by ADR-0009.
- **FFmpeg AAC decode:** rejected for this slice because native qualified
  decoders already exist and demux-only dependency scope must remain narrow.
- **Qt Multimedia/QMediaPlayer:** forbidden because it would own playback,
  decoding and timing outside the engine.
- **Persisted waveform samples as project truth:** rejected because they are a
  derived cache and would make decoder/build differences a project mutation.

# Primary references

- `https://developer.apple.com/documentation/audiotoolbox/audio-converter-services`
- `https://developer.apple.com/documentation/AudioToolbox/encoding-and-decoding-audio`
- `https://learn.microsoft.com/en-us/windows/win32/medfound/aac-decoder`
- `https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclock-getposition`
