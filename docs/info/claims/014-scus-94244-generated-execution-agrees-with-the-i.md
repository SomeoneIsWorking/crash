---
id: C014
kind: claim
status: holds
created: 2026-08-22
tags: crash3,recomp,oracle,syscall
depends: tools/resident_recomp.py, tests/crash3_recomp_boundary.cpp, titles/crash3/core/crash3_runtime.cpp, psxport.pin
reconfirmed: 2026-08-24
verified_at: 2026-08-24 23:24:07
---

## Claim

SCUS-94244 generated execution agrees with the independent CPU oracle through game main and the first syscall boundary

## Evidence

Real SCUS_942.44 emitted 986 candidates from 297 seeds at psxport 57a17a14/emitter 2026-08-22.1. Eight canonical calls agreed 34/34 each; call 3 reached tracked game main 0x80048AA0, call 8 reached 0x80048C38, and the oracle entered 0xBFC00180 two instructions later at step 75963. The generated wrapper selected EnterCriticalSection and shipping HLE changed IRQ 1->0. The 12/12 suite proved missing-call, wrong-target, altered-state, repeated-target, and out-of-text-seed opposite answers.

## What would falsify it

Any real SCUS_942.44 call target or register differs, game main is not call 3 at 0x80048AA0, the syscall words/selector or IRQ transition differs, a negative control stops refusing, generated code is edited, or the framework/emitter/runtime changes without re-verification.

## Re-confirmed 2026-08-22

Reconfirmed after all implementation and documentation edits: SCUS-94244 regenerated 297->986 candidates at emitter 2026-08-22.1; calls one through eight each matched 34/34, call three matched game main 0x80048AA0, syscall boundary and IRQ 1->0 passed, every opposite-answer control passed, authoritative Clang CTest passed 5/5, and cached retail identity/runtime/crt0 gates passed 4/4, 15/15 plus mutation, and 6/6.

## Re-confirmed 2026-08-22

Post-change authoritative verification: python3 tools/verify.py passed Clang build and CTest 5/5; cached serial-scoped executable/oracle/runtime gates passed as documented on 2026-08-22.

## Re-confirmed 2026-08-24

Against recorded psxport d2266f4b, the refactored TitleBoundaryRunner preserved real SCUS_942.44 generation (297 seeds to 986 candidates), eight calls at 34/34 fields each including game main, the separate IRQ 1-to-0 HLE transition, all opposite-answer controls, and SELFTEST 12/12.

## Re-confirmed 2026-08-24

At fetchable recorded psxport bc8c8897, real SCUS_942.44 regenerated 297 seeds to 986 candidates; eight calls agreed 34/34 including game main, the separate IRQ transition passed, and SELFTEST passed 12/12.

## Re-confirmed 2026-08-24

After aca3ecb, Crash 3 calls one through eight each agreed 34/34, oracle vectored at step 75963, and controls passed 12/12.

## Re-confirmed 2026-08-24

Post-landing SCUS-94244 generated/oracle execution remains 34/34 through eight resident calls, including game main and the title-specific syscall wrapper; its 13/13 boundary suite and clean 9c2e3f1c pin were reconfirmed.
