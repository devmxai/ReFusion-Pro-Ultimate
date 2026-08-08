# Project.rfx language v1

The canonical order is strict. Unknown declarations are errors.

```rfx
rfx 1;

project id("stable_project_id") revision(2)
  name("Project name");

composition id("stable_composition_id") name("Main") {
  canvas px(1080, 1920);
  frame_rate rational(30, 1);
  duration frames(900);

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
}
```

Text content replaces `content shape` with:

```rfx
content text {
  value("UTF-8 text");
  font_family("Arial");
  font_size px(72);
  layout_width px(800);
  direction(ltr);
  fill rgba8("#FFFFFFFF");
}
```

Allowed animation property paths are exactly:

- `transform.position.x`
- `transform.position.y`
- `transform.scale.x`
- `transform.scale.y`
- `transform.rotation`
- `transform.opacity`

Strings support `\"`, `\\`, `\n`, `\r` and `\t`. Colors use exactly
`#RRGGBBAA`. Numbers are decimal without exponent notation.
