---
id: C007
kind: claim
status: holds
created: 2026-08-22
tags:
depends: CMakeLists.txt, psxport.pin, titles/crash1/executable.json, titles/crash1/recomp_seeds.json, tools/crash1_recomp.py, tests/crash1_recomp_boundary.cpp
---

## Claim

Crash 1's clean pinned generated path agrees with the independent reference CPU at all eight resident calls before the BIOS exception boundary.

## Evidence

A clean Clang build against recorded psxport 3418a79b ran CMake crash1_recomp_boundary_check: calls 1-8 at steps 57910, 57931, 57935, 57962, 62065, 62073, 62077, and 62081 each agreed on 34/34 CPU-state fields. The out-of-text seed, real 7/8 short trace, repeated-target ambiguity, and altered-register controls produced their required opposite answers; SELFTEST 9/9. The oracle then leaves mapped text at 0xBFC00180 at step 62083.

## What would falsify it

Any target or state field disagrees/disappears; the clean runner no longer owns a Game; a negative control passes; executable bytes, emitter, oracle capture, runner, comparator, or framework pin changes without re-verification.
