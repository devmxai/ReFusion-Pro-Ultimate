---
id: UCAS-WP05
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G2-bounded,G3-breadth,G10-public-packages
owning_gate: G2
depends_on: UCAS-WP02,UCAS-WP03
decision_dependencies: recipe-IR-package-version-trust-ADRs
cross_plan_dependencies: MP-001,G2-WP02,G2-WP06,G10
evidence_owner: G2-WP06
owner_role: recipe-compiler
evidence: docs/evidence/G2/G2-WP06.md
---

# Outcome

Compile a versioned parameterized Recipe into a deterministic normalized
ChangeSet without adding a live evaluator or platform-specific authoring path.

The G2 slice is built-in, bounded and materialized only. Downloadable/public
packages and live declarative graphs remain G10 after G5.

# Dependencies

UCAS-WP02, UCAS-WP03 and accepted Recipe/package/version decisions.

# Deliverables

- RecipeDescriptor and typed ParameterDescriptor contracts;
- bounded declarative Recipe IR and declared topology/ownership effects;
- pure compiler, stable slot model and deterministic entity-ID allocator;
- package manifest, immutable version/content digest and exact dependency lock;
- depth, node, operation, memory, pass and asset budgets;
- signature, publisher, license, provenance and permission fields;
- explicit package/descriptor migration and conformance-fixture format.

# Verification and exit

- identical inputs produce identical ChangeSet bytes/digest on Clang and MSVC;
- compilation uses no network, wall clock, OS font, random source, native code or
  backend state;
- unknown operation, missing dependency, cycle or budget excess fails closed;
- compile cannot publish state; only UCAS-WP03 Commit may do so;
- no dependency upgrade occurs during project open.

# Failure and rollback

Disable package compilation/loading. Previously materialized normal project
entities remain valid and renderable.
