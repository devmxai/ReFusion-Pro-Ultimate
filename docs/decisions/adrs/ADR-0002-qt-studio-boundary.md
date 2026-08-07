---
id: ADR-0002
kind: adr
status: accepted
title: Qt 6 QML as command-only Studio shell
owner_role: studio-architecture
decision_due: G0-WP02
last_verified: 2026-08-07
---

# Decision

Use Qt 6/QML for the native cross-platform Studio shell. Qt owns controls,
panels, accessibility, input and view models; an engine presenter owns the
project Canvas surface. Qt types do not cross into engine contracts.

# Conditions

- Select commercial or LGPL compliance lane and an exact Qt release/module list.
- No Qt Multimedia product pipeline and no QImage/QPainter Canvas bridge.
- Prove native viewport integration on macOS and Windows in G1.

# Scope of acceptance

The architectural boundary and current Qt 6.11.1 development baseline are
accepted. Distribution under Qt Commercial or LGPL is intentionally separated
into ADR-0005 because it requires the product owner's legal/financial choice.
