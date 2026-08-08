# Project.rfx language v2

RFX1 is a readable migration input. This project's canonical source is RFX2.
Every Transform declares `position`, `anchor`, `scale`, `rotation`, then
`opacity`. Every Layer/Group ID is globally unique.

```rfx
group id("grp_cta") name("CTA Group") {
  range frames(0, 900);
  transform {
    position canvas_px(540, 960);
    anchor canvas_px(540, 960);
    scale ratio(1, 1);
    rotation degrees(0);
    opacity ratio(1);
  }
  children {
    layer("lyr_body");
    layer("lyr_label");
  }
  animate transform.rotation {
    key frame(0) value(-2);
    key frame(450) value(2);
    key frame(900) value(-2);
  }
}

root {
  layer("lyr_background");
  group("grp_cta");
}
```

`root` is bottom-to-top draw order. A non-root visual appears exactly once in
one Group's ordered `children`. Unknown, duplicate, multiply-parented, orphaned,
out-of-range and cyclic references are rejected.

EXP-002 Groups are pass-through. Group opacity must remain exactly `1` and may
not be animated. Do not invent Group mask/FX/isolation fields.
