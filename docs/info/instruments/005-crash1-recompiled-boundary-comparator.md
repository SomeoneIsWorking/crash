---
id: I005
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

`tools/crash1_recomp.py` plus `tests/crash1_recomp_boundary.cpp` — emits the Crash 1 resident
substrate, captures its first selected generated-call state, and compares it with psxport's
independent `oracle_trace` register block.

## Validated by

On the real verified USA executable the tool reported 34/34 agreement at `0x80011A18`. Its
production emitter path refused an explicit seed outside the executable text. Its production state
comparator also reported exactly one named mismatch when fed an otherwise-real port capture with
`a0` changed by one bit, proving the disagreement answer rather than only the all-equal answer.

## Known failure modes

The symbolic crt0 decoder selects the port interception target, while the oracle independently
captures its first executed call; disagreement between those targets refuses before state comparison.
The runner captures a function-call boundary, not every guest instruction, and this result covers
only entry-to-first-call execution. It does not certify the remaining emitted functions, BIOS,
devices, overlays, or gameplay.
