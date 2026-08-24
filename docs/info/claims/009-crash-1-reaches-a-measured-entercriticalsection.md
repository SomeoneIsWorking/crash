---
id: C009
kind: claim
status: holds
created: 2026-08-22
tags: crash1,syscall,oracle
depends: game/core/resident_program.cpp, game/core/enter_critical_frontier.cpp, titles/crash1/core/crash1_port.cpp, titles/crash1/executable.json, tools/resident_recomp.py, tests/recomp_boundary_support.h
reconfirmed: 2026-08-24
verified_at: 2026-08-24 23:06:25
---

## Claim

Crash 1 reaches a measured EnterCriticalSection syscall boundary after eight oracle-matched calls; its generated port wrapper executes the shipping HLE transition while post-syscall oracle equality remains explicitly unproven

## Evidence

Pinned psxport 7f5d3f13 crash1_recomp_boundary_check passed 12/12 on the real SCUS_949.00: eight call states agreed 34/34, oracle step 62083 entered BFC00180 after the addiu-a0-1/syscall-0 wrapper, port HLE returned prior IRQ 1 and changed IRQ delivery 1 to 0, and a different execution-proven function refused as the wrapper.

## What would falsify it

Any call state differs; the real wrapper words, selector, return value or IRQ transition changes; the wrong-function discriminator accepts; or this result is represented as post-syscall oracle agreement before Cause/EPC validation and EPC+4 resume exist.

## Re-confirmed 2026-08-22

Final pinned psxport 7f5d3f13 gate on real SCUS_949.00 passed SELFTEST 12/12: calls one through eight agreed 34/34, oracle syscall vector and port EnterCriticalSection behavior were distinguished, and the wrong-function discriminator refused.

## Re-confirmed 2026-08-22

After the inheritance-only Crash1Runtime refactor, crash1_recomp_boundary_check again passed all eight 34/34 call comparisons, port EnterCriticalSection IRQ 1->0, all negatives, and SELFTEST 12/12.

## Re-confirmed 2026-08-22

Pinned ad5cf802 Crash 1 boundary gate matched eight calls 34/34, then separately proved generated EnterCriticalSection IRQ 1->0 and all three refusal/disagreement controls; SELFTEST 12/12.

## Re-confirmed 2026-08-22

Post-change verification on clean psxport ad5cf802: Clang CTest 4/4, title provision tests 9/9, Crash 1 boundary SELFTEST 12/12 with all eight calls 34/34 and EnterCriticalSection IRQ 1->0, Crash 1 oracle 39/39 and crt0 6/6, Crash 2 identity 11/11/runtime facts 15/15/oracle 39/39/crt0 6/6.

## Re-confirmed 2026-08-22

Post-Crash3 full Crash 1 boundary gate executed the measured generated EnterCriticalSection wrapper, retained selector 1, return 1, IRQ 1 to 0, and refused a different proven function.

## Re-confirmed 2026-08-22

After extracting the shared serial-scoped runner/comparator at psxport 57a17a14, real SCUS_949.00 regenerated 115->653 candidates and retained eight calls at 34/34, the EnterCriticalSection IRQ 1->0 result, and all 12/12 controls.

## Re-confirmed 2026-08-22

Post-change authoritative verification: python3 tools/verify.py passed Clang build and CTest 5/5; cached serial-scoped executable/oracle/runtime gates passed as documented on 2026-08-22.

## Re-confirmed 2026-08-24

Against recorded psxport d2266f4b, the refactored TitleBoundaryRunner preserved real SCUS_949.00 generation (115 seeds to 653 candidates), eight calls at 34/34 fields each, the separate IRQ 1-to-0 HLE transition, all opposite-answer controls, and SELFTEST 12/12.

## Re-confirmed 2026-08-24

After aca3ecb, Crash 1 call eight agreed 34/34, oracle vectored at step 62083, generated HLE disabled IRQ, and mutation controls passed.

## Re-confirmed 2026-08-24

Against clean recorded psxport 9c2e3f1c on 2026-08-24, crash1_recomp_boundary_check passed 13/13: eight calls agreed 34/34, manifest-tracked game main and first syscall matched their ordinals, the oracle vectored at step 62083, the shared shipping EnterCritical owner returned prior IRQ 1 and disabled delivery, and all refusal controls passed. crash1_port linked the same ResidentProgram and EnterCritical owners.
