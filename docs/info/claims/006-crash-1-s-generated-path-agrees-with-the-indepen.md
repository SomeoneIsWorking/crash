---
id: C006
kind: claim
status: holds
created: 2026-08-21
tags:
depends: CMakeLists.txt, psxport.pin, titles/crash1/executable.json, titles/crash1/recomp_seeds.json, tools/crash1_recomp.py, tests/crash1_recomp_boundary.cpp
reconfirmed: 2026-08-24
verified_at: 2026-08-24 20:05:16
---

## Claim

Crash 1's generated path agrees with the independent reference CPU on the complete comparable CPU
state at the first four executed calls, including the third call's return path.

## Evidence

CMake `crash1_recomp_boundary_check` on the verified USA executable consumed pinned psxport
`3418a79b`'s canonical ordinal captures. Generated execution matched the independent oracle 34/34 at
step 57,910 target `0x80011A18`, step 57,931 target `0x80011D88`, step 57,935 target
`0x8003E0C0`, and step 57,962 target `0x8001652C`. The fourth call occurs after the third target
returns through `0x80011D98`, so it checks that leaf body and return path. The shared oracle fixture
passed 8/8. A real trace capped immediately before call four refused with a 3/4 denominator and no
boundary block. The emitter refused an out-of-text seed and a changed fourth-boundary `a0` produced
exactly one named mismatch. The executable denominator is five observed addresses (four generated
bodies executed, one next call target), not 653 emitted candidates.

## What would falsify it

Any field or target disagrees/disappears; canonical ordinal capture omits or misorders a call; either
negative control passes; executable bytes, emitter, oracle capture, runner, comparator, or framework
pin changes without re-verification.

## Re-confirmed 2026-08-21

Pinned psxport `9f1bb927`: the canonical shared selftest passed 8/8; generated calls one and two
matched 34/34 at steps 57,910 and 57,931; the short real window refused 1/2 with no boundary block;
out-of-text emission refused; altered call-two `a0` produced one mismatch; Crash selftest passed 6/6.

## Re-confirmed 2026-08-21

Post-landing Crash selftest passed 6/6 and canonical oracle selftest 8/8; calls one and two remained 34/34 at steps 57910 and 57931.

## Re-confirmed 2026-08-21

Pinned psxport `692b9b20`: Crash selftest passed 9/9; calls one through four matched 34/34 at
steps 57,910, 57,931, 57,935, and 57,962, with call four following the call-three return through
`0x80011D98`. The real short trace refused 3/4, a repeated target refused as ordinal-ambiguous,
out-of-text emission refused, and altered call-four `a0` produced one named mismatch.

## Re-confirmed 2026-08-21

Post-landing crash1_recomp_boundary_check passed 9/9; calls one through four matched 34/34 at steps 57910, 57931, 57935, and 57962, with repeated-target ambiguity and altered-a0 controls producing the opposite answer.

## Re-confirmed 2026-08-21

Pinned psxport ce2c83ad: Crash selftest passed 9/9; calls one through four matched 34/34 at steps 57910, 57931, 57935, and 57962, including the call-three return. The real short trace refused 3/4, repeated-target ambiguity refused, out-of-text emission refused, and altered call-four a0 produced one named mismatch.

## Re-confirmed 2026-08-21

Post-landing ce2c83ad generated path retained 34/34 at steps 57910, 57931, 57935, and 57962; short trace, repeated target, out-of-text seed, and altered-a0 controls all refused.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b: Crash selftest passed 9/9; calls one through four matched 34/34 at steps 57910, 57931, 57935, and 57962, including the call-three return. The real short trace refused 3/4, repeated-target ambiguity refused, out-of-text emission refused, and altered call-four a0 produced one named mismatch.

## Re-confirmed 2026-08-21

Post-landing four-call generated path remained 34/34 at steps 57910, 57931, 57935, and 57962 with short-trace, repeated-target, out-of-text, and altered-a0 opposites passing.

## Re-confirmed 2026-08-22

Clean pinned 3418a79b build after whole-Game runner initialization: calls one through four remain 34/34 and all four negative-control classes pass within the extended eight-call SELFTEST 9/9.

## Re-confirmed 2026-08-22

Pinned psxport 7f5d3f13 generated runner with the direct Crash1Runtime installed matched the independent oracle 34/34 at calls one through four, including call three return; all negative controls passed in SELFTEST 9/9.

## Re-confirmed 2026-08-22

Pinned psxport 7f5d3f13 boundary gate matched all 34 fields at calls one through four and retained all refusal controls within SELFTEST 12/12.

## Re-confirmed 2026-08-22

Re-ran crash1_recomp_boundary_check after the boundary-runtime inheritance refactor: complete comparable state agreed at all eight calls and SELFTEST passed 12/12.

## Re-confirmed 2026-08-22

Pinned ad5cf802 crash1_recomp_boundary_check matched calls one through eight 34/34, including the call-three return path; SELFTEST 12/12.

## Re-confirmed 2026-08-22

Post-change verification on clean psxport ad5cf802: Clang CTest 4/4, title provision tests 9/9, Crash 1 boundary SELFTEST 12/12 with all eight calls 34/34 and EnterCriticalSection IRQ 1->0, Crash 1 oracle 39/39 and crt0 6/6, Crash 2 identity 11/11/runtime facts 15/15/oracle 39/39/crt0 6/6.

## Re-confirmed 2026-08-22

Post-Crash3 full Crash 1 boundary gate retained 34/34 equality at each of eight executed call boundaries and SELFTEST 12/12.

## Re-confirmed 2026-08-22

Post-commit default-tree Crash 1 generated/syscall regression passed SELFTEST 12/12; all eight calls remained 34/34 and IRQ transition remained 1->0.

## Re-confirmed 2026-08-22

Post-change authoritative verification: python3 tools/verify.py passed Clang build and CTest 5/5; cached serial-scoped executable/oracle/runtime gates passed as documented on 2026-08-22.

## Re-confirmed 2026-08-24

At recorded psxport d2266f4b, real SCUS_949.00 retained eight generated/oracle call comparisons at 34/34 fields each, separate EnterCriticalSection HLE evidence, and every 12/12 opposite-answer control.

## Re-confirmed 2026-08-24

At fetchable recorded psxport bc8c8897, real SCUS_949.00 retained eight generated/oracle comparisons at 34/34 fields each, separate EnterCriticalSection HLE evidence, and all 12/12 opposite-answer controls.
