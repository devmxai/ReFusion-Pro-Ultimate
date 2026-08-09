---
id: UCAS-WP09A
kind: work-package
status: proposed
plan: PLAN-UCAS-001
stage_route: G3-asset-spine,G5-provider-integration
owning_gate: G3
depends_on: UCAS-WP02,UCAS-WP03,UCAS-WP04
decision_dependencies: asset-identity-ingest-generation-privacy-license-ADR
cross_plan_dependencies: MP-001,G3,G4-media-asset-boundaries
evidence_owner: future-G3-stage-plan
owner_role: asset-ingest-and-provenance
evidence: docs/evidence/UCAS/UCAS-WP09A.md
---

# Outcome

Provide one content-addressed Asset path for imported, generated and relinked
creative media without giving an Agent, provider or renderer filesystem authority.

# Deliverables

- AssetId, immutable content digest, type/profile and project-relative locator;
- bounded ingest, MIME/signature verification, decompression and size limits;
- provenance, source, provider/model/version, prompt-policy, license, consent,
  privacy and C2PA-compatible metadata where applicable;
- generated-asset request as a privileged plan step followed by explicit ingest;
- cancellation, retry, quarantine, relink and missing-asset diagnostics;
- no temporary URL or network access during project evaluation/render;
- separate G5 provider integration over the same ingest contract.

# Verification and exit

- path traversal, symlink escape, MIME spoof and decompression-bomb corpus rejects;
- equal bytes produce one content identity across macOS/Windows;
- failed/cancelled generation cannot publish a partial project revision;
- missing/revoked assets preserve project state and fail closed;
- provider data handling, licensing and user approval are auditable.

# Failure and rollback

Disable generation/providers and retain local verified ingest. Never substitute a
different asset silently or expose raw host paths to remote clients.

