---
id: C022
kind: claim
status: holds
created: 2026-08-26
tags: crash1,bios,c0,exception
depends: titles/crash1/bios_contract.json, tests/crash1_c0_exception_contract.cpp
---

## Claim

Crash 1 consumes C0 table slot 6 as the retail C(06h) exception-handler address `0x00000C80`, then
copies fourteen words there before A(44h)

## Evidence

PSX-SPX's retail-BIOS analysis identifies C0 table function 6 as ExceptionHandler and its address as
`0x00000C80`: <https://psx-spx.consoledev.net/kernelbios/#bios-patches>. Independently, Ghidra 12.0.4
disassembly of the verified `SCUS_949.00` bytes shows B(56h) GetC0Table, `lw v0,0x18(v0)`, and a copy
loop over the exclusive source range `[0x80043204,0x8004323C)`. That range is 56 bytes, or fourteen
words, and is followed by A(44h). The shipping-HLE consumer probe returned the opposite answer before
the framework fix—its C0 table was `0x8000F800`, but slot 6 at `0x8000F818` read zero. Against clean
pushed psxport `99a42aa3`, the same seam returns `0x00000C80`. These two answers validate the consumer
probe without treating Crash's intended output as the source of the BIOS value.

## What would falsify it

The verified executable bytes cease to encode the stated B(56h), slot-6 load, copy endpoints, or
A(44h) continuation; a retail-BIOS trace shows C(06h) is not `0x00000C80`; or an independently seeded
oracle reaches the continuation with a different slot value.
