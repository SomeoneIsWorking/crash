---
id: 5
title: Crash cannot render because execution stops before BIOS boot
status: open
symptom: The Crash 1 product exits at the first syscall boundary without a frame loop, camera state, or native graphics producer
tags: crash1,rendering,boot,blocked
created: 2026-08-22
updated: 2026-08-25
---

## Root cause

The focused harness has removed the old syscall-model blocker: it validates Cause `0x20` and EPC
`0x8003E1FC`, resumes both CPUs at `0x8003E200`, and agrees 34/34 at the following B(56h) dispatch
(`pc=0xB0`, `t1=0x56`, `ra=0x800431B8`). The product still stops after its measured
`EnterCriticalSection` HLE transition, and neither path executes the B0 HLE call. No later BIOS
continuation, frame loop, camera state, or game-code graphics submitter has been reached.

## What was tried / dead ends

Static emission produced 653 candidates, but only nine addresses have execution provenance. That
count cannot substitute for executing the boot spine, and neither a placeholder picture nor
reconstruction from post-projection OT/GTE output would establish game-owned rendering.

## Proper next step

Integrate the proven syscall continuation into `crash1_port`, compare the B(56h) HLE effect and next
real boundary, then continue the execution spine to the first frame loop. Camera and
graphics-submitter RE begins only after that upstream path is real.

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
