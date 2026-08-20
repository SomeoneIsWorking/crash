---
id: 1
title: Crash scaffold has no local registry command entrypoints
status: open
symptom: python3 tools/info.py, tools/catalog.py, and tools/re_frontier.py all fail because the files do not exist; the original numbered frontier parsed as zero entries.
tags: workflow,registries,scaffold
created: 2026-08-20
updated: 2026-08-20
---

## Root cause

The initial Crash scaffold predates the required project-information and issue-catalog entrypoints,
and wrote `docs/re-frontier.md` as numbered prose rather than the shared tracker schema.

## Current state

The frontier data is now in the shared tracker schema and passes the global engine over 9 entries.
Claims, instruments, and this issue exist in-repo. Local `tools/info.py`, `tools/catalog.py`, and
`tools/re_frontier.py` entrypoints still do not exist.

## Proper fix

Hoist the remaining shared information/catalog engines into psxport and expose one non-duplicated
consumer entry mechanism. Do not copy another game’s scripts: the global no-duplication rule and their
documented drift failures rule that out.
