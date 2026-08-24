---
id: C012
kind: claim
status: holds
created: 2026-08-22
tags: crash3,identity,oracle
depends: titles/crash3/executable.json, tools/provision_title.py, CMakeLists.txt
reconfirmed: 2026-08-24
verified_at: 2026-08-24 19:46:32
---

## Claim

Crash Bandicoot 3's USA disc boots serial-coded executable SCUS_942.44 rather than its bundled DRAGON/SPYRO.EXE, and its first executed crt0 call agrees between symbolic decode and the independent CPU oracle on all 6 comparable fields

## Evidence

Real USA CHD provisioning followed SYSTEM.CNF to SCUS_942.44 and matched titles/crash3/executable.json at 11/11 facts; verify_executable selftest passed 4/4; bundled DRAGON/SPYRO.EXE disagreed on 6 tracked identity/header fields; the 10-case provision suite proved SYSTEM.CNF selection; pinned psxport ad5cf802 oracle_spike passed 39/39 and crossvalidate_crt0 reached jal ordinal 1 at step 71790, target 0x800112B8, with 6 agree, 0 disagree, 0 unseen.

## What would falsify it

A supplied USA disc's SYSTEM.CNF boots a target other than SCUS_942.44, the provisioner selects bundled SPYRO.EXE by presence, any tracked hash/header/marker changes, or the independent oracle disagrees at the first call.

## Re-confirmed 2026-08-22

At recorded psxport 57a17a14, cached retail SCUS_942.44 matched manifest and executable selftest passed 4/4; oracle_spike passed 39/39; independent/symbolic first-call state retained 6/6 at step 71790 target 0x800112B8. The 10-case provision contract still proves SYSTEM.CNF, not DRAGON/SPYRO.EXE, owns selection; no disc was configured for a fresh extraction this run.

## Re-confirmed 2026-08-22

Post-change authoritative verification: python3 tools/verify.py passed Clang build and CTest 5/5; cached serial-scoped executable/oracle/runtime gates passed as documented on 2026-08-22.

## Re-confirmed 2026-08-24

At recorded psxport d2266f4b, cached real SCUS_942.44 passed oracle_spike 43/43 and symbolic/independent first-call comparison 6/6 at step 71790 target 0x800112B8; the 10-case provisioning contract still passed, including DRAGON/SPYRO.EXE refusal. No disc was configured for fresh reprovisioning.
