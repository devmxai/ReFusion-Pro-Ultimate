---
id: ADR-0001
kind: adr
status: accepted
title: C++20 portable core baseline
owner_role: principal-architecture
decision_due: G0-WP02
last_verified: 2026-08-07
---

# Context

ReFusion requires one native engine across macOS, Windows, iOS, and Android,
while Skia's current build requires C++20. A newer language baseline must not
reduce supported compilers or create a separate mobile implementation.

# Decision

Use C++20 as the portable engine and public build baseline. Newer compiler
features may be used only behind explicit capability checks and may not enter a
public contract until all target toolchains qualify them.

# Consequences

- Core and contracts remain portable and testable without Qt or platform SDKs.
- Clang/MSVC differences are handled in toolchains/adapters, not semantics.
- C++23 may be reconsidered after the desktop/mobile toolchain matrix passes.

# Evidence

- The foundation Core, CLI, and test compile as C++20.
- Current Skia build documentation requires a C++20-capable compiler.
- The decision minimizes divergence across MSVC, Apple Clang, Android Clang,
  and iOS toolchains while preserving a later language-baseline ADR.
