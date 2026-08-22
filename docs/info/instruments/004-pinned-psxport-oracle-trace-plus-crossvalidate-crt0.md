---
id: I004
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

The pinned psxport `57a17a14` `oracle_trace` plus `crossvalidate_crt0.py`, as wired by each title's CMake
oracle boundary target for its in-image libcInit shape.

## Validated by

Real verified `SCUS_949.00`, `SCUS_941.54`, and `SCUS_942.44` traces independently captured each
first executed crt0 jal at steps 57,910, 81,725, and 71,790 respectively; each crossvalidation agreed
on 6/6 fields after the pin change. The independent CPU fixture passed 39/39, including the
wrong-target and pending-load modeled-return refusals. The bounded crossvalidator selftest
passed 5/5, including an unreached-call refusal and a wrong-target disagreement; the canonical tracer
CLI selftest passed 8/8, including insufficient-window and wrong-BIOS-function refusals.

## Known failure modes

This compares only the first crt0 call boundary. It does not cover the full boot, BIOS implementation,
hardware devices, generated Crash code, or gameplay. Trust is tied to the recorded framework pin; a
changed oracle tracer or comparator requires both call-shape controls and the no-call refusal again.
