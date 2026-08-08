---
id: G2-WP02
kind: work-package
status: proposed
gate: G2
owner_role: core-project-architecture
evidence: docs/evidence/G2/G2-WP02.md
---

# Outcome

Implement one versioned portable schema and capability/property registry for
the bounded project, hierarchy, Text/Shape/Image and animation model.

# Dependencies

G2-WP01 accepted decisions.

# Read first

- `docs/architecture/INVARIANTS.md`
- accepted outcome of RFC-0001 and RFC-0002
- accepted successor/version of `ARCH-VA-001`

# Allowed paths

`contracts/`, portable `src/core/`, generators, generated read-only artifacts,
unit/property/fuzz tests, examples and this package's evidence.

# Forbidden paths

Qt/Skia/OS/GPU types in Core; per-effect parser switches; QML-owned schema;
decimal-time or implicit pixel units; executable project C++; divergent platform
schemas; silent migration or unknown-property loss.

# Deliverables

- stable typed IDs for Project, Composition, Asset/Source, VisualLayer,
  ContentNode, LayerGroup, Effect and Mask ownership;
- explicit parent/order, half-open ranges and typed units;
- explicit `position parent_px` and `anchor local_px` semantics with a checked,
  stable-ID-preserving migration from experimental vocabulary;
- bounded TextBox width/height/padding, paragraph direction and horizontal/
  vertical alignment, line-height/spacing/wrap/overflow plus packaged Font
  Asset identity and fail-closed resolution rules;
- distinct authored geometry and derived layout/logical/ink/mask/effect/world
  bounds types; derived metrics are never editable serialized truth;
- descriptor/value/port registry with validation and qualification metadata;
- registry-addressed `owner-id + property-id` capability metadata, including
  an explicit animatable/not-animatable state for every property;
- Step/Linear/CubicBezier animation representation;
- canonical serialization, schema fingerprint and deterministic migration;
- generator projections for validation, format vocabulary, Inspector metadata,
  commands, CLI/MCP and Agent examples;
- cycle, duplicate-ID, invalid-unit/port/time and resource-limit rejection;
- exact rational/subframe experiment disposition without weakening ADR-0009.

# Verification

- serialize/parse/serialize is byte-stable for the same schema;
- migration is idempotent and preserves stable IDs/digest;
- registry projections share one digest and compile without hand-edited drift;
- TextBox/alignment and effect-ownership projections share that same digest;
- property/fuzz corpus fails closed under bounded memory/time;
- supported rate fixtures demonstrate named, drift-free mapping;
- architecture and docs checks pass on macOS and Windows Core lanes.

# Evidence path

`docs/evidence/G2/G2-WP02.md`.

# Failure and rollback

Reject the schema revision if it needs platform types, loses unknown data/IDs,
cannot migrate atomically, or requires a parser switch for routine descriptors.
Retain the previous accepted schema and migration path.

# Exact handoff condition

WP03 and WP04 receive an immutable schema/registry version, conformance corpus,
semantic digest rules and accepted exact-time representation.
