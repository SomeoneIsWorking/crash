---
id: C023
kind: claim
status: holds
created: 2026-08-26
tags: crash1,oracle,bios,c0,copy
depends: titles/crash1/bios_contract.json, titles/crash1/executable.json
---

## Claim

Crash 1's real ordered independent-CPU trace preserves the selector-1 and B(56h) returns plus the C0
slot seed through completion of the fourteen-word exception patch copy

## Evidence

Against clean pushed psxport `99a42aa3`, `oracle_trace` executes verified `SCUS_949.00`, models
selector-1 return at EPC `0x8003E1FC`, models B(56h) returning `0x8000F800`, and seeds
`0x8000F818=0x00000C80`. Its first later captured boundary is local wrapper `0x8004323C` at step 62,165
with `ra=0x800431E8` and `v0=0x00000CB8`, exactly one word past the fourteen-word destination. Repeating
the real trace with seed `0x00000C84` reaches the same wrapper and step with `v0=0x00000CBC`, providing
the opposite answer. This claim stops at the wrapper; issue #8 records why A(44h) is not yet captured.

## What would falsify it

The verified executable no longer encodes the tracked copy loop, either ordered trace fails to apply
both modeled returns and its explicit RAM seed, the two seeds cease to shift the copy endpoint by four
bytes, or an exact same-CPU trace shows the wrapper is reached before all fourteen stores complete.
