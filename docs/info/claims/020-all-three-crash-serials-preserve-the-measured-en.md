---
id: C020
kind: claim
status: holds
created: 2026-08-25
tags: crash,syscall,oracle
depends: tools/resident_recomp.py#check_enter_critical_boundary, game/core/resident_program.cpp#runResidentProgram, game/core/enter_critical_frontier.cpp#runEnterCriticalFrontier, tests/recomp_boundary_support.h#TitleBoundaryRunner, psxport.pin
---

## Claim

All three Crash serials preserve the measured EnterCriticalSection syscall exception across shipping HLE and agree 34/34 with the independent CPU at the first post-syscall B0 dispatch boundary

## Evidence

On clean Clang build scratch/build-clang-8611d756 against recorded framework 8611d756/emitter 2026-08-24.2, crash1, crash2, and crash3 each passed SELFTEST 16/16. Cause=0x20 and title-specific EPC agreed exactly, one Status mode-stack pop resumed at EPC+4, and the next state agreed 34/34 at PC 0x000000B0; SCUS-94900 carried t1=0x56 and ra=0x800431B8. Wrong selector, wrong syscall target, 7/8 trace, repeated target, altered register, and out-of-text seed all produced the opposite answer.

## What would falsify it

Any title Cause/EPC/resume value or post-return CPU state differs; the B0 target or SCUS-94900 t1=0x56/ra=0x800431B8 changes; any refusal control accepts; generated code is edited; or the framework/emitter/runtime changes without re-verification.
