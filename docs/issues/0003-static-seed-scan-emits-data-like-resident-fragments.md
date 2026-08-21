---
id: 3
title: static seed scan emits data-like resident fragments
status: open
symptom: Crash 1 emission includes candidate functions whose generated bodies decode embedded text/data as instructions
tags: crash1,recompiler,discovery
created: 2026-08-21
updated: 2026-08-21
---

## Observation

The real USA executable emission reports 115 binary-rooted seeds and 653 resident functions. Some
generated candidates, for example `0x80011168`, contain obvious embedded strings and unhandled opcode
comments rather than coherent code. These files are gitignored evidence and are not hand-edited.

## Impact

The 653-function denominator proves what the current emitter produced, not that all 653 candidates
are executable functions. It does not invalidate the measured entry-to-first-call slice: that
generated path independently agrees with the oracle on 34/34 fields. It does prevent treating the
whole substrate or static discovery count as verified.

## Proper next investigation

Continue boundary comparison from `0x80011A18` and record the first real divergence or fail-fast
miss. Separately distinguish executable pointer/table roots from false-positive data candidates in
the shared emitter before claiming complete resident discovery. This repository must not patch or
delete generated fragments locally.
