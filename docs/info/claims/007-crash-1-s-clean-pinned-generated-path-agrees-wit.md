---
id: C007
kind: claim
status: holds
created: 2026-08-22
tags:
depends: CMakeLists.txt
reconfirmed: 2026-08-24
verified_at: 2026-08-24 23:08:05
---

## Claim

Crash 1's clean pinned generated path agrees with the independent reference CPU at all eight resident calls before the BIOS exception boundary.

## Evidence

A clean Clang build against recorded psxport 3418a79b ran CMake crash1_recomp_boundary_check: calls 1-8 at steps 57910, 57931, 57935, 57962, 62065, 62073, 62077, and 62081 each agreed on 34/34 CPU-state fields. The out-of-text seed, real 7/8 short trace, repeated-target ambiguity, and altered-register controls produced their required opposite answers; SELFTEST 9/9. The oracle then leaves mapped text at 0xBFC00180 at step 62083.

## What would falsify it

Any target or state field disagrees/disappears; the clean runner no longer owns a Game; a negative control passes; executable bytes, emitter, oracle capture, runner, comparator, or framework pin changes without re-verification.

## Re-confirmed 2026-08-22

Pinned psxport 7f5d3f13 generated runner with the direct Crash1Runtime installed matched the independent oracle 34/34 at all eight resident calls before 0xBFC00180; execution denominator remained 9 proven addresses and all negative controls passed in SELFTEST 9/9.

## Re-confirmed 2026-08-22

Fresh pinned psxport 7f5d3f13 Clang build matched all eight calls 34/34, then controlled the port EnterCriticalSection transition separately; three refusal or disagreement controls passed and SELFTEST reported 12/12.

## Re-confirmed 2026-08-22

Re-ran crash1_recomp_boundary_check against the current framework integration after the runtime refactor: emission and all eight boundaries passed; SELFTEST 12/12.

## Re-confirmed 2026-08-22

Clean Clang build recording and pinning psxport ad5cf802 matched all eight Crash 1 calls 34/34 and passed the full SELFTEST 12/12.

## Re-confirmed 2026-08-22

Post-change verification on clean psxport ad5cf802: Clang CTest 4/4, title provision tests 9/9, Crash 1 boundary SELFTEST 12/12 with all eight calls 34/34 and EnterCriticalSection IRQ 1->0, Crash 1 oracle 39/39 and crt0 6/6, Crash 2 identity 11/11/runtime facts 15/15/oracle 39/39/crt0 6/6.

## Re-confirmed 2026-08-22

Fresh Crash3-era Clang build against recorded ad5cf802 regenerated Crash 1 and retained eight-call 34/34 equality, execution denominator 9, and SELFTEST 12/12.

## Re-confirmed 2026-08-22

Post-commit default-tree Crash 1 generated/syscall regression passed SELFTEST 12/12; all eight calls remained 34/34 and IRQ transition remained 1->0.

## Re-confirmed 2026-08-22

Post-change authoritative verification: python3 tools/verify.py passed Clang build and CTest 5/5; cached serial-scoped executable/oracle/runtime gates passed as documented on 2026-08-22.

## Re-confirmed 2026-08-24

The authoritative Clang tree and pin check used clean recorded psxport d2266f4b; the real SCUS_949.00 boundary gate then passed all eight 34/34 comparisons and SELFTEST 12/12.

## Re-confirmed 2026-08-24

The authoritative Clang tree and pin check used clean fetchable psxport bc8c8897; real SCUS_949.00 then passed eight 34/34 generated/oracle comparisons and SELFTEST 12/12.

## Re-confirmed 2026-08-24

After aca3ecb, the clean recorded bc8c8897 build regenerated Crash 1 and reproduced all eight 34/34 comparisons plus controls.

## Re-confirmed 2026-08-24

Against newly recorded clean psxport 9c2e3f1c, Crash 1 again matched all eight call states 34/34, tracked game main and syscall ordinals by manifest, and passed all 13 controls; pin check agreed with both Clang build trees.
