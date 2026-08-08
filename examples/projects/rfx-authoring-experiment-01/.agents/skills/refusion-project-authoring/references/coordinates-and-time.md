# Coordinates, scale and timeline

## Canvas

The canvas origin is `(0, 0)` at top-left. X grows right and Y grows down.
`position canvas_px(x, y)` and `anchor canvas_px(x, y)` are true Composition
pixels. Core applies `T(position) * R(rotation) * S(scale) * T(-anchor)` and then
the parent matrix. Shape size and text layout width are also Composition pixels.
Scale is a positive ratio; rotation is clockwise degrees; opacity is 0 to 1.

For a 1080x1920 Reel, the canvas center is `(540, 960)`.

## Timeline

`frame_rate rational(numerator, denominator)` is the legal project rate.
`duration frames(count)` is the composition's exclusive end.

`range frames(start, duration)` means:

- inclusive start frame = `start`;
- duration in frames = `duration`;
- exclusive end frame = `start + duration`.

At 30/1 fps, `range frames(30, 840)` starts at 1 second and ends at frame
870 (29 seconds). Use integer frames as the authored truth. The Core compiler
derives project nanoseconds with checked rational conversion.

Animation `key frame(n)` uses an absolute composition frame and must be ordered,
unique, finite and inside the layer's inclusive start/exclusive-end boundary as
validated by the current Core contract. Prefer the last visible frame when a
terminal value must be visible before a layer disappears; frame equal to the
exclusive end is accepted as the mathematical boundary but is not displayed in
the half-open active range.
