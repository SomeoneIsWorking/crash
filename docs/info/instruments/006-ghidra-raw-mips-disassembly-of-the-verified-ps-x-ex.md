---
id: I006
kind: instrument
status: trusted
created: 2026-08-26
---

## Instrument

Ghidra 12.0.4 raw-binary MIPS little-endian disassembly of the verified Crash 1 PS-X EXE, loaded at
`0x8000F800` so file offset `0x800` maps to the manifest text address `0x80010000`.

## Validated by

At `0x8003E1F8` the disassembler reports the already execution-validated `addiu a0,1; syscall 0`
wrapper, establishing the raw file-to-guest-address mapping against the independent boundary harness.
At `0x8004319C..0x80043248` it reports distinct B(56h) and A(44h) dispatches, the same return addresses,
C0 `+0x18` load, and fourteen-word copy emitted by psxport's separate MIPS decoder. The exclusive
source endpoints `0x80043204` and `0x8004323C` independently bound that denominator. The two selectors
and their intervening instructions provide a non-uniform result; this instrument is not trusted merely
because one expected mnemonic appeared.

## Known failure modes

Raw loading does not apply symbols, relocation metadata, or automatic function recovery. The base must
be recomputed from the PS-X EXE header and file-header size for another executable. This instrument
establishes instruction bytes and control flow only; it does not prove BIOS return values, live memory,
or that the shipping product reached the range.
