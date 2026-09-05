---
id: C024
kind: claim
status: holds
created: 2026-08-27
tags: crash1,crash2,crash3,frame-loop,vsync,ownership
depends: titles/crash1/executable.json, titles/crash2/executable.json, titles/crash3/executable.json, game/core/native_frame_loop_contract.cpp
reconfirmed: 2026-09-04
verified_at: 2026-09-04
---

## Claim

Each Crash trilogy runtime owns its retail VSync address and a non-null host frame driver. The
framework converts guest VSync into a typed `FrameBoundary`; Crash 1 resumes from the measured guest
continuation, while Crash 2 and Crash 3 refuse unproven frame steps.

## Evidence

The three verified executables record distinct complete VSync bodies at Crash 1
`[0x8003E4F0,0x8003E638)`, Crash 2 `[0x8004A484,0x8004A5CC)`, and Crash 3
`[0x8004B2A8,0x8004B3F0)`. Against exact pinned PSXPort/Lightrec revisions, the production-boundary
tests initialize each platform owner and observe `FrameBoundary` for both queried modes. Crash 1's
test also proves its two GpuUpdate JAL sites remain guest instructions and the host service preserves
their return address; the frame-transition override independently produces the same typed exit.

## What would falsify it

Any title's manifest no longer fingerprints its runtime-declared VSync body, a runtime returns a
null frame driver, VSync executes its retail wait instead of returning a typed boundary, Crash 1
installs a native-function override on either JAL call site or loses its continuation, or a refusing
driver advances an unmeasured frame.
