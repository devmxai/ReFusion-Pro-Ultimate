# Coordinates and time

Canvas origin is top-left. X grows right and Y grows down. In RFX4/RFX5, `position`
uses the immediate parent's `parent_px`; a root node uses Composition pixels.
`anchor` uses the node's own `local_px`. A local transform is applied as
`T(position) * R(rotation) * S(scale) * T(-anchor)`. Scale and opacity are
dimensionless ratios. Parent transforms are evaluated by Core, not duplicated
onto child keyframes. TextBox geometry is centered at local `(0,0)` and remains
distinct from derived logical, ink, effect-expanded and world bounds.

Typed UI/property commands and measured alignment commit pixel coordinates on
the binary-exact `1/1024 px` authored grid. A calculated Agent pixel value must
use `round(value * 1024) / 1024` before writing a candidate. Explicit existing
RFX literals are preserved rather than silently normalized. Ratios, rotation
and opacity do not use the pixel grid.

`range frames(start, duration)` has an inclusive start and exclusive end at
`start + duration`. Animation keys use absolute Composition frames, not
layer-relative frames. Integer frames are authored truth; Core derives checked
rational ProjectTime nanoseconds.
