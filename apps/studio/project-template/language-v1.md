# Project.rfx language v1

Order is strict and unknown declarations fail closed.

```rfx
layer shape id("stable_layer_id") name("Card") {
  range frames(0, 900);
  transform {
    position canvas_px(540, 960);
    scale ratio(1, 1);
    rotation degrees(0);
    opacity ratio(1);
  }
  content shape {
    size px(800, 500);
    corner_radius px(40);
    fill rgba8("#7C5CFFFF");
  }
  animate transform.position.x {
    key frame(0) value(240);
    key frame(450) value(840);
    key frame(900) value(240);
  }
}
```

Text content contains, in order: `value`, `font_family`, `font_size px`,
`layout_width px`, `direction(ltr|rtl)` and `fill rgba8("#RRGGBBAA")`.

Animation paths: `transform.position.x`, `transform.position.y`,
`transform.scale.x`, `transform.scale.y`, `transform.rotation`, and
`transform.opacity`.
