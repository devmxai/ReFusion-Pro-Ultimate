---
id: ADR-0014
kind: adr
status: accepted
title: Portable media assets, linked clips and exact relink policy
owner_role: project-media-architecture
decision_due: VI-WP00
last_verified: 2026-08-09
accepted_by: product-owner-user-instruction-2026-08-09
---

# Context

ReFusion currently serializes Shape/Text/Group state but has no portable media
asset, source, VideoClip or AudioClip identity. A file path is not a project
identity: it changes when a project moves between macOS and Windows and cannot
be allowed into canonical project truth. A single imported container can own
both Video and Audio streams, while the Timeline must expose two independently
editable tracks without duplicating the underlying file.

# Decision

Extend the current canonical `Project.rfx` experiment with one versioned RFX6
media slice. RFX6 is the vertical-slice format, not an implicit final shipping
format decision for all of G2. RFX1-RFX5 remain readable migration inputs and
materialize empty media collections.

Use these stable portable identities:

```text
AssetRecord
  AssetId + sha256 + byte_size + media_kind
  project_relative_original + original_display_name + provenance

MediaSource
  MediaSourceId + AssetId + media_index_contract_version
  selected video/audio StreamId + stream metadata/index digest
  resolution state: resolved | missing | digest_mismatch | unsupported

LinkedImport
  LinkedImportId + MediaSourceId + optional VideoClipId + optional AudioClipId

VideoClip / AudioClip
  independent stable ClipId + LinkedImportId + MediaSourceId + StreamId
  Composition half-open range + exact signed source in/out
  enabled + locked + typed clip properties
```

One clip can be selected, muted, disabled, moved or trimmed independently. The
link is explicit identity/provenance, not coupled UI state. Future unlinking is
a typed command; duplicating the container or manufacturing hidden Layers is
not.

The first slice admits **copy import only**. The file portal returns an opaque
host token; the filesystem adapter streams it into a staging area while
computing SHA-256. The committed project layout is:

```text
<Project>/
  Project.rfx
  Assets/
    Media/<asset-id>/original.<validated-extension>
  .refusion/
    Cache/MediaIndex/<media-source-id>/<source-digest>.rfxmi
    Cache/Waveforms/<audio-clip-id>/<audio-digest>.rfxwf
    Journal/Import/<transaction-id>/...
    Diagnostics/...
```

`Assets/Media` stores the container once even when both linked clips exist.
Media index and waveform files are derived, digest-bound and rebuildable. They
never replace `Project.rfx` plus the immutable asset bytes as legal truth.

Import materializes the copied original, index and cache staging first; then one
atomic ChangeSet publishes `AssetRecord + MediaSource + LinkedImport + clips +
root order` as a single accepted Revision. Cancellation, failure or process
interruption publishes none of them. Project source is committed last through
the existing journal/LKG authority.

# Relink and replacement

- Normal move/copy portability needs no relink because the original is inside
  the project package.
- A missing asset can be relinked only to byte-identical content in this first
  slice. The adapter verifies SHA-256 and byte size before restoring the
  original; the semantic project Revision does not change.
- Selecting different bytes is not relink. It requires a future explicit
  `ReplaceMediaSource` ChangeSet that reindexes, revalidates, records a new
  source identity and prepares Runtime before atomic publication.
- Absolute source paths, macOS bookmarks, Windows shell tokens and recent-file
  paths are host-local `.refusion` state only and are regenerated or discarded
  on another host.
- Missing/digest-mismatched media round-trips with all source state intact,
  renders as unresolved with typed diagnostics and preserves Last-Known-Good;
  it is never replaced by black/silence as an accepted approximation.

# Agent/UI/MCP authority

UI, CLI and future MCP submit the same typed `ImportVideoIntent`,
`SetClipRange`, `SetClipEnabled` and `RelinkExactAsset` commands. Agents address
stable IDs and exact time/sample values returned by engine inspection. No Agent
writes cache/index bytes, invents an asset ID from a filename or edits more than
one truth file to perform an import.

# Alternatives rejected

- **Absolute-path project references:** non-portable and forbidden.
- **One copied file per Video/Audio track:** wastes storage and breaks shared
  provenance.
- **Reference import in the first slice:** deferred until cross-host token,
  offline and relink behavior can be qualified without weakening portability.
- **Store full sample indexes/waveforms in Project.rfx:** rejected because they
  are large, derived and rebuildable.
- **Two independent Video and Audio import revisions:** rejected because partial
  publication would violate linked-import atomicity.

# Consequences

Copy import uses project storage but guarantees that a macOS-created project
contains the exact bytes Windows needs. Large-source reference workflows can be
added later as a separate capability with host-local leases and explicit
offline semantics. VI-WP01 must define canonical RFX6 grammar/migration and
conformance receipts before any Studio import button is connected.
