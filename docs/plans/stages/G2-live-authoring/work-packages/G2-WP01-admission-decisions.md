---
id: G2-WP01
kind: work-package
status: proposed
gate: G2
owner_role: program-architecture
evidence: docs/evidence/G2/G2-WP01.md
---

# Outcome

Enter G2 with accepted, testable decisions for the canonical project authoring
format, hierarchy/compositing model, exact reference slices, budgets and format
migration authority.

# Dependencies

G0/G1 exit review, EXP-001/EXP-001A/EXP-002/EXP-004/EXP-005/EXP-006 evidence,
owner review EV-VA-0001, RFC-0001, RFC-0002, ADR-0008 and ADR-0009. Planning
can occur earlier; acceptance cannot be fabricated.

# Read first

- `docs/plans/MASTER_PLAN.md`
- `docs/architecture/INVARIANTS.md`
- `docs/research/visual-authoring-hierarchy-screening-draft.md`
- `docs/decisions/rfcs/RFC-0001-project-rfx-authoring-experiment.md`
- `docs/decisions/rfcs/RFC-0002-visual-authoring-hierarchy.md`
- `docs/architecture/VISUAL_AUTHORING_MODEL.md`

# Allowed paths

Decision, architecture, experiment, schema-spike, fixture, evidence, status and
plan paths needed to settle entry decisions. Bounded throwaway probes may enter
`experiments/` or tests; no product feature expansion is authorized here.

# Forbidden paths

No silent RFC approval, shipping-format claim, broad FX implementation, public
node/plugin API, G2 activation without entry evidence, or rewrite of accepted
ADR history.

# Deliverables

- written disposition of EXP-001 and EXP-001A;
- written disposition of the partial/revision-required findings in EXP-002,
  EXP-004, EXP-005 and EXP-006;
- accepted/revised/rejected decisions for RFC-0001 and RFC-0002, followed by
  ADRs when accepted;
- explicit disposition/supersession of ADR-0008;
- bounded VS-01 Subscribe Group and Text/Shape/Image fixture definitions;
- explicit bounded decisions for TextBox/alignment, Font identity, measurement
  ownership, effect ownership and topology-preserving authoring intents;
- exact device tiers, fault corpus, thresholds and visual tolerance method;
- confirmed WP dependency graph, owners and evidence schema;
- decision on the experiment needed for exact rational/subframe representation.

# Verification

- decision register is generated and current;
- every accepted claim points to evidence and an owner;
- schema/render/time choices are not hidden in code or test fixtures;
- macOS-only results remain labelled and cannot satisfy Windows entry evidence;
- `rfdev.py docs-doctor`, `rfdev.py architecture-check`, and repository policy
  tests pass.

# Evidence path

`docs/evidence/G2/G2-WP01.md` after the decision review actually occurs.

# Failure and rollback

If evidence does not support a candidate, revise/reject the RFC and retain the
experimental format behind its current boundary. Do not migrate user projects
or activate downstream WPs.

# Exact handoff condition

WP02 may start only when the project format, hierarchy contract, reference
fixture and exact acceptance budgets are recorded by authorized decisions.
