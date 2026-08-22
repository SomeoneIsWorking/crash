---
id: 4
title: Clean Crash boundary runner segfaults before call one
status: resolved
symptom: A clean pinned Crash 1 boundary build exits 139 before the first captured call while the older cached runner passes
tags: crash1,harness,reproducibility,timing
created: 2026-08-22
updated: 2026-08-22
---

## Root cause

The focused runner instantiated a bare `Core`. Cleanly re-emitted generated code accounts guest
instructions through `rec_guest_instruction_ticks`, which dereferences `core->game->timing`; a bare
core has no owning machine and therefore crashed in `Timing::advanceGuestInstructionTicks`. The old
cached binary did not contain the clean emission and concealed the invalid harness construction.

## What was tried / dead ends

The older cached runner still reached call one, so its success could not establish clean-build
reproducibility. A clean build against the recorded framework pin produced the opposite answer and
GDB localized the null owner to instruction-tick accounting.

## Resolution

The runner now owns a `Game` and executes through that machine's wired `Core`. A clean pinned build
passed calls one through eight at 34/34 plus the out-of-text seed, short-window, repeated-target, and
altered-register controls (`SELFTEST 9/9`).
