---
id: ADR-0002
kind: adr
status: proposed
title: Qt 6 QML as command-only Studio shell
owner_role: studio-architecture
decision_due: G0-WP02
last_verified: 2026-08-07
---

# Proposed decision

Use Qt 6/QML for the native cross-platform Studio shell. Qt owns controls,
panels, accessibility, input and view models; an engine presenter owns the
project Canvas surface. Qt types do not cross into engine contracts.

# Conditions

- Select commercial or LGPL compliance lane and an exact Qt release/module list.
- No Qt Multimedia product pipeline and no QImage/QPainter Canvas bridge.
- Prove native viewport integration on macOS and Windows in G1.

