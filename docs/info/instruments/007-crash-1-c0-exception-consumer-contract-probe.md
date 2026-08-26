---
id: I007
kind: instrument
status: trusted
created: 2026-08-26
---

## Instrument

`crash1_c0_exception_contract_probe` calls shipping `Hle::dispatchBios('B', 0x56)`, reads the returned
guest C0 table through `Core::mem_r32`, and compares slot 6 with the tracked retail address from
`titles/crash1/bios_contract.json`.

## Validated by

Before the framework fix it produced the required other answer rather than a uniform pass: B(56h)
returned table `0x8000F800`, slot 6 at `0x8000F818` read `0x00000000`, and the probe refused the
independently sourced `0x00000C80` contract with exit status 2. Against clean pushed psxport
`99a42aa3`, the unchanged shipping-HLE read returns `0x00000C80` and exits zero. The probe is now a
normal CTest, so the instrument has demonstrated both answers through the same production seam.

## Known failure modes

This directly tests the shipping HLE table result, not serialized product reach, the fourteen-word
copy, or the independent CPU. A passing slot check would not by itself prove the A(44h) boundary or
later boot. The expected address remains trustworthy only while the cited retail-BIOS evidence holds.
