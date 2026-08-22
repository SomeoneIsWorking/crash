---
id: 5
title: Crash cannot render because execution stops before BIOS boot
status: open
symptom: Crash has no runnable port binary, frame loop, camera state, or native graphics producer
tags: crash1,rendering,boot,blocked
created: 2026-08-22
updated: 2026-08-22
---

## Root cause

The Crash repository is still a CPU-boundary scaffold, not a game port. The clean pinned gate proves
the resident entry through call eight. That target is the real `addiu $a0, 1; syscall 0` wrapper for
`EnterCriticalSection`; two oracle instructions later the independent CPU correctly enters
`0xBFC00180`. The focused oracle has no BIOS or syscall-exception return model, so it cannot yet
validate Cause/EPC and resume at EPC+4. No port binary, BIOS continuation, frame loop, camera state,
or game-code graphics submitter has been reached.

## What was tried / dead ends

Static emission produced 653 candidates, but only nine addresses have execution provenance. That
count cannot substitute for executing the boot spine, and neither a placeholder picture nor
reconstruction from post-projection OT/GTE output would establish game-owned rendering.

## Proper next step

Add a validated independent-oracle syscall return with Cause/EPC and wrong-selector/wrong-boundary
refusals, then continue the execution spine to the first frame loop. Camera and graphics-submitter RE
begins only after that upstream path is real.

### Note (2026-08-22)
The first text exit is now identified exactly. `SCUS_949.00` call eight targets `0x8003E1F8`
(`addiu $a0, 1; syscall 0`), so step 62,083 at `0xBFC00180` is the independent CPU's correct
syscall exception for `EnterCriticalSection`, not an arbitrary boot crash. The controlled generated
port wrapper returns prior IRQ state `1` and disables IRQ delivery; a different execution-proven
function refuses. Rendering remains blocked because `oracle_trace` has no syscall-exception Cause/EPC
validation or EPC+4 resume, so no post-syscall equality or later boot is claimed.
