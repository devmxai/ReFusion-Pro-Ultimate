---
id: RFC-0001
kind: rfc
status: experimenting
title: Typed single-file Project.rfx authoring experiment
owner_role: product-owner
decision_due: G2-entry
---

# Problem

The JSON project-open seed proves file-backed rendering but is not the final
agent-authoring format. A previous native-C++ workspace made one source file
obvious to an agent, but required compile/link/load for ordinary edits, exposed
executable project code, duplicated project metadata, and could not provide
safe UI/source round-tripping for arbitrary C++.

# Required decision

Determine whether a single typed declarative `Project.rfx` source gives an
external agent a precise and economical authoring surface while preserving one
native C++ semantic model, one accepted revision, one evaluator and one render
graph.

# Options and trade-offs

- Keep JSON: smallest implementation step, but weak unit/type ergonomics for
  direct agent edits and not the selected experiment.
- Make `project.cpp` canonical: native procedural freedom, but executable,
  compile-bound, unsafe to rewrite from UI commands and unsuitable for mobile.
- Typed `Project.rfx`: named fields and units, deterministic compilation to Core
  types and fast live reload, at the cost of owning a versioned grammar/compiler.
- Hybrid: `Project.rfx` for project semantics and optional isolated C++ modules
  for new procedural sources/effects. This is the experiment recommendation.

# Experiments and evidence

`EXP-001` must prove a real bounded vertical slice, not empty folders:

1. open a canonical `Project.rfx` project;
2. compile it in portable C++ into the existing immutable `ProjectSnapshot`;
3. render the accepted Shape/Text/animation composition through the existing
   Skia GPU path;
4. expose deterministic validate/describe commands;
5. ship project-local `AGENTS.md` and a discoverable authoring Skill whose
   examples compile;
6. reject malformed, stale or semantically invalid candidates without replacing
   Last-Known-Good;
7. retain exact source locations in diagnostics.

This experiment does not claim Video/Audio import, masks, general FX, save/undo,
shipping migrations, mobile live compilation or the final G2 format.

# Security, licensing and platform impact

`Project.rfx` is non-executable data. The compiler is standard C++ and its Core
result contains no Qt, Skia, OS, codec or GPU handles. No dependency or license
is added. Native extensions remain outside this experiment and subject to the
existing isolation/mobile policy.

# Recommendation

Run the typed single-file experiment. Keep ADR-0008's JSON document as historical
seed evidence; do not extend it into a shipping-format claim. Decide at G2 entry
whether to accept, revise or reject `Project.rfx` based on the experiment.

# Final disposition

Experiment authorized by the product owner on 2026-08-07. Architectural adoption
remains undecided until G2-entry evidence review.
