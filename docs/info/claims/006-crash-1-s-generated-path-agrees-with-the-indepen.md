---
id: C006
kind: claim
status: holds
created: 2026-08-21
tags:
depends: CMakeLists.txt, psxport.pin, titles/crash1/executable.json, titles/crash1/recomp_seeds.json, tools/crash1_recomp.py, tests/crash1_recomp_boundary.cpp
reconfirmed: 2026-08-21
verified_at: 2026-08-21 11:17:18
---

## Claim

Crash 1's generated path agrees with the independent reference CPU on the complete comparable CPU state at both the first and second executed calls.

## Evidence

CMake crash1_recomp_boundary_check on the verified USA executable consumed pinned psxport `9f1bb927`'s canonical ordinal captures; generated execution matched the independent oracle 34/34 at step 57910 target 0x80011A18 and 34/34 at step 57931 target 0x80011D88. The shared oracle fixture passed 8/8, including distinct ordinal targets, first-call alias equality, the complete 33-register block, and insufficient-ordinal refusal; a real trace capped immediately before call two refused with a 1/2 denominator and no boundary block. The emitter refused an out-of-text seed and a changed second-boundary a0 produced exactly one named mismatch. The executable denominator is 3 observed addresses (2 generated bodies executed, 1 next call target), not 653 emitted candidates.

## What would falsify it

Any field or target disagrees/disappears; canonical ordinal capture omits or misorders a call; either
negative control passes; executable bytes, emitter, oracle capture, runner, comparator, or framework
pin changes without re-verification.

## Re-confirmed 2026-08-21

Pinned psxport `9f1bb927`: the canonical shared selftest passed 8/8; generated calls one and two
matched 34/34 at steps 57,910 and 57,931; the short real window refused 1/2 with no boundary block;
out-of-text emission refused; altered call-two `a0` produced one mismatch; Crash selftest passed 6/6.
