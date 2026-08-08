---
id: EXP-005
kind: work-package
status: implementation-passed-owner-evaluation-revision-required
gate: pre-G3-experiment
owner_role: compositing-effects
evidence: docs/evidence/experiments/EXP-005.md
---

# Ordered Layer Mask and FX stack experiment

## Outcome

Prove one deterministic local Layer pipeline in portable project state:
content and border, ordered rounded-rectangle masks, ordered local FX, transform,
opacity and Layer blend. Studio must submit atomic stack replacements while an
external Agent can author the same order directly in RFX3.

## Included

- stable `MaskId`/`EffectId`, enable state and deterministic authored order;
- intersect and inverted rounded-rectangle masks in local composition pixels;
- Gaussian Blur, Drop Shadow and Glow with checked sigma/color/offset domains;
- atomic `SetLayerMasksCommand` and `SetLayerEffectsCommand` with CAS,
  idempotency and Last-Known-Good preservation;
- Inspector add/edit/enable/invert/remove controls;
- one Skia saveLayer boundary so mask applies to content before FX and border/
  content receive the stack once;
- RFX3 grammar, project-local Agent Skill and round-trip tests.

## Excluded

Bezier/path masks, feather/expansion/keyframes, arbitrary effect graphs,
adjustment Layers, Group masks/FX/isolation, Backdrop Glass, Motion Blur,
procedural textures, Precomposition and platform qualification beyond macOS.

## Kill criteria

Reject the experiment if mask/FX order exists only in QML or Metal, the same
effect is applied independently to Shape subparts, invalid parameters publish,
disabled nodes disappear from project truth, or Windows must receive different
semantic project state.

## Verification

Use the same commands as EXP-004. Core tests cover validation and round-trip;
Studio tests cover accepted/rejected stack edits; macOS Visual tests compile and
execute the Metal/Skia path. Visual judgment remains owner-only.

## Exact handoff

Select a Shape or Text Layer, add and edit a rounded mask, then add Blur, Shadow
and Glow in order. Verify enable/invert/remove behavior, playback continuity,
Revision increments and RFX3 persistence. Owner acceptance is required.

## Owner evaluation result

The 2026-08-08 Reels review confirmed that Drop Shadow and the bounded static
FX stack render as owner-local effects. Revision is required because a Glow
request produced duplicate Text Layers to approximate unsupported animated
effect properties. EXP-006 adds topology-preserving effect intents, capability
diagnostics and Agent guardrails. General animation of Glow color/intensity/
sigma remains G3 scope and must fail closed until implemented.
