# ReFusion Project.rfx authoring experiment 01

This is the first real test of a typed, single-file project source. Opening this
folder means opening `Project.rfx`; the native Core compiler turns that source
into the same immutable snapshot used by Timeline, Inspector, Canvas and the
Skia GPU renderer.

The project is a 1080x1920, 30 fps, 900-frame composition: exactly 30 seconds.
For a layer, `range frames(start, duration)` records its absolute start and its
duration. Its exclusive end is `start + duration`. `describe` prints all three
values in frames and the derived nanosecond boundaries.

The RFX2 revision adds the bounded EXP-002 hierarchy proof: `grp_hero` is one
pass-through LayerGroup with six ordered children, one parent rotation animation
and an explicit root draw order. Studio shows the Group as one collapsed row;
double-clicking it drills down to the addressable child Layers. Core, not QML,
evaluates the parent transform for Skia.

This experiment currently proves Shape, Text, Transform2D anchor, scalar
transform animation and pass-through LayerGroup hierarchy. It does not pretend
that Video, Audio, masks, isolated group opacity or general FX already exist.
