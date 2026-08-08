# ReFusion Agent command catalog

Generated from Visual Property Registry
`rfx-vp-fnv1a64:f6f8d1b88fe2d101` and the Core capability table.

## Digital eye

- `refusion-cli outline <Project.rfx>`
- `refusion-cli inspect <Project.rfx> <layer:id|group:id>`
- `refusion-cli measure <Project.rfx> <time-ns> --json`
- `refusion-cli capabilities`
- `refusion-cli validate <Project.rfx> --json`
- `refusion-cli lint <Project.rfx>`
- `refusion-cli diff <before.rfx> <after.rfx>`

## Typed commits

- `commit group <Project.rfx> <group-id> <name> <node-ref>...`
- `commit add-glow <Project.rfx> <layer-id> <effect-id> <sigma> <#RRGGBBAA>`
- `commit align <Project.rfx> <subject-ref> <target-ref> <time-ns> <horizontal> <vertical> <geometry|logical|ink>`

Typed commits apply revision CAS, Core validation and one atomic canonical file
replacement. A running Studio then revalidates the revision through Application
authority.
