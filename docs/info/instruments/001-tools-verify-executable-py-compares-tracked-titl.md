---
id: I001
kind: instrument
status: trusted
created: 2026-08-20
---

## Instrument

tools/verify_executable.py — compares tracked title identity, header, and VSync-body facts to a real PS-X EXE through psxport psexe

## Validated by

2026-08-20: real SCUS_949.00 matched 11/11; selftest 4/4 proved a wrong manifest hash and one-byte executable mutation fail, while malformed input refuses instead of passing over no corpus.

2026-08-27: cached retail SCUS_949.00, SCUS_941.54, and SCUS_942.44 each matched 12/12.
Each title's selftest passed 6/6; a shifted VSync entry disagreed on the body fingerprint, and an
altered VSync byte still disagreed after updating both whole-file hashes. This demonstrates both
answers specifically at the new body-fingerprint boundary.

2026-08-27: the instrument now discovers every nested runtime range carrying `entry`, `end`, and
`body_sha256`, rather than leaving Crash 1's native boot/frame ranges as unchecked prose. Cached
SCUS_949.00 matched 27/27 facts and its selftest passed 8/8, including a shifted native-runtime entry
and an altered native-runtime byte after updating both whole-file hashes. Crash 2 and Crash 3 retain
their 12/12 matches and 6/6 controls because they currently declare only the VSync range.

## Known failure modes

This verifies the supplied executable, not which disc produced it or whether any caller reaches
VSync. It also depends on psxport's PS-X EXE header parser; the independent whole-file hash and
embedded-marker checks do not independently validate that parser's decoded addresses.
