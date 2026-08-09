---
id: UCAS-WP08B
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G3
owning_gate: G3
depends_on: UCAS-WP06,UCAS-WP07,UCAS-WP08A
decision_dependencies: professional-motion-spatial-spring-ADR
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,future-G3-stage-plan
evidence_owner: future-G3-stage-plan
owner_role: professional-motion
evidence: docs/evidence/UCAS/UCAS-WP08B.md
---

# Outcome

Extend the accepted curve truth into professional value/speed editing, spatial
paths and reusable parameterized motion without hand-written keyframe hacks.

# Dependencies

UCAS-WP08A, UCAS-WP06 and active G3.

# Deliverables

- temporal/value handle model and Speed/Value Graph projections;
- separate spatial paths and temporal progress;
- Auto, Continuous, Broken and Roving authoring operations where accepted;
- reusable ease, fade, slide, scale-pop, overshoot, bounce, elastic, pulse,
  repeat and ping-pong Recipe families;
- deterministic Spring/decay contract with settle and overshoot bounds;
- typed stagger, sequence, property composition/conflict and swept-bounds rules;
- Graph Editor that edits Core curves only.

# Verification and exit

- Speed and Value views round-trip to one curve digest;
- UI and Agent motion edits normalize and evaluate identically;
- Spring/non-settle, overshoot and swept-bounds fixtures pass;
- no native platform spring or sampled mystery preset is used;
- desktop semantic and performance qualification passes for the admitted set.

# Failure and rollback

Disable advanced editors/Recipes while preserving the canonical base track and
Last-Known-Good project state.
