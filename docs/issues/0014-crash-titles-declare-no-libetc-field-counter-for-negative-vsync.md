---
id: 14
title: Crash titles declare no libetc field counter, so a negative VSync query aborts
status: open
symptom: crash_native_frame_contract aborts with "VSync negative query at 0x8003E4F0 has no measured libetc field counter" when built against psxport 51df140f or later
state_items: S011
tags: crash1,crash2,crash3,vsync,platform-hle,framework-pin
created: 2026-09-19
updated: 2026-09-19
---

## Root cause

psxport `51df140f` ("Serve measured libetc VSync queries without advancing fields", 2026-09-12) gave
`PlatformHlePlan` a query contract: a guest `VSync` call with a NEGATIVE argument is a request for
the elapsed-field count, not a frame wait, so the framework answers it from the title's declared
libetc field counter and does not advance a field. A title that never declared that counter cannot
be answered truthfully, so the framework aborts by name rather than returning a fabricated zero or a
host-side count. That refusal is correct; the missing fact is Crash's.

No Crash title declares `vsyncQueryCounterAddress`. Crash 1's canonical libetc VSync is
`[0x8003E4F0,0x8003E638)` (claim 024, re-frontier), Crash 2's is `[0x8004A484,0x8004A5CC)`, Crash 3's
is at `0x8004B2A8`. The counter global each body loads for a negative argument has not been measured.

## How it surfaces

`crash_native_frame_contract` (test 14) aborts. Every other test passes: measured 2026-09-19 with
Clang, 20 of 21 CTest cases green including `crash_cpp_policy` and `crash1_execution_boundary`.

Neither `game/core/native_frame_loop_contract.cpp` nor `tests/native_frame_loop_contract.cpp` is
involved in any current change; this is pure framework drift. `psxport.pin` records `9c7dd098`, which
predates `51df140f`, so a fresh clone at the recorded pin does not hit it — only a build against the
live framework tree does, which is what the verifier uses.

## Next step

Measure the counter. For each title, disassemble the libetc VSync body and find the global it loads
on the negative-argument path — the same shape C-12 recorded as `kVSyncQueryCounterAddress`
(`0x800EEB98`, zeroed by VSync setup and incremented by the callback dispatcher) and Spider-Man
recorded as `0x800B397C`. Declare it in each title's platform facts, then re-run this test against
framework HEAD and bump `psxport.pin`.

Do not work around this by removing the abort, declaring a guessed address, or pinning Crash to a
pre-`51df140f` framework to keep the gate green. The abort is the framework telling the truth about
an undeclared fact.
