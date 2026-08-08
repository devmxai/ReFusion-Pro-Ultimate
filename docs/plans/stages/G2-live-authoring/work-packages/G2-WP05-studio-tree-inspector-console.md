---
id: G2-WP05
kind: work-package
status: proposed
gate: G2
owner_role: studio-authoring
evidence: docs/evidence/G2/G2-WP05.md
---

# Outcome

Project the accepted hierarchy and registry into a native Qt Timeline tree,
minimal Inspector and Console while preserving the command-only UI boundary.

# Dependencies

G2-WP03 immutable revision/diagnostic feeds and G2-WP04 hierarchy/evaluation
snapshots.

# Read first

- `docs/architecture/INVARIANTS.md`
- G2-WP03/G2-WP04 contracts
- generated registry Inspector metadata

# Allowed paths

`apps/studio/`, Studio adapters/view models, generated presentation metadata,
UI automation/accessibility tests and evidence.

# Forbidden paths

Project model, clock, decoder, renderer, cache, acceptance queue or mutable
hierarchy in QML; hand-coded descriptor family switches; `QImage`, `QPainter`,
Qt Multimedia or Qt-owned Canvas frames.

# Deliverables

- immutable `TimelineTreeModel` keyed by stable semantic IDs;
- collapsed/expanded group rows, selection and drag/reparent intents;
- double-click drill-down, breadcrumb and deterministic focus restore;
- FX, masks and animation-property lanes nested under their semantic owner;
  none projects as an independent Visual Layer or NLE Track;
- schema-driven controls for the bounded Text/Shape/Image/Group properties;
- TextBox/alignment controls, measured read-only bounds and typed node-to-node
  alignment commands sourced only from accepted snapshots;
- shared selection reflected by Timeline, Inspector and Canvas snapshot;
- Console projection of candidate/revision/evaluation diagnostics;
- keyboard/accessibility paths and responsive desktop layout;
- typed command receipts, busy/conflict/rejection states and no UI-owned timers.

# Verification

- Subscribe Group is one collapsed row and expands to stable children;
- a composite Background is one collapsed root Group, and adding local Glow
  does not add a Timeline Layer row;
- every UI edit emits a typed intent and updates only after acceptance;
- Agent/source revisions update Timeline, Inspector and Canvas on one stamp;
- rejection leaves all panels on the same prior revision;
- no descriptor requires a QML command-family switch;
- architecture check, UI automation and physical macOS/Windows interaction pass.

# Evidence path

`docs/evidence/G2/G2-WP05.md`.

# Failure and rollback

If a view cannot represent a new descriptor generically, it reports an explicit
unsupported editor and retains read-only inspection; it may not mutate hidden
state or invent defaults.

# Exact handoff condition

The bounded project is fully editable from UI commands and externally accepted
changes appear coherently across all Studio projections.
