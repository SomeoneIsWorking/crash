---
id: 14
title: Crash titles declare no libetc field counter, so a negative VSync query aborts
status: closed
symptom: crash_native_frame_contract aborts with "VSync negative query at 0x8003E4F0 has no measured libetc field counter" when built against psxport 51df140f or later
state_items: S011
tags: crash1,crash2,crash3,vsync,platform-hle,framework-pin
created: 2026-09-19
updated: 2026-09-19
closed: 2026-09-19
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

## Resolution (2026-09-19)

Measured, not guessed. All three libetc VSync bodies have the identical shape: the negative-argument
arm is a `bgez $a0` that falls through to a `lui`/`lw` pair loading one global, which the body then
returns. The same global is the counter the body reads, increments past its callback wait, and
latches into its last-VSync slot.

| title | VSync body | negative arm | field counter |
|---|---|---|---|
| Crash 1 (`SCUS_949.00`) | `[0x8003E4F0,0x8003E638)` | `0x8003E528` `bgez $a0` -> `0x8003E530` | **`0x800549F0`** |
| Crash 2 (`SCUS_941.54`) | `[0x8004A484,0x8004A5CC)` | `0x8004A4BC` `bgez $a0` -> `0x8004A4C4` | **`0x8005DC98`** |
| Crash 3 (`SCUS_942.44`) | `[0x8004B2A8,0x8004B3F0)` | `0x8004B2E0` `bgez $a0` -> `0x8004B2E8` | **`0x8005F384`** |

Each is recorded once, in that title's `executable.json` under `runtime.vsync.query_counter`, and
reaches `PlatformHlePlan` through `NativeFrameLoopContract::vsyncQueryCounter`. A `static_assert`
per title refuses a zero, so an undeclared counter is a build failure rather than a runtime abort.

`tests/native_frame_loop_contract.cpp` also encoded the PRE-`51df140f` contract: it asserted a
`FrameBoundary` typed exit for BOTH arms, so it could not tell a served query from a swallowed
frame. The two arms are now distinct — `VSync(-1)` returns the counter in `v0`, produces no typed
exit, and does not advance the counter; `VSync(0)` still exits at a frame boundary. Falsified in
both directions: expecting the wrong count fails all three titles.

crash 21/21, pin bumped `9c7dd098` -> `534ee67e`.
