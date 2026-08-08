# Diagnostics and repair

Compiler diagnostics contain source line, column, stable code and message.
`RFX-RFX-*` means lexical/grammar/unit failure. `RFX-PROJECT-*` means compiled
semantics failed. `RFX-REV-409` is a stale base; re-read the active project.
`RFX-REV-NEXT-409` requires exactly active revision plus one.
`RFX-CAP-FX-ANIMATION-001` means effect-property animation is unavailable;
remove the unsupported intent instead of duplicating Layers.
`RFX-MEASURE-PORT-001` means the selected logical/ink alignment basis has no
admitted Text layout port. `RFX-MEASURE-NODE-INACTIVE-001` means a target is not
active at the exact requested time. `RFX-MEASURE-POSTCONDITION-001` means the
candidate did not meet the 0.25 px alignment postcondition. In all cases, do
not guess glyph offsets with Text anchor values.

Rejected candidates never replace Last-Known-Good. Read the latest JSONL event,
repair only the rejected intent, validate a separate candidate, then publish it.
