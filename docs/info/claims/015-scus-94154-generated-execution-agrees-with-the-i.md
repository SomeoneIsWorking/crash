---
id: C015
kind: claim
status: holds
created: 2026-08-22
tags: crash2,recomp,oracle,syscall
depends: tools/resident_recomp.py#check_comparison, tests/recomp_boundary_support.h#TitleBoundaryRunner, titles/crash2/core/crash2_runtime.cpp#programImage_
reconfirmed: 2026-08-24
verified_at: 2026-08-24 20:05:17
---

## Claim

SCUS-94154 generated execution agrees with the independent CPU through eight resident calls and reaches its own EnterCriticalSection boundary

## Evidence

Against psxport d2266f4b, tools/scus_94154_recomp.py --selftest emitted 998 candidates from 270 seeds and compared calls 1-8 at 34/34 fields each: 0x8001144C, 0x800117BC, game main 0x80049BD4, 0x80015614, 0x8004B1B8, 0x8004EC30, 0x8004F1F8, and 0x80049D1C. The oracle entered 0xBFC00180 at step 85898. The generated wrapper separately returned prior IRQ 1 and disabled delivery. The 12/12 both-answer suite refused out-of-text seed, 7/8 trace, repeated target, altered register, and wrong syscall target cases.

## What would falsify it

Any of the eight real call states disagrees; the 12-case controls stop producing both answers; the executable identity changes; or a later syscall-resume comparison disproves the boundary state.

## Re-confirmed 2026-08-24

Against recorded psxport d2266f4b, real SCUS_941.54 regenerated from 270 seeds to 998 candidates; calls one through eight each agreed 34/34, call three independently reached game main 0x80049BD4, call eight reached wrapper 0x80049D1C, the oracle then entered 0xBFC00180 at step 85898, the separate HLE IRQ 1-to-0 transition passed, and every 12/12 opposite-answer control passed.

## Re-confirmed 2026-08-24

At fetchable recorded psxport bc8c8897, real SCUS_941.54 regenerated 270 seeds to 998 candidates; eight calls agreed 34/34 including game main 0x80049BD4 and wrapper 0x80049D1C, the separate IRQ transition passed, and SELFTEST passed 12/12.
