---
id: G1-WP06
kind: work-package
status: planned-development-only
gate: G1
owner_role: desktop-release
evidence: docs/evidence/G1/G1-WP06.md
---

# Outcome

Produce traceable, non-redistributable macOS and Windows development packages
from the same source revision with dependency fingerprints, symbols and
clean-machine install/launch/uninstall receipts.

# External entry evidence

Clean macOS and Windows test machines plus protected build runners. Qt Commercial
SDK/entitlement and production signing authorities are explicitly deferred to
the G6 redistributable RC gate.

# Required proof

The local development payload records its non-redistributable Qt source,
runtime module census, source/toolchain/provenance and payload digest; clean test
machines install, launch, render the fixture and uninstall without hidden paths.
This proof must not be promoted or distributed as a release artifact.
