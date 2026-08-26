---
id: C021
kind: claim
status: holds
created: 2026-08-26
tags: crash1,boot,bios,product
depends: titles/crash1/core/crash1_boot_frontier.cpp#makeBootFrontierProgram, titles/crash1/core/crash1_boot_frontier.cpp#checkPostGetC0Dispatch, tools/resident_recomp.py#require_tracked_bios_dispatch, titles/crash1/executable.json
---

## Claim

Crash 1's shipping product composition advances through its first B(56h) HLE call to the following A(44h) pre-HLE boundary

## Evidence

Ghidra 12.0.4 disassembly of the verified `SCUS_949.00` bytes at `0x8004319C..0x80043248` shows the syscall call, B(56h), `lw v0,0x18(v0)`, a fourteen-word copy over the exclusive source range `[0x80043204,0x8004323C)`, and the following A(44h) tail dispatch with `ra=0x800431E8`. Against clean pushed framework `99a42aa3`, the authoritative Clang verifier builds `crash1_port` and passes 8/8 CTests, including the 14/14 boot-frontier composition check and shipping C0 consumer contract. The real resident differential gate remains 18/18 through the independently matched B(56h) pre-HLE state. No product process was launched, so serialized runtime reach to A(44h) remains unverified; the ordered oracle separately proves the patch copy completes but currently stops at local wrapper `0x8004323C` before its non-link A(44h) tail dispatch.

## What would falsify it

A product build stops configuring the syscall as a returning transition or A(44h) as the final dynamic boundary, the verified executable bytes no longer encode the stated B(56h) continuation, the independent CPU disagrees with the tracked first-BIOS facts, or a serialized product run fails to reach A(44h).
