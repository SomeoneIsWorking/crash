---
id: C003
kind: claim
status: holds
created: 2026-08-21
tags: crash1,disc,provisioning
depends: tools/provision_title.py, tools/verify_executable.py, titles/crash1/executable.json
reconfirmed: 2026-08-22
verified_at: 2026-08-22 18:39:57
---

## Claim

Crash 1 provisioning publishes an executable only when the selected disc's `SYSTEM.CNF` boots
`SCUS_949.00` and the extracted file matches all 11 tracked North American executable facts.

## Evidence

A real Crash Bandicoot USA disc passed the boot-target and 11/11 executable gate. A real Crash 2 USA
disc refused on its `SCUS_941.54` boot target without replacing the provisioned Crash 1 executable.
Nine production-seam tests additionally prove both publish/refusal answers and every resolution
source.

## What would falsify it

Falsified if a non-Crash-1 disc or identity mismatch publishes output, a valid USA disc refuses, an
invalid configured source falls through, or extracted copyrighted data appears in tracked files.

## Re-confirmed 2026-08-22

Pinned ad5cf802 real-disc gates provisioned Crash 1 and Crash 2 only after each SYSTEM.CNF target matched 11/11 executable facts; the shared serial-aware contract passed 9/9 including wrong-title refusal.

## Re-confirmed 2026-08-22

Post-change verification on clean psxport ad5cf802: Clang CTest 4/4, title provision tests 9/9, Crash 1 boundary SELFTEST 12/12 with all eight calls 34/34 and EnterCriticalSection IRQ 1->0, Crash 1 oracle 39/39 and crt0 6/6, Crash 2 identity 11/11/runtime facts 15/15/oracle 39/39/crt0 6/6.

## Re-confirmed 2026-08-22

The refactored three-title provisioner passed 10/10 tests and real Crash 1 provisioning still followed SYSTEM.CNF to SCUS_949.00 and matched 11/11 facts.

## Re-confirmed 2026-08-22

Post-commit default-tree provisioning regression passed all 10 three-title tests; real Crash 1 identity remained 11/11.
