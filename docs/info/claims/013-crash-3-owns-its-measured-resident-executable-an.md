---
id: C013
kind: claim
status: holds
created: 2026-08-22
tags: crash3,runtime,inheritance,crt0
depends: CMakeLists.txt, psxport.pin, titles/crash3/core/crash3_runtime.cpp, titles/crash3/executable.json, tools/verify_runtime_image.py
---

## Claim

Crash 3 owns its measured resident executable and crt0 facts through a directly inherited GuestProgramImage runtime with no GameConfig or GameHooks compatibility view

## Evidence

A clean Clang 22 build against recorded psxport ad5cf802 passed crash3_runtime_inheritance; verify_runtime_image compared production Crash3Runtime to real SCUS_942.44 plus shipping crt0_extract and matched 15/15, while an altered global pointer produced one named disagreement. Normal CTest passed 5/5, including the 10-case serial-aware provision contract.

## What would falsify it

Crash3Runtime exposes a legacy compatibility view, invents a runtime product, any of the 15 real-image facts disagrees or disappears, the altered-fact control stops disagreeing, or the framework pin/interface changes without re-verification.
