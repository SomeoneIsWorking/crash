---
id: C003
kind: claim
status: holds
created: 2026-08-21
tags: crash1,disc,provisioning
depends: tools/provision_crash1.py, tools/verify_executable.py, titles/crash1/executable.json
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
