---
id: C004
kind: claim
status: holds
created: 2026-08-21
tags: crash1,oracle,crt0
depends: CMakeLists.txt, psxport.pin, titles/crash1/executable.json, tools/provision_crash1.py
reconfirmed: 2026-08-22
verified_at: 2026-08-22 17:39:21
---

## Claim

The independent reference CPU and symbolic decoder agree on all 6 comparable fields at Crash 1's
first crt0 call boundary in the verified North American executable.

## Evidence

CMake `crash1_oracle_boot_check` re-provisioned the 11/11-matching executable, ran the oracle fixture
39/39, and reached the first executed jal at step 57,910, target `0x80011A18`. The independent state
agreed on gp, target, a0, planned sp, planned a0, and planned a1. A 50,000-step run refused with no
call and zero comparisons.

## What would falsify it

Falsified if either method changes, any of the 6 fields disagrees or disappears, the no-call control
reports agreement, provisioning yields different bytes, or the framework pin changes without
re-verification.

## Re-confirmed 2026-08-21

Reverified through the final crash1_oracle_boot_check target at pinned psxport 2b5ef7b5: real USA executable matched 11/11, oracle fixture passed 22/22, first crt0 call at step 57,910 agreed 6/6, and a 50,000-step no-call window refused with exit 2 and zero comparisons.

## Re-confirmed 2026-08-21 03:05:54

2026-08-21 post-integration: crash1_oracle_boot_check reprovisioned the 11/11 real USA executable, oracle_spike passed 22/22, first call at step 57910 targeted 0x80011A18, crossvalidation agreed 6/6, and the 50000-step control refused exit 2.

## Re-confirmed 2026-08-21 03:06:27

2026-08-21 landed integration recheck: the unchanged real oracle evidence remains 22/22 fixture, step 57910 target 0x80011A18, 6/6 agreement, and 50000-step refusal.

## Re-confirmed 2026-08-21

Post-landing real USA oracle boot gate passed: identity 11/11, oracle fixture 22/22, first call step 57910, symbolic/oracle 6/6.

## Re-confirmed 2026-08-21

Pinned psxport 9f1bb927: crash1_oracle_boot_check reprovisioned the real USA executable at 11/11 identity facts, oracle_spike passed 39/39, first call remained step 57910 target 0x80011A18, and symbolic/oracle state agreed 6/6.

## Re-confirmed 2026-08-21

Post-landing crash1_oracle_boot_check passed identity 11/11, oracle_spike 39/39, and symbolic/oracle first-call agreement 6/6 on psxport 9f1bb927.

## Re-confirmed 2026-08-21

Pinned psxport 692b9b20: verified executable identity 11/11, independent CPU fixture 39/39, symbolic/oracle first-call state 6/6 at step 57910 target 0x80011A18, bounded crossvalidator controls 5/5, and canonical tracer CLI controls 8/8.

## Re-confirmed 2026-08-21

Post-landing executable identity passed 11/11, oracle_spike passed 39/39, and independent symbolic/oracle first-call comparison passed 6/6 at step 57910 on psxport 692b9b20.

## Re-confirmed 2026-08-21

Pinned psxport ce2c83ad: verified executable identity 11/11, independent CPU fixture 39/39, symbolic/oracle first-call state 6/6 at step 57910 target 0x80011A18, bounded crossvalidator controls 5/5, and canonical tracer CLI controls 8/8.

## Re-confirmed 2026-08-21

Post-landing ce2c83ad verification passed executable identity 11/11, oracle_spike 39/39, and the real symbolic/oracle crt0 comparison 6/6 at step 57910.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b: verified executable identity passed 11/11, independent CPU fixture passed 39/39, symbolic/oracle first-call state agreed 6/6 at step 57910 target 0x80011A18, bounded crossvalidator controls passed 5/5, and canonical tracer CLI controls passed 8/8.

## Re-confirmed 2026-08-21

Post-landing real Crash crt0 crossvalidation remained 6/6 against psxport 3418a79b.

## Re-confirmed 2026-08-22

Pinned psxport 7f5d3f13 crash1_oracle_boot_check reprovisioned the real USA disc executable with 11/11 facts, passed the independent CPU fixture 39/39, and crossvalidated the first crt0 call 6/6 at step 57,910.

## Re-confirmed 2026-08-22

Pinned psxport 7f5d3f13 oracle_spike passed 39/39 and the real SCUS_949.00 symbolic/oracle first crt0 boundary agreed 6/6 at step 57910.
