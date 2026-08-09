# Supported capabilities — EXP-006F Agent digital-eye slice

Supported: empty Composition, Shape/Text layers, stable IDs, exact frame ranges,
parent-pixel position, local-pixel anchor/size, positive scale, rotation degrees, opacity,
RGBA8 solid/linear/radial Shape fill, ordered gradient stops, border,
normal/multiply/screen/overlay Layer blend, rounded-rectangle masks, ordered
Gaussian Blur/Drop Shadow/Glow, centered local TextBox with explicit padding,
paragraph direction/alignment/wrap/overflow/line-height/letter-spacing,
explicitly unqualified system Font convenience identity and qualified
project-relative packaged Font assets using the pinned Noto Sans Latin/Arabic
baseline, byte-backed FreeType shaping and ICU line breaks,
scalar keyframes for position X/Y,
scale X/Y, rotation and opacity, explicit root draw order, pass-through
LayerGroups, ordered children, one-parent validation and Timeline drill-down.
The qualified Desktop product build also exposes typed RFX6 Video import and
exact Asset relink through `media_commands` in the CLI capability response.
Import creates one content-addressed original plus linked, independently
addressable Video and Audio Clips; exact relink accepts byte-identical content
only and creates no semantic Revision.

Topology-safe Core intents are available for grouping sibling nodes,
reparenting nodes, adding one owner-local effect and one-shot measured node
alignment. Alignment declares geometry/logical/ink bounds, exact Composition
time and horizontal/vertical relation; logical/ink Text alignment requires the
engine measurement port and every accepted result is rechecked within 0.25 px.
A static effect or alignment request must preserve Layer/Group/root counts.

Unsupported: decoded Video Canvas playback, Audio output/waveform editing,
Video/Audio Inspector editing, Image/SVG layers, Glass/backdrop, procedural
textures/noise/paper, Motion Blur, color adjustment, arbitrary FX/shaders,
plugins, nested compositions, group masks, group FX, group opacity/isolation
and arbitrary C++ project code. Effect-property animation remains unsupported.
The external Agent exposes typed GroupNodes, AddEffect(Glow) and AlignNodes
commits plus JSON outline/inspect/measure/capabilities/validate/lint/diff.
Other supported RFX5 declarations still use a separate candidate followed by
validation and semantic diff. Never guess Text anchor offsets or add placeholder
Layers when a typed capability rejects the request.
