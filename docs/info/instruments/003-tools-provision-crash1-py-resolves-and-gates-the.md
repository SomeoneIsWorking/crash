---
id: I003
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

`tools/provision_crash1.py` — resolves a user-supplied disc, delegates ISO/CHD reads to psxport
`discdump`, verifies the boot target, and invokes the existing executable identity gate before
publishing under `scratch/`.

## Validated by

2026-08-21: a real Crash Bandicoot USA disc produced the expected boot target and 11/11 executable
match; a real Crash 2 USA disc produced the opposite answer and refused on `SCUS_941.54`. Nine tests
exercise every resolution source, precedence, invalid/ambiguous inputs, successful publish, and
failed identity without publish.

## Known failure modes

This establishes the supplied disc's boot executable identity, not a whole-disc hash, sector-level
provenance, successful emulated boot, gameplay, overlays, or native implementation. Disc parsing
depends on psxport `discdump`; executable parsing depends on psxport `psexe`.
