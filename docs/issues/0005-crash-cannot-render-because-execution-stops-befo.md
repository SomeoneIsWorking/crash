---
id: 5
title: Crash cannot render because execution stops before BIOS boot
status: open
symptom: The Crash 1 product is composed through B(56h) to the following pre-HLE A(44h) boundary, without verified C0-table state, a frame loop, camera state, or a native graphics producer
tags: crash1,rendering,boot,blocked
state_items: S005,S006,S007,S009
created: 2026-08-22
updated: 2026-08-26
---

## Root cause

The focused harness validates Cause `0x20` and EPC `0x8003E1FC`, resumes both CPUs at `0x8003E200`,
and agrees 34/34 at B(56h) (`pc=0xB0`, `t1=0x56`, `ra=0x800431B8`). Retail disassembly then proves
that Crash consumes the B(56h) result with `lw v0,0x18(v0)`, copies fourteen words, and tail-dispatches
to A(44h) with `ra=0x800431E8`. The product is now composed to allow B(56h) through shipping HLE and
stop before A(44h); its focused Clang build and 14/14 composition test pass, but serialized execution
is still outstanding.

The C0 table defect from issue #7 is resolved in psxport `99a42aa3`: retail BIOS evidence and the
shipping-HLE consumer CTest agree slot 6 is writable C(06h) exception handler `0x00000C80`. The real
ordered oracle completes Crash's fourteen-word patch, then stops at local wrapper `0x8004323C` before
its non-link A(44h) tail dispatch (issue #8). No frame loop, camera state, or game-code graphics
submitter has been reached.

## What was tried / dead ends

Static emission produced 666 candidates, but only nine addresses have execution provenance. That
count cannot substitute for executing the boot spine, and neither a placeholder picture nor
reconstruction from post-projection OT/GTE output would establish game-owned rendering.

## Proper next step

Resolve issue #7 in the shared BIOS HLE and chained oracle, compare A(44h), then continue the execution
spine to the first frame loop. Camera and graphics-submitter RE begins only after the upstream path is
real.

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
