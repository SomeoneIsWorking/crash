---
id: C024
kind: claim
status: holds
created: 2026-08-27
tags: crash1,crash2,crash3,frame-loop,vsync,ownership
depends: titles/crash1/executable.json, titles/crash2/executable.json, titles/crash3/executable.json, game/core/native_frame_loop_contract.cpp
---

## Claim

Each Crash trilogy runtime owns its retail VSync address and a non-null host frame driver; every
guest VSync mode and every attempt to replace the framework fatal trap aborts before a retail wait
can execute

## Evidence

The three cached verified executables match distinct complete VSync bodies at Crash 1
`[0x8003E4F0,0x8003E638)`, Crash 2 `[0x8004A484,0x8004A5CC)`, and Crash 3
`[0x8004B2A8,0x8004B3F0)`. Each executable gate passes 6/6 with altered-entry and altered-body
opposite-answer controls; each compiled runtime agrees 20/20 with its executable and manifest. A
Clang build passes all three runtime contracts plus the native-frame death gate: for each title an
unproven frame step, VSync(-1), VSync(0), and trap-replacement attempt terminate with SIGABRT. The
Crash 1 product rebuild includes the same initialization seam before generated dispatch. Crash 1's
driver records only a finite boot seam; Crash 2 and Crash 3 record missing products, so none claims a
runnable frame.

## What would falsify it

Any title's manifest no longer fingerprints the runtime-declared VSync body, a runtime returns a null
frame driver, any guest VSync mode returns successfully, a registrar can replace the fatal handler,
Crash 1 dispatches generated code before initializing and requiring the contract, or any refusing
driver advances a frame without a newly measured title-specific loop boundary and service order.
