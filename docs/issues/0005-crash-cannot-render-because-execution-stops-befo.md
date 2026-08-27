---
id: 5
title: Crash 1 has no game-owned native renderer
status: open
symptom: A diagnostic real-disc run reaches 120 host frame fences and observes guest primitives, but earns zero native producer rows and has no visually inspected compatibility frame
state_items: S005,S006,S007,S009
tags: crash1,rendering,native,producers
created: 2026-08-22
updated: 2026-08-27
---

## Root cause

Crash 1's native host loop can execute the generated CoreLoop and reconcile frame fences, but that
timing owner is not a graphics owner. The diagnostic 120-frame run observed 125,680 primitive records
while the native producer database remained empty: zero native rows, zero claims, and zero spans
joined to a native producer. Camera state, pre-GTE object transforms, and game-code graphics submitters
remain unidentified, so no grounded native renderer, widescreen projection, or interpolation path
exists.

## What was tried / dead ends

Static candidate counts and guest primitive counts are not native-producer coverage. Neither a
placeholder picture nor reconstruction from post-projection OT/GTE output would establish game-owned
rendering.

## Proper next step

First reproduce the 120-frame result in a fully isolated run and capture a compatibility-renderer leg
for visual inspection. Then identify camera state and graphics submitters before GTE submission,
instrument those title-owned functions, and earn native producer rows against the same build. Only
owned projection and previous/current transform state can ground widescreen and interpolation.

### Note (2026-08-22)
The first text exit is now identified exactly. `SCUS_949.00` call eight targets `0x8003E1F8`
(`addiu $a0, 1; syscall 0`), so step 62,083 at `0xBFC00180` is the independent CPU's correct
syscall exception for `EnterCriticalSection`, not an arbitrary boot crash. The controlled generated
port wrapper returns prior IRQ state `1` and disables IRQ delivery; a different execution-proven
function refuses. Rendering remains blocked because `oracle_trace` has no syscall-exception Cause/EPC
validation or EPC+4 resume, so no post-syscall equality or later boot is claimed.

### Note (2026-08-22)
SCUS_942.44 now reaches the same honest class of boundary without borrowing SCUS_949.00 addresses: eight generated/oracle call states agree 34/34 through game main 0x80048AA0 and EnterCriticalSection wrapper 0x80048C38; the independent CPU enters 0xBFC00180 at step 75963. Crash 3 rendering remains blocked on the same missing syscall Cause/EPC validation and EPC+4 oracle resume, not on an A(39h) BIOS return.

### Note (2026-08-22)
SCUS-94154 now independently reaches the same honest boundary: generated/oracle state agrees 34/34 at eight serial-specific calls through game main 0x80049BD4 and EnterCriticalSection wrapper 0x80049D1C; the oracle vectors to 0xBFC00180 at step 85,898. All three Crash serials are now blocked by missing syscall Cause/EPC validation and EPC+4 resume. psxport d2266f4b adds same-CPU I_STAT/I_MASK MMIO only; it does not remove this syscall blocker.

### Note (2026-08-24)
Crash 1 is no longer blocked on the absence of a product executable. `crash1_port` and the focused
harness share `ResidentProgram` setup and the measured `EnterCriticalSection` owner, so the product
loads the real executable and reaches the same verified boundary. This removes the false-launcher
structural block but does not resolve this issue: the product exits at the boundary and still renders
no frame.

### Note (2026-08-25)
Recorded psxport `8611d756` and the serial-scoped shipping harness remove the missing Cause/EPC model:
Crash 1, 2, and 3 each pass 16/16, retain the measured syscall exception, resume at EPC+4, and agree
34/34 at the following B0 dispatch. Wrong selector and wrong syscall target refuse. This advances the
differential boundary but does not resolve the issue: the Crash 1 product still stops at the syscall,
and no B0 HLE, frame, input loop, audio loop, camera, or graphics producer has executed.

### Note (2026-08-26)
Against clean psxport 54af32cb, Crash 1 now emits 666 candidates and its focused gate passes 18/18: eight call states agree 34/34, Cause/EPC resume agrees, and the manifest-backed B(56h) boundary agrees at pc=0xB0, t1=0x56, ra=0x800431B8; an altered function selector refuses. crash1_boot_frontier now composes the shipping product with the syscall as a returning transition and B(56h) as its final pre-HLE boundary. A focused Clang product build and 10/10 hermetic configuration test pass. No game was launched during parallel work, so serialized product execution is still required; the B0 HLE, later boot, and first frame remain unresolved.

### Note (2026-08-26)
Ghidra disassembly of the verified retail bytes identifies the exact post-B(56h) continuation:
`0x800431C0` reads C0 slot `+0x18`; its loop copies fourteen words from
`[0x80043204,0x8004323C)`, and
`0x8004323C..0x80043244` tail-dispatches A(44h) with return address `0x800431E8`. The product final
boundary is now that A(44h) pre-HLE dispatch, and the focused Clang build plus 14/14 composition checks
pass. No game was launched. PSX-SPX grounds slot 6 at `+0x18` as C(06h) ExceptionHandler at
`0x00000C80`; psxport `99a42aa3` and the consumer CTest now satisfy that table contract. The ordered
real oracle applies the syscall/B56 returns and C0 seed and completes the patch copy, but its generic
first-call capture stops at local wrapper `0x8004323C`. Issue #8 owns the exact post-model A(44h)
capture needed for the next measurable boundary.

### Note (2026-08-27)
Issues #10 and #11 are resolved. Against pinned psxport `3c342ec3`, the isolated product read the exact
NSD-selected payload, crossed retail page decompression, reconciled 120/120 frame fences, and produced
visually inspected compatibility captures of the SCEA Presents splash without guest VSync. The run
earned no native producer rows; it advances the loop frontier without advancing native rendering.
