# Project.rfx language v3 — visual styling experiment

RFX3 preserves RFX2 hierarchy and adds portable Shape paint, border, Layer
blend, ordered rounded-rectangle masks and ordered local FX. RFX1/RFX2 remain
readable; canonical saves and newly created projects emit `rfx 3`.

Layer order is strict: range, transform, optional blend, content, masks, FX,
then animations. IDs for Layers, Groups, masks and FX must remain stable and
unique. Unknown declarations fail closed.

```rfx
layer shape id("lyr_card") name("Card") {
  range frames(0, 1800);
  transform {
    position canvas_px(540, 960);
    anchor canvas_px(0, 0);
    scale ratio(1, 1);
    rotation degrees(0);
    opacity ratio(1);
  }
  blend(normal);
  content shape {
    size px(720, 420);
    corner_radius px(56);
    fill linear_gradient {
      start local_px(-360, -210);
      end local_px(360, 210);
      stop ratio(0) color rgba8("#7C5CFFFF");
      stop ratio(1) color rgba8("#20D0FFFF");
    };
    stroke width px(2) color rgba8("#FFFFFF70");
  }
  mask rounded_rect id("mask_card") enabled(true) inverted(false) {
    position local_px(0, 0);
    size px(680, 380);
    corner_radius px(44);
  }
  effect drop_shadow id("fx_shadow") enabled(true) {
    offset px(0, 20);
    sigma px(16, 16);
    color rgba8("#00000070");
  }
  effect glow id("fx_glow") enabled(true) {
    sigma px(18);
    color rgba8("#7C5CFFC0");
  }
}
```

Solid fills use `fill rgba8("#RRGGBBAA");`. Radial fills use
`fill radial_gradient { center local_px(x,y); radius px(r); ... };`.
Gradient stop ratios are strictly increasing within 0..1 and each gradient has
2..32 stops. Supported blend modes are `normal`, `multiply`, `screen` and
`overlay`. FX execute in authored order after masks and before Layer transform/
blend. Sigma values are composition pixels in 0..256.

This experiment does not admit Glass/backdrop sampling, procedural paper/noise,
textures, motion blur, Group FX/isolation or nested Composition instances.
Never approximate those requests with Gaussian blur or hidden platform state.
