---
id: C001
kind: claim
status: holds
created: 2026-08-20
tags: crash1,executable
depends: titles/crash1/executable.json, tools/verify_executable.py
---

## Claim

Crash Bandicoot targets the North American SCUS-94900 executable SCUS_949.00; the measured image is 290816 bytes, SHA-1 b2af088a162e046ad07f532d46b655cfcc42a05c, with entry 0x8003E018 and text [0x80010000,0x80056800).

## Evidence

tools/verify_executable.py --check over the real image matched 11/11 manifest facts: name, size, SHA-1, SHA-256, six PS-X EXE header fields, and both embedded region markers. --selftest passed 4/4 including wrong tracked hash, mutated executable, and malformed executable.

## What would falsify it

Falsified if a newly provisioned North American retail disc extracts different executable bytes, any tracked manifest fact differs from the executable, the entry falls outside its declared text, or the embedded North America/SCUS markers are absent.

## Re-confirmed 2026-08-21

`tools/provision_crash1.py` used psxport `discdump` on a real USA disc: `SYSTEM.CNF` selected
`SCUS_949.00`, and the extracted image matched all 11 manifest facts including SHA-256
`aabf1464f90b2e0b81e712b77aebbdb88f303b16ce830535e2b0cd886ee280f2`.
