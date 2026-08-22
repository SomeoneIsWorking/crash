---
id: I003
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

`tools/provision_title.py` — selects one serial-identified Crash title, resolves its user-supplied
disc, delegates ISO/CHD reads to psxport `discdump`, verifies the title-specific boot target, and
invokes the existing executable identity gate before publishing under `scratch/`.

## Validated by

2026-08-22: real Crash 1, Crash 2, and Crash 3 USA discs each produced their expected serial-coded
boot target and an 11/11 executable match. The Crash 3 disc also contains `DRAGON/SPYRO.EXE`; the
instrument followed `SYSTEM.CNF` to `SCUS_942.44`, while a fixture that made the bundled path the boot
target refused. Ten tests exercise every resolution source, three-way title-key isolation,
invalid/ambiguous inputs, successful publish, wrong-title boot targets, and failed identity without
publish.

## Known failure modes

This establishes the supplied disc's boot executable identity, not a whole-disc hash, sector-level
provenance, successful emulated boot, gameplay, overlays, or native implementation. Disc parsing
depends on psxport `discdump`; executable parsing depends on psxport `psexe`.
