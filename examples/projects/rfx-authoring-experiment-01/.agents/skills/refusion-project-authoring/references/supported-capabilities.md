# Supported capabilities — EXP-006F

Supported: Shape/Text Layers, exact ranges and rational time, stable IDs,
explicit root order, pass-through LayerGroups, parent/local Transform domains,
solid/linear/radial paint, border, ordered rounded masks, static Gaussian Blur,
Drop Shadow and Glow, centered TextBox/paragraph fields, qualified Font
identity, Transform keyframes and measured geometry/logical/ink alignment.

Agent read operations: JSON outline, inspect, measure, capabilities, validate,
semantic lint and diff. Typed commits: GroupNodes, AddEffect(Glow) and
AlignNodes. Rejection retains Last-Known-Good.

Unsupported: Video/Audio/Image/SVG import in this authoring language slice,
Glass/backdrop, Motion Blur, arbitrary FX/shaders, animated effect properties,
Group isolation/FX/masks, nested Composition/Precomposition, plugins or
arbitrary C++ project code. Report the returned capability code and do not
approximate unavailable work.
