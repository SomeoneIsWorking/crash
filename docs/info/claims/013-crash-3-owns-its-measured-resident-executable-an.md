---
id: C013
kind: claim
status: holds
created: 2026-08-22
tags: crash3,runtime,inheritance,crt0
depends: CMakeLists.txt, psxport.pin, titles/crash3/core/crash3_runtime.cpp, titles/crash3/executable.json, tools/verify_runtime_image.py
reconfirmed: 2026-08-22
verified_at: 2026-08-22 19:23:54
---

## Claim

Crash 3 owns its measured resident executable and crt0 facts through a directly inherited GuestProgramImage runtime with no GameConfig or GameHooks compatibility view

## Evidence

At recorded psxport `57a17a14`, the authoritative Clang build passed
`crash3_runtime_inheritance`. `verify_runtime_image.py` compared production `Crash3Runtime` to real
`SCUS_942.44` plus shipping `crt0_extract` and matched 15/15, including game main `0x80048AA0`;
an altered global pointer produced one named disagreement. Normal CTest passed 5/5, including the
10-case serial-aware provision contract.

## What would falsify it

Crash3Runtime exposes a legacy compatibility view, invents a runtime product, any of the 15 real-image facts disagrees or disappears, the altered-fact control stops disagreeing, or the framework pin/interface changes without re-verification.

## Re-confirmed 2026-08-22

At recorded psxport 57a17a14, Crash3Runtime inheritance passed; production runtime facts matched real SCUS_942.44 15/15 including game main 0x80048AA0, and the altered global-pointer control produced one named disagreement. Authoritative Clang CTest passed 5/5.
