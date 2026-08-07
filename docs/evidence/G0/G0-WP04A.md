---
id: EV-G0-WP04A
kind: evidence-record
gate: G0
work_package: G0-WP04A
status: local-passed
source_commit: 24d5946e442de09ac9ccc798f9e7aeedeee04502
date: 2026-08-07
---

# Authority and boundary correction evidence

## Delivered

- The concrete Application Host is private to `src/application` and is the only
  service owning `core::ProjectAuthority`.
- Qt StudioBridge and CLI depend on `ProjectCommandService`; neither constructs
  or links directly to the concrete Core authority.
- Successful receipts now carry a non-blocking empty diagnostic; every rejection
  path explicitly carries a blocking diagnostic.
- A negative Repo OS test injects forbidden Studio/CLI authority ownership and
  proves the architecture gate rejects it.
- StudioBridge has an integration test for accepted and rejected commands,
  revision/snapshot updates and diagnostic notifications.

## Verification

- `macos-core`: 3/3 tests passed.
- `macos-studio`: 4/4 tests passed.
- `macos-core-sanitized`: 3/3 tests passed under ASan+UBSan.
- Clean combined Studio+Metal+Skia build: 6/6 tests passed.
- Studio launch smoke remained alive without stderr until deliberately stopped.
- Architecture Check: 17 owned files, zero problems after the final bridge test
  source and policy rules were committed.

## Failure behavior

Direct `ReFusion::Core` links or `ProjectAuthority` ownership in Studio/CLI fail
the policy test. Invalid commands preserve Last-Known-Good and surface the same
typed diagnostic through the Application service.

## Deliberately unclaimed

This is a process-local G0 command boundary. Persistence, file reconciliation,
MCP, multi-process coordination and full project schemas remain G2 work.
