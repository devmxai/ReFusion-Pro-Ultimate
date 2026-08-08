---
id: G2-WP07
kind: work-package
status: proposed
gate: G2
owner_role: live-authoring-release
evidence: docs/evidence/G2/G2-WP07.md
---

# Outcome

Integrate the accepted G2 spine into traceable macOS and Windows development
artifacts and prove the complete create/open/edit/group/save/reopen/recover loop.

# Dependencies

G2-WP01 through G2-WP06 complete with accepted decisions and evidence.

# Read first

- `docs/plans/stages/G2-live-authoring/PLAN.md`
- all G2 WP evidence
- `docs/product/PRODUCT_CONTRACT.md`
- platform/release policies and current risk register

# Allowed paths

Integration, packaging, fixtures, clean-machine scripts, performance/fault/UI
tests, evidence, status/checkpoint and release documentation.

# Forbidden paths

Waiving Windows, substituting screenshots for semantic evidence, hidden CPU
video work, redistributable licensing claims, broad G3/G4 feature expansion, or
marking G2 complete with planned/compile-only evidence.

# Deliverables

- real empty-project creation and accepted reference project round-trip;
- UI and external Agent edit sessions on the same workspace;
- Subscribe Group collapsed/drill-down/parent-child animation demonstration;
- sanitized Reels regression with one Background Group, one Title Layer owning
  local Shadow/Glow and one Subscribe Group with measured Text alignment;
- exact ID, unit, time, digest and migration preservation receipts;
- UI/Agent AddEffect and AlignNodes parity plus topology-count postconditions;
- invalid/stale/partial/cycle/port/device-failure recovery corpus;
- preview/offline semantic-equivalence probe;
- calibrated performance/memory results and capability telemetry;
- macOS arm64 and Windows x64 clean-machine artifacts, provenance, launch,
  project round-trip, crash recovery and uninstall evidence;
- exit checkpoint with exact remaining G3/G4 risks.

# Verification

- all G2 plan matrix rows point to immutable evidence;
- test/architecture/docs/dependency checks pass on both desktop lanes;
- physical platform receipts identify OS, CPU/GPU/device/driver and artifact;
- no candidate can create mixed Timeline/Inspector/Canvas revisions;
- the Reels regression preserves root count, hierarchy, TextBox/alignment and
  FX ownership across save/reopen, and rejects unsupported animated FX without
  changing Last-Known-Good;
- failure rollback retains the same recoverable project and LKG;
- no G3/G4 or paid-product claim is attached to the artifact.

# Evidence path

`docs/evidence/G2/G2-WP07.md`.

# Failure and rollback

Any missing platform, corruption, semantic divergence, unbounded resource growth
or second-authority finding blocks G2. Keep the last green development artifact
and return to the owning WP; never promote a partially qualified build.

# Exact handoff condition

An authorized gate review confirms every G2 exit criterion and updates MP-001
status. Only then may the detailed G3 plan be finalized and activated.
