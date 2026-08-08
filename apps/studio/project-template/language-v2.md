# Project.rfx language v2 — hierarchy experiment

Order is strict and unknown declarations fail closed. RFX1 remains readable,
but canonical saves and newly created projects use `rfx 2`.

Every Layer and Group is declared once. `root` defines bottom-to-top root draw
order. Each non-root node appears exactly once in one Group's ordered `children`
block. Missing, duplicate, multiply parented, out-of-range or cyclic references
are rejected before the revision becomes active.

```rfx
rfx 2;

project id("stable_project_id") revision(2) name("Example");

composition id("stable_composition_id") name("Main") {
  canvas px(1080, 1920);
  frame_rate rational(60, 1);
  duration frames(1800);

  layer shape id("lyr_body") name("Button Body") {
    range frames(0, 1800);
    transform {
      position canvas_px(540, 960);
      anchor canvas_px(0, 0);
      scale ratio(1, 1);
      rotation degrees(0);
      opacity ratio(1);
    }
    content shape {
      size px(620, 180);
      corner_radius px(90);
      fill rgba8("#7C5CFFFF");
    }
  }

  layer text id("lyr_label") name("Label") {
    range frames(0, 1800);
    transform {
      position canvas_px(540, 980);
      anchor canvas_px(0, 0);
      scale ratio(1, 1);
      rotation degrees(0);
      opacity ratio(1);
    }
    content text {
      value("SUBSCRIBE");
      font_family("Arial");
      font_size px(64);
      layout_width px(520);
      direction(ltr);
      fill rgba8("#FFFFFFFF");
    }
  }

  group id("grp_subscribe") name("Subscribe Group") {
    range frames(0, 1800);
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
    animate transform.scale.x {
      key frame(0) value(1);
      key frame(900) value(1.08);
      key frame(1800) value(1);
    }
    animate transform.scale.y {
      key frame(0) value(1);
      key frame(900) value(1.08);
      key frame(1800) value(1);
    }
  }

  root {
    group("grp_subscribe");
  }
}
```

`position` and `anchor` use exact Composition pixels. Group transform is applied
once to the subtree; never duplicate parent keyframes onto children.

Groups in EXP-002 are pass-through. Their opacity must remain exactly `1` and
they cannot animate opacity. Group masks, FX and isolated opacity are explicitly
unsupported until their compositing contract is accepted.

Animation paths: `transform.position.x`, `transform.position.y`,
`transform.scale.x`, `transform.scale.y`, `transform.rotation`, and
`transform.opacity` for Layers. Groups support the same paths except opacity.
