---
id: UCAS-WP09
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G3
owning_gate: G3
depends_on: UCAS-WP05,UCAS-WP06,UCAS-WP07,UCAS-WP08B,UCAS-WP09A
decision_dependencies: background-generator-color-resource-ADRs
cross_plan_dependencies: MP-001,PLAN-XPLAT-FIX-001,future-G3-stage-plan
evidence_owner: future-G3-stage-plan
owner_role: background-and-generators
evidence: docs/evidence/UCAS/UCAS-WP09.md
---

# Outcome

Deliver a parameterized Background system that spans native paints, patterns,
assets and deterministic generators without expanding root Timeline clutter.

# Dependencies

UCAS-WP05–08B, UCAS-WP09A, active G3 and admitted Asset capabilities.

# Deliverables

- solid, linear, radial and sweep gradients with explicit color interpolation;
- bounded patterns, geometric compositions and content-addressed image/video
  backgrounds when their asset/media gates are admitted;
- seeded/versioned noise, grain, paper, texture and procedural fields;
- animated gradient, drift, parallax, evolution and lighting Recipes;
- one Background owner or intentional Group with internal nodes hidden from the
  root Timeline by default;
- real parameters for color, stops, angle, scale, seed, speed, intensity,
  softness, blend, opacity and quality profile;
- resource/pass/bounds/color/tile/crop policies and provenance.

# Verification and exit

- every generator is deterministic and versioned;
- Recipe output remains editable and atomically updates owned channels;
- assets use stable IDs/digests rather than absolute paths or temporary URLs;
- preview/export and macOS/Windows semantic/visual/performance evidence pass;
- missing assets/capabilities fail closed without changing Last-Known-Good.

# Failure and rollback

Disable the unavailable generator/Recipe. Existing materialized results remain;
no silent substitution with a downloaded raster is allowed.
