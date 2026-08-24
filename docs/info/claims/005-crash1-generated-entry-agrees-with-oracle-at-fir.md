---
id: C005
kind: claim
status: holds
created: 2026-08-21
tags: crash1,recompiler,oracle,crt0
depends: CMakeLists.txt, psxport.pin, titles/crash1/executable.json, titles/crash1/recomp_seeds.json, tools/crash1_recomp.py, tests/crash1_recomp_boundary.cpp
reconfirmed: 2026-08-24
verified_at: 2026-08-24 20:14:17
---

## Claim

Crash 1's generated resident entry path agrees with the independent reference CPU on the complete
comparable CPU state at the first executed crt0 call in the verified North American executable.

## Evidence

Pinned psxport `3418a79b` emitter version `2026-08-12.1` discovered 115 static candidate seeds and emitted 653
resident candidates with zero configured overlays. The generated entry `0x8003E018` reached override
target `0x80011A18`; against the independent oracle's first executed call, `pc`, all 31 nonzero GPRs,
`lo`, and `hi` agreed 34/34. The same gate proved the emitter refuses an out-of-text seed and the
comparator names a one-bit `a0` disagreement.

## What would falsify it

Any of the 34 fields disagrees or disappears; generated execution returns or misses before the
boundary; the oracle first call differs from the symbolic target; the negative controls pass; the
verified executable bytes, seed policy, emitter, runner, comparator, or framework pin changes
without re-verification.

## Re-confirmed 2026-08-21

Post-format final gate: the real USA executable expanded 115 roots into 653 emitted candidates; the
generated entry matched independent oracle state 34/34; out-of-text seed refusal and one-bit `a0`
disagreement controls both passed.

## Re-confirmed 2026-08-21

Post-landing crash1_recomp_boundary_check passed 4/4: 115 static candidate seeds -> 653 emitted
candidates, out-of-text refusal, 34/34 real oracle agreement, and named one-bit a0 mismatch.

## Re-confirmed 2026-08-21

Pinned psxport `9f1bb927`: 115 static candidate seeds produced 653 candidates; generated call one
matched independent oracle 34/34 at step 57,910 target `0x80011A18`; out-of-text emission and named
mismatch controls refused/disagreed.

## Re-confirmed 2026-08-21

Post-landing generated call one remained 34/34 at step 57910 with out-of-text and altered-register controls passing.

## Re-confirmed 2026-08-21

Pinned psxport 692b9b20: 115 static candidate seeds emitted 653 candidates; generated call one matched the independent oracle 34/34 at step 57910, while out-of-text emission refused and the altered-register comparator produced one named mismatch.

## Re-confirmed 2026-08-21

Post-landing crash1_recomp_boundary_check passed 9/9; generated execution matched the independent oracle 34/34 at calls one through four, with the short-window and out-of-text refusals passing on psxport 692b9b20.

## Re-confirmed 2026-08-21

Pinned psxport ce2c83ad: 115 static candidate seeds emitted 653 candidates; generated call one matched the independent oracle 34/34 at step 57910, while out-of-text emission refused and the altered-register comparator produced one named mismatch.

## Re-confirmed 2026-08-21

Post-landing ce2c83ad four-call boundary gate passed 9/9 with 34/34 at calls one through four and all refusal controls intact.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b: 115 static candidate seeds emitted 653 candidates; generated call one matched the independent oracle 34/34 at step 57910, while out-of-text emission refused and the altered-register comparator produced one named mismatch.

## Re-confirmed 2026-08-21

Post-landing generated entry matched all 34 compared fields at each of four canonical oracle calls; boundary selftest passed 9/9.

## Re-confirmed 2026-08-22

Clean pinned 3418a79b build after whole-Game runner initialization: call one remains 34/34 and all four negative-control classes pass within SELFTEST 9/9.

## Re-confirmed 2026-08-22

Pinned psxport 7f5d3f13 generated runner with the direct Crash1Runtime installed matched the independent oracle 34/34 at call one; all emission, short-window, ambiguity, and altered-register negative controls passed in SELFTEST 9/9.

## Re-confirmed 2026-08-22

Pinned psxport 7f5d3f13 boundary gate matched all 34 fields at call one and retained all refusal controls within SELFTEST 12/12.

## Re-confirmed 2026-08-22

Re-ran crash1_recomp_boundary_check after the boundary-runtime inheritance refactor: calls 1-8 each agreed 34/34 and SELFTEST passed 12/12.

## Re-confirmed 2026-08-22

Pinned ad5cf802 crash1_recomp_boundary_check emitted 115->653 candidates and matched call one 34/34; all controls passed within SELFTEST 12/12.

## Re-confirmed 2026-08-22

Post-change verification on clean psxport ad5cf802: Clang CTest 4/4, title provision tests 9/9, Crash 1 boundary SELFTEST 12/12 with all eight calls 34/34 and EnterCriticalSection IRQ 1->0, Crash 1 oracle 39/39 and crt0 6/6, Crash 2 identity 11/11/runtime facts 15/15/oracle 39/39/crt0 6/6.

## Re-confirmed 2026-08-22

Post-Crash3 full Crash 1 boundary gate re-emitted 115 seeds to 653 candidates and retained 34/34 equality at call one plus all later verified calls.

## Re-confirmed 2026-08-22

Post-commit default-tree Crash 1 generated/syscall regression passed SELFTEST 12/12; all eight calls remained 34/34 and IRQ transition remained 1->0.

## Re-confirmed 2026-08-22

Post-change authoritative verification: python3 tools/verify.py passed Clang build and CTest 5/5; cached serial-scoped executable/oracle/runtime gates passed as documented on 2026-08-22.

## Re-confirmed 2026-08-24

At recorded psxport d2266f4b, real SCUS_949.00 regenerated 115 seeds to 653 candidates and its generated entry path agreed with the independent CPU at all eight call boundaries, 34/34 fields each; SELFTEST passed 12/12.

## Re-confirmed 2026-08-24

At fetchable recorded psxport bc8c8897, real SCUS_949.00 regenerated 115 seeds to 653 candidates and its generated entry path agreed with the independent CPU at eight call boundaries, 34/34 fields each; SELFTEST passed 12/12.

## Re-confirmed 2026-08-24

After aca3ecb, crash1_recomp_boundary_check regenerated 653 candidates and passed every entry/call comparison and 12/12 controls.
