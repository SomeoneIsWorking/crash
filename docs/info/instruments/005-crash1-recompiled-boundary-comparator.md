---
id: I005
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

`tools/crash1_recomp.py` plus `tests/crash1_recomp_boundary.cpp` — emits Crash 1's resident
candidates, consumes psxport's canonical independent-oracle ordinal call captures, captures the same
generated-call states, and compares complete register files.

## Validated by

The shared oracle selftest passes 8/8; it proves that calls one and two capture distinct targets,
the first-call alias selects ordinal one, a capture contains all 33 canonical registers, and missing
call three refuses without a boundary block. On the real verified USA executable the
consumer reported port/oracle agreement 34/34 at call one `0x80011A18` and 34/34 at call two
`0x80011D88`. Its production emitter refused an explicit seed outside executable
text. A real oracle trace capped immediately before call two refused with only `1/2` boundaries. Its
production state comparator reported exactly one named mismatch when fed the real second port
capture with `a0` changed by one bit, proving both missing-boundary and disagreement answers rather
than only the all-equal answer.

## Known failure modes

The symbolic crt0 decoder selects only the expected first target. The shared oracle recovers the
second target from actual execution; the consumer does not reimplement `jal` or delay-slot tracking.
The runner captures function-call boundaries, not every guest instruction, and
this result covers only entry-through-second-call execution. It does not certify the remaining
emitted candidates, BIOS, devices, overlays, or gameplay. The 115 static seeds and 653 emitted
candidates are discovery denominators; only three addresses currently have execution provenance.
