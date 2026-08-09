---
id: UCAS-WP00
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: pre-G2-planning
owning_gate: planning-only
depends_on: none
decision_dependencies: none
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,RESEARCH-AAPR-001
evidence_owner: UCAS-WP00
owner_role: creative-systems-research
evidence: docs/evidence/UCAS/UCAS-WP00.md
---

# Outcome

Freeze the current technical baseline, vocabulary and named professional
workflow corpus before architecture decisions or implementation expand.

# Entry

- `RESEARCH-AAPR-001` has completed screening.
- MP-001 and PLAN-XPLAT-FIX-001 remain unchanged authorities.
- Current code, catalog, CLI/MCP and qualification claims are auditable.

# Deliverables

- Correct/Partial/Wrong/Missing inventory of current capabilities;
- stable vocabulary for Capability, Descriptor, Recipe, Preset, Style Profile,
  Style Pack, Receipt, Generator, Curve and Extension;
- named corpus covering backgrounds, text, shapes, motion, FX, choreography and
  one editorial paper-collage scene;
- measurable coverage method for any future 60–70% workflow claim;
- baseline latency, token/context, project-size and render metrics;
- catalog taxonomy with research-only versus admitted capability labels;
- decision-question register routed to UCAS-WP01.

# Verification and exit

- no missing feature is represented as implemented or qualified;
- every corpus item names required primitive capabilities and platform profile;
- coverage is measured against the named corpus, not an open-ended style list;
- the owner accepts the corpus and vocabulary as the planning baseline;
- evidence records the exact repository commit and audit commands.

# Failure and rollback

This package is documentation-only. Reject or revise the corpus without code,
project-format or migration impact.
