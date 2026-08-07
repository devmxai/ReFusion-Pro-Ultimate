---
id: G1-WP06
kind: work-package
status: blocked-awaiting-commercial-and-signing-authority
gate: G1
owner_role: desktop-release
evidence: docs/evidence/G1/G1-WP06.md
---

# Outcome

Produce traceable macOS and Windows internal installers from the same source
revision with exact Commercial Qt SDKs, dependency notices/SBOM, symbols and
clean-machine install/launch/uninstall receipts.

# External entry evidence

Qt Commercial entitlement covering version/seats/CI/products, Apple signing and
notarization credentials, Windows signing authority, protected runners, and
named secret owners.

# Required proof

No Homebrew/developer Qt in payload; runtime module census matches policy; one
payload digest maps to source/toolchain/SBOM/provenance; clean machines install,
launch, render the fixture and uninstall without developer paths.
