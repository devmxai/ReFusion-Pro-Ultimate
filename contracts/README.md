# Contracts

This directory will hold canonical machine-readable project, command,
capability, property, diagnostic, extension, and MCP schemas. Generated C++,
Inspector metadata, SDK docs, and Agent catalogs must derive from these sources
or from the same Engine Registry; they are never independent hand-written truths.

FX/plugin contracts describe stable contribution IDs, versions, typed state,
migrations, capability/resource declarations and content digests. A project
never stores a native plugin binary path or platform-specific replacement.
Unknown contributions round-trip as unresolved and fail closed.

`architecture/cross-platform-visual-boundary-exceptions.json` is the
`XPF-WP00A` frozen debt receipt. Its baseline is digest-pinned by `rfdev.py` and
never grows; only `active_allowances` may be reduced or removed as XPF-WP02,
XPF-WP03 and XPF-WP06 clean the recorded boundaries.

`visual/cross-platform-capability-matrix.json` is the machine-readable
`XPF-WP06` claim ledger. Every Registry capability is listed for the macOS
Metal, Windows D3D12, iOS Metal canary and Android Vulkan canary profiles. Its
ordered states are independently truthful: source definition cannot imply a
compile, a compile cannot imply a physical run, and no profile is qualified
until semantic, calibrated visual and performance evidence all pass. The
architecture check rejects missing Registry entries or an evidence-state jump.

`project/refusion-project-rfx-exp1.ebnf` is the strict source grammar for the
bounded EXP-001 agent-authoring experiment. It is experimental evidence, not a
shipping-format commitment. The native portable compiler is the enforcement
authority; prose and examples must remain subordinate to it.

EXP-001A admits a valid empty Composition in the same grammar so a newly
created workspace can open with an empty Canvas and Timeline. This does not
admit empty or invalid IDs, zero duration, arbitrary rates, or UI-authored
dimensions; those remain Core-validated registry decisions.

`project/refusion-project-rfx-exp2.ebnf` is the bounded EXP-002 hierarchy
extension. It adds explicit root order, `LayerGroup`, ordered typed child
references and transform anchor. RFX1 remains readable and canonical writing
migrates to RFX2. Groups are pass-through in this proof: group opacity must stay
at 1.0 until isolated group compositing is separately admitted. This is evidence
for RFC-0002, not acceptance of that RFC or the shipping format.

`project/refusion-project-rfx-exp3.ebnf` extends the experiment with one
portable Shape paint value (solid/linear/radial), border, Layer blend mode,
ordered rounded-rectangle masks and ordered Gaussian Blur/Drop Shadow/Glow.
RFX1/RFX2 stay readable and canonical experimental writing migrates to RFX3.
Backdrop Glass, procedural textures, Motion Blur, Group isolation and nested
Compositions remain explicitly outside this grammar.

`project/refusion-project-rfx-exp4.ebnf` is the bounded EXP-006B schema. It
adds a Core Registry fingerprint, explicit `parent_px` position and `local_px`
anchor domains, centered local TextBox/padding, paragraph layout attributes and
qualified packaged Font identity. RFX1–RFX3 remain readable migration inputs;
canonical experimental writing migrates to RFX4. Derived glyph/logical/ink/
effect/world measurements are deliberately absent until TextLayoutPort exists.

`project/refusion-project-rfx-exp5.ebnf` is the bounded `XPF-WP06` contribution
schema. It binds the generated Mask/FX Registry digest and replaces the three
hand-written FX bodies plus one mask body with a single ordered typed parameter
grammar. RFX1–RFX4 stay readable migration inputs; canonical experimental
writing migrates to RFX5. A new contribution extends the Registry and common
lowering/execution path, never a platform-specific project syntax.
