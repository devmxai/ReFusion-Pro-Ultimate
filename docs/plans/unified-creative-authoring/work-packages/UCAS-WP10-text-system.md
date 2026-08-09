---
id: UCAS-WP10
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G3
owning_gate: G3
depends_on: UCAS-WP06,UCAS-WP07,UCAS-WP08B,UCAS-WP09A
decision_dependencies: deterministic-text-selector-typography-ADRs
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,future-G3-stage-plan
evidence_owner: future-G3-stage-plan
owner_role: typography-and-text-motion
evidence: docs/evidence/UCAS/UCAS-WP10.md
---

# Outcome

Provide professional Text styling and animation while keeping one Text Layer,
cluster-safe typography and owner-local properties/FX.

# Dependencies

UCAS-WP08B, UCAS-WP09A, deterministic packaged-font/text layout and active G3.

# Deliverables

- packaged-font identity, face/style/variation/features/language/script contract;
- paragraph, alignment, tracking, leading, baseline, box and overflow controls;
- Text Animator/Selector model over character, grapheme cluster, word and line;
- Range, Wiggly and deterministic seeded-order selectors where admitted;
- type-on, word/line build, fade, slide, scale, rotate, tracking, mask reveal and
  annotation Recipes with editable parameters;
- Arabic/RTL/mixed-direction rules that preserve shaping and cluster integrity;
- Text FX and animation projected inside the Text owner, not duplicate Layers.

# Verification and exit

- Latin, Arabic, RTL, mixed, diacritic and wrap fixtures pass cross-platform;
- logical/ink/effect/world bounds remain measurable at exact times;
- one Text Layer remains one root Timeline row when collapsed;
- selector and Recipe results are deterministic and UI/Agent-equivalent;
- no per-character root Layer or system-font fallback in qualified profiles.

# Failure and rollback

Reject unsupported selector/font modes with typed diagnostics and retain the Text
Layer and Last-Known-Good.
