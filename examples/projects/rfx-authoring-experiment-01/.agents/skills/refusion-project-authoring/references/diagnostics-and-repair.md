# Diagnostics and repair

CLI diagnostics use:

```text
/path/Project.rfx:line:column: error CODE: message
```

- `RFX-RFX-LEX-*`: invalid character, number or string escape. Repair the exact
  token at the reported location.
- `RFX-RFX-PARSE-*`: declaration or grammar order is wrong. Compare only the
  surrounding declaration with `language-v1.md`.
- `RFX-RFX-TIME-*`: frame arithmetic cannot be represented. Keep frame values
  within the composition and use the project rate.
- `RFX-PROJECT-*`: compiled semantics are invalid, such as duplicate IDs,
  invalid transforms or ranges beyond the composition.
- `RFX-REV-409`: the candidate was based on an old active revision. Read the
  current file/snapshot and reapply the intent to current revision plus one.
- `RFX-REV-NEXT-409`: candidate revision was not exactly active plus one.
- `RFX-PROJECT-ID-409`: project identity changed. Restore the stable project ID.

Studio writes one JSON object per event to
`.refusion/Diagnostics/session.jsonl`. A rejected candidate does not replace the
active snapshot. Do not increment again merely because validation failed; base
the next candidate on the active revision reported by Studio.
