# RFX5 canonical contribution declarations

New projects and canonical saves use RFX5. RFX1–RFX4 remain readable migration
inputs. The header binds both generated registries:

```rfx
rfx 5;
registry digest("rfx-vp-fnv1a64:<engine-generated>");
contributions digest("rfx-vc-fnv1a64:<engine-generated>");
```

Mask and FX bodies use one descriptor-driven parameter grammar. Parameter IDs,
types, units, ranges and order come only from
`references/visual-contributions.md`:

```rfx
mask rounded_rect id("mask_card") enabled(true) inverted(false) {
  parameter position_x number(0);
  parameter position_y number(0);
  parameter width number(720);
  parameter height number(240);
  parameter corner_radius number(48);
}

effect glow id("fx_glow") enabled(true) {
  parameter sigma number(18);
  parameter color color_rgba8("#7C5CFFC0");
}
```

Do not invent a platform-specific effect declaration or legacy parameter
spelling. Unknown descriptors, missing/reordered parameters, type mismatches or
digest mismatches fail before the project Revision is accepted.
