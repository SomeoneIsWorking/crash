---
id: C012
kind: claim
status: holds
created: 2026-08-22
tags: crash3,identity,oracle
depends: titles/crash3/executable.json, tools/provision_title.py, CMakeLists.txt
---

## Claim

Crash Bandicoot 3's USA disc boots serial-coded executable SCUS_942.44 rather than its bundled DRAGON/SPYRO.EXE, and its first executed crt0 call agrees between symbolic decode and the independent CPU oracle on all 6 comparable fields

## Evidence

Real USA CHD provisioning followed SYSTEM.CNF to SCUS_942.44 and matched titles/crash3/executable.json at 11/11 facts; verify_executable selftest passed 4/4; bundled DRAGON/SPYRO.EXE disagreed on 6 tracked identity/header fields; the 10-case provision suite proved SYSTEM.CNF selection; pinned psxport ad5cf802 oracle_spike passed 39/39 and crossvalidate_crt0 reached jal ordinal 1 at step 71790, target 0x800112B8, with 6 agree, 0 disagree, 0 unseen.

## What would falsify it

A supplied USA disc's SYSTEM.CNF boots a target other than SCUS_942.44, the provisioner selects bundled SPYRO.EXE by presence, any tracked hash/header/marker changes, or the independent oracle disagrees at the first call.
