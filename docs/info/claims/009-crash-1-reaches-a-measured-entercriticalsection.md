---
id: C009
kind: claim
status: holds
created: 2026-08-22
tags: crash1,syscall,oracle
depends: tools/crash1_recomp.py, tests/crash1_recomp_boundary.cpp, titles/crash1/core/crash1_runtime.cpp
reconfirmed: 2026-08-22
verified_at: 2026-08-22 18:17:21
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
