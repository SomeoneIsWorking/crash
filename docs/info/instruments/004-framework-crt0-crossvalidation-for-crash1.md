---
id: I004
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

Pinned psxport `ce2c83ad` `oracle_trace` plus `crossvalidate_crt0.py`, as wired by CMake
`crash1_oracle_boot_check` for Crash 1's in-image libcInit shape.

## Validated by

The independent CPU fixture passed 39/39. The real verified `SCUS_949.00` trace independently
captured the first executed crt0 jal at step 57,910 and crossvalidation agreed on 6/6 fields. The
bounded crossvalidator selftest passed 5/5, including an unreached-call refusal and a wrong-target
disagreement; the canonical tracer CLI selftest passed 8/8, including insufficient-window and wrong
BIOS-function refusals.

## Known failure modes

This compares only the first crt0 call boundary. It does not cover the full boot, BIOS implementation,
hardware devices, generated Crash code, or gameplay. Trust is tied to the recorded framework pin; a
changed oracle tracer or comparator requires both call-shape controls and the no-call refusal again.
