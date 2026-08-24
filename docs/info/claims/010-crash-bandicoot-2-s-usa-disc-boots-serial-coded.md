---
id: C010
kind: claim
status: holds
created: 2026-08-22
tags: crash2,identity,oracle
depends: titles/crash2/executable.json, tools/provision_title.py
reconfirmed: 2026-08-24
verified_at: 2026-08-24 19:46:31
---

## Claim

Crash Bandicoot 2's USA disc boots serial-coded executable SCUS_941.54, and its first executed crt0 call agrees between symbolic decode and the independent CPU oracle on all 6 comparable fields

## Evidence

Real USA CHD provisioning matched SYSTEM.CNF to titles/crash2/executable.json at 11/11 facts; verify_executable selftest passed 4/4; clean psxport 7f5d3f13 crossvalidate_crt0 reached jal ordinal 1 at step 81725, target 0x8001144C, and reported 6 agree, 0 disagree, 0 unseen. tests/test_provision_titles.py passed 9/9 including Crash 1 boot-target refusal.

## What would falsify it

A supplied USA disc boots a target other than SCUS_941.54, any tracked hash/header/marker changes, the serial-aware provisioner accepts Crash 1 as Crash 2, or the independent oracle disagrees at the first call.

## Re-confirmed 2026-08-22

Pinned ad5cf802 Crash 2 real gate matched SYSTEM.CNF and executable 11/11, passed oracle_spike 39/39, and symbolic/independent first-call state agreed 6/6 at step 81725 target 0x8001144C.

## Re-confirmed 2026-08-22

Post-change verification on clean psxport ad5cf802: Clang CTest 4/4, title provision tests 9/9, Crash 1 boundary SELFTEST 12/12 with all eight calls 34/34 and EnterCriticalSection IRQ 1->0, Crash 1 oracle 39/39 and crt0 6/6, Crash 2 identity 11/11/runtime facts 15/15/oracle 39/39/crt0 6/6.

## Re-confirmed 2026-08-22

Refactored three-title provisioning passed 10/10; real Crash 2 still matched 11/11, runtime 15/15 plus altered-fact disagreement, oracle_spike 39/39, and first-call state 6/6 at step 81725 target 0x8001144C.

## Re-confirmed 2026-08-22

Post-commit default-tree real Crash 2 gate passed identity 11/11, executable selftest 4/4, oracle 39/39, and first call 6/6.

## Re-confirmed 2026-08-24

At recorded psxport d2266f4b, cached real SCUS_941.54 passed identity selftest 4/4, oracle_spike 43/43, and symbolic/independent first-call comparison 6/6 at step 81725 target 0x8001144C; the 10-case provisioning contract passed in CTest. No disc was configured for fresh reprovisioning, so prior SYSTEM.CNF evidence remains the disc-selection authority.
