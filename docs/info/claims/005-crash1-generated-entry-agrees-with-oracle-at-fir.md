---
id: C005
kind: claim
status: holds
created: 2026-08-21
tags: crash1,recompiler,oracle,crt0
depends: CMakeLists.txt, psxport.pin, titles/crash1/executable.json, titles/crash1/recomp_seeds.json, tools/crash1_recomp.py, tests/crash1_recomp_boundary.cpp
reconfirmed: 2026-08-21
verified_at: 2026-08-21 03:32:04
---

## Claim

Crash 1's generated resident entry path agrees with the independent reference CPU on the complete
comparable CPU state at the first executed crt0 call in the verified North American executable.

## Evidence

Pinned psxport `2b5ef7b5` emitter version `2026-08-12.1` discovered 115 binary roots and emitted 653
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
