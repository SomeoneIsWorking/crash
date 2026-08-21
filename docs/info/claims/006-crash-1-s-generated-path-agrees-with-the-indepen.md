---
id: C006
kind: claim
status: holds
created: 2026-08-21
tags:
depends: CMakeLists.txt, psxport.pin, titles/crash1/executable.json, titles/crash1/recomp_seeds.json, tools/crash1_recomp.py, tests/crash1_recomp_boundary.cpp
reconfirmed: 2026-08-21
verified_at: 2026-08-21 13:08:10
---

## Claim

Crash 1's generated path agrees with the independent reference CPU on the complete comparable CPU
state at the first four executed calls, including the third call's return path.

## Evidence

CMake `crash1_recomp_boundary_check` on the verified USA executable consumed pinned psxport
`ce2c83ad`'s canonical ordinal captures. Generated execution matched the independent oracle 34/34 at
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
