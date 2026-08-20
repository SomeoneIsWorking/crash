---
id: I001
kind: instrument
status: trusted
created: 2026-08-20
---

## Instrument

tools/verify_executable.py — compares tracked title identity/header facts to a real PS-X EXE through psxport psexe

## Validated by

2026-08-20: real SCUS_949.00 matched 11/11; selftest 4/4 proved a wrong manifest hash and one-byte executable mutation fail, while malformed input refuses instead of passing over no corpus.

## Known failure modes

This verifies the supplied executable, not which disc produced it. It also depends on psxport's PS-X
EXE header parser; the independent hash and embedded-marker checks do not independently validate that
parser's decoded addresses.
