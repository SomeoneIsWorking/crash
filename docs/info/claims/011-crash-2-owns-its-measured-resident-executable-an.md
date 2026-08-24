---
id: C011
kind: claim
status: holds
created: 2026-08-22
tags: crash2,runtime,inheritance,crt0
depends: CMakeLists.txt, psxport.pin, titles/crash2/core/crash2_runtime.cpp, titles/crash2/executable.json, tools/verify_runtime_image.py
reconfirmed: 2026-08-24
verified_at: 2026-08-24 20:14:18
---

## Claim

Crash 2 owns its measured resident executable and crt0 facts through a directly inherited GuestProgramImage runtime with no GameConfig or GameHooks compatibility view

## Evidence

Clean Clang build against recorded psxport ad5cf802 passed crash2_runtime_inheritance; verify_runtime_image compared the production Crash2Runtime value to the real SCUS_941.54 plus shipping crt0_extract and matched 15/15, while an altered global pointer produced one named disagreement. Normal CTest passed 4/4 and psxport_sync --check matched ad5cf802.

## What would falsify it

Crash2Runtime exposes a legacy compatibility view, invents a runtime product, any of the 15 real-image facts disagrees or disappears, the altered-fact control stops disagreeing, or the framework pin/interface changes without re-verification.

## Re-confirmed 2026-08-22

Fresh Clang 22 CTest passed Crash2Runtime inheritance; real SCUS_941.54 retained 15/15 typed runtime agreement and the altered global pointer produced exactly one named disagreement against recorded psxport ad5cf802.

## Re-confirmed 2026-08-22

Post-commit default-tree Crash2Runtime passed its direct inheritance test and all 15 runtime image facts, including the altered-fact disagreement control.

## Re-confirmed 2026-08-22

Post-change authoritative verification: python3 tools/verify.py passed Clang build and CTest 5/5; cached serial-scoped executable/oracle/runtime gates passed as documented on 2026-08-22.

## Re-confirmed 2026-08-24

Against recorded psxport d2266f4b, the authoritative Clang build and CTest passed 5/5; Crash2Runtime direct inheritance passed, and the real SCUS_941.54 boundary gate independently confirmed tracked game main 0x80049BD4 at oracle call three while rechecking executable identity.

## Re-confirmed 2026-08-24

At recorded psxport d2266f4b, authoritative Clang CTest passed Crash2Runtime direct inheritance; production runtime facts matched cached real SCUS_941.54 at 15/15 including independently observed game main 0x80049BD4, and the altered-global-pointer control produced one named disagreement.

## Re-confirmed 2026-08-24

At fetchable recorded psxport bc8c8897, CTest passed Crash2Runtime direct inheritance and explicit guestVramIsPicture=false; production runtime facts matched cached real SCUS_941.54 at 15/15 with the altered-fact control, and its boundary gate passed SELFTEST 12/12.

## Re-confirmed 2026-08-24

After aca3ecb, Crash 2 runtime CTest passed and real SCUS_941.54 reached tracked game main 0x80049BD4 with 34/34 state agreement.
