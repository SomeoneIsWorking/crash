---
id: C013
kind: claim
status: holds
created: 2026-08-22
tags: crash3,runtime,inheritance,crt0
depends: CMakeLists.txt
reconfirmed: 2026-08-24
verified_at: 2026-08-24 23:24:05
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

## Re-confirmed 2026-08-24

After aca3ecb, Crash 3 runtime CTest passed and its real-data boundary regenerated 986 candidates with eight 34/34 comparisons.

## Re-confirmed 2026-08-24

Clean recorded psxport 9c2e3f1c authoritative CTest passed Crash3Runtime inheritance; cached real SCUS_942.44 matched all 15 runtime facts with one altered-fact disagreement, and its serial boundary gate passed 13/13.

## Re-confirmed 2026-08-24

At bb3a497, crash3_runtime_inheritance passed in authoritative CTest 6/6, with 15/15 typed executable facts and the clean framework pin unchanged.
