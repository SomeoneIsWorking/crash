---
id: 7
title: Crash patches address zero because the native C0 table omits slot 6
status: resolved
symptom: After B(56h), Crash copies its exception patch to address zero instead of the retail C(06h) handler at 0x00000C80
tags: crash1,bios,c0,oracle,framework
state_items: S003,S004
created: 2026-08-26
updated: 2026-08-26
---

## Root cause

Crash's verified routine at `0x8004319C` calls B(56h) GetC0Table, loads table slot 6 with
`lw v0,0x18(v0)`, then copies the 14 words in `[0x80043204,0x8004323C)` to that destination before
tail-dispatching A(44h) FlushCache. The loop's exclusive source endpoint proves the denominator:
`(0x8004323C - 0x80043204) / 4 = 14`.

The PSX-SPX retail-BIOS analysis identifies C0 table slot 6 as C(06h) ExceptionHandler and its value as
`0x00000C80`: <https://psx-spx.consoledev.net/kernelbios/#bios-patches>. This is the same patch pattern
described for other retail titles, not a value inferred from Crash's desired output.

Before psxport `99a42aa3`, `Hle::workAreaInit()` returned a private C0 page from B(56h), but initialized
only offsets `+0x00/+0x04`. Slot 6 at `+0x18` therefore read zero, so Crash would write the patch over
low RAM `0x00000000..0x00000037`. The root cause was incomplete shared BIOS table semantics, not a
Crash address or renderer defect.

## Negative diagnostic

`crash1_c0_exception_contract_probe` drives the shipping HLE B(56h) path and reads slot 6 through the
guest memory interface. Before the fix it produced the required opposite answer, naming returned table
`0x8000F800`, slot address `0x8000F818`, actual zero, and required `0x00000C80`. Against clean pushed
psxport `99a42aa3`, the same executable exits zero and is registered as a normal CTest.

## Proper fix

psxport `99a42aa3` publishes C0 slot 6 as the writable retail exception-handler address `0x00000C80`
and covers it through the shipping B(56h) HLE. The Crash consumer probe verifies that shared behavior.

A Crash-local B(56h) override is rejected: BIOS table ownership is shared framework behavior, and a
title override would only hide the incomplete HLE from every other consumer.

The independent oracle can now apply the ordered syscall and B(56h) returns and seed the consumer-owned
C0 word, but its generic first-post-model-call policy stops at Crash's intermediate local wrapper
before the non-link A(44h) tail dispatch. Issue #8 owns that distinct capture gap and the remaining
register/RAM comparison.
