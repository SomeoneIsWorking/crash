---
id: 2
title: crt0 crossvalidator mistakes Crash first text exit for libcInit boundary
status: resolved
symptom: Crash 1 crossvalidate_crt0 reports 1 agreement, 5 disagreements, and 1 unseen field even though gp agrees
tags: crash1,oracle,instrument,crt0
created: 2026-08-21
updated: 2026-08-21
---

## Root cause

The old shared crossvalidator assumed crt0 reached libcInit by leaving mapped text through an A(39h)
BIOS thunk, so it compared the register snapshot at the first mapped-text exit. Crash 1 calls an
in-image libcInit at `0x80011A18`; `crt0_extract` explicitly reports A(39h) thunk NO. Execution
continues for 62,083 steps and first exits at `0xBFC00180` after unrelated jal `0x8003E1F8`, so that
snapshot could not validate the crt0 call.

## Evidence

The permanent oracle fixture passed 22/22. On the real verified SCUS_949.00, crt0_extract resolved 8/8 symbolic fields and libcInit 0x80011A18. oracle_trace then captured the unrelated later exit; crossvalidate reported gp agreement but 5 disagreements and 1 unseen field. This is a valid negative from an instrument outside its supported crt0 shape, not evidence that the executable identity is wrong.

## Proper fix

Generalize the framework oracle trace/crossvalidator to capture the first executed crt0 call and
compare that register state without selecting the expected symbolic target. Retain the BIOS-thunk
title control, the Crash in-image call control, and the no-call refusal.

## Resolution

Pinned psxport `2b5ef7b5` makes `oracle_trace` independently record the first executed jal after the
crt0 entry prologue instead of assuming libcInit leaves mapped text. Crash 1 reaches target
`0x80011A18` at step 57,910 and agrees 6/6; CTR remains 7/7; a 50,000-step Crash window refuses with
exit 2 and zero comparisons. CMake `crash1_oracle_boot_check` now owns the reproducible Crash gate.
