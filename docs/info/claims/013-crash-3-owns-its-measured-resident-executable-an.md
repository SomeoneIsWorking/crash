---
id: C013
kind: claim
status: holds
created: 2026-08-22
tags: crash3,runtime,inheritance,crt0
depends: CMakeLists.txt, psxport.pin, titles/crash3/core/crash3_runtime.cpp, titles/crash3/executable.json, tools/verify_runtime_image.py
reconfirmed: 2026-08-24
verified_at: 2026-08-24 20:05:17
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

## Re-confirmed 2026-08-22

Post-change authoritative verification: python3 tools/verify.py passed Clang build and CTest 5/5; cached serial-scoped executable/oracle/runtime gates passed as documented on 2026-08-22.

## Re-confirmed 2026-08-24

At recorded psxport d2266f4b, authoritative Clang CTest passed Crash3Runtime direct inheritance; production runtime facts matched cached real SCUS_942.44 at 15/15 including game main 0x80048AA0, and the altered-global-pointer control produced one named disagreement.

## Re-confirmed 2026-08-24

At fetchable recorded psxport bc8c8897, CTest passed Crash3Runtime direct inheritance and explicit guestVramIsPicture=false; production runtime facts matched cached real SCUS_942.44 at 15/15 with the altered-fact control, and its boundary gate passed SELFTEST 12/12.
