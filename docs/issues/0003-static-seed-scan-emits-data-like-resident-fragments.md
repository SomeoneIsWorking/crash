---
id: 3
title: static seed scan emits data-like resident fragments
status: open
symptom: Crash 1 emission includes candidate functions whose generated bodies decode embedded text/data as instructions
tags: crash1,recompiler,discovery
state_items: S003
created: 2026-08-21
updated: 2026-08-26
---

## Observation

The real USA executable emission under psxport `54af32cb` reports 115 static candidate seeds and 666
emitted candidates. Some
generated candidates, for example `0x80011168`, contain obvious embedded strings and unhandled opcode
comments rather than coherent code. These files are gitignored evidence and are not hand-edited.

The specific `0x80011168` false positive comes from `code_pointer_tables`: the 18-word run at
`0x80054A24` mostly contains code addresses, but its first word is the data address `0x80011168`.
That data happens to decode as a non-UNKNOWN instruction, so the current run-level test promotes it
along with the real entries. This is a measured mixed-table failure, not a reason to delete one
generated fragment locally.

## Impact

The 666-candidate denominator proves what the current emitter produced, not that all 666 candidates
are executable functions. It does not invalidate the execution-proven entry-through-eighth-call
slice: the generated path independently agrees with the oracle 34/34 at each boundary. It does
prevent treating the whole substrate or static discovery count as verified. Current output now names
115 static seeds and 666 emitted candidates separately from the executable denominator: nine
candidate addresses have execution provenance—eight emitted bodies execute, and one further candidate
is observed as a call target without executing its body.

## Proper next investigation

The independent CPU and shipping path now agree through the first pre-HLE B(56h) dispatch; that does
not establish provenance for the rest of the static candidate set. Make the shared emitter distinguish
each mixed-table entry's executable provenance instead of accepting every target because the run as a
whole looks like code. This repository must not patch or delete generated fragments locally, and
explicit title seeds remain empty until an actual indirect execution miss proves an address.
