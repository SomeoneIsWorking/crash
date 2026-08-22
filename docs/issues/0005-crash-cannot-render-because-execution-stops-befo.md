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
only the resident entry through call eight. Two oracle instructions later execution leaves mapped
executable text at the BIOS exception vector `0xBFC00180`; no port binary, BIOS continuation, frame
loop, camera state, or game-code graphics submitter has been reached.

## What was tried / dead ends

Static emission produced 653 candidates, but only nine addresses have execution provenance. That
count cannot substitute for executing the boot spine, and neither a placeholder picture nor
reconstruction from post-projection OT/GTE output would establish game-owned rendering.

## Proper next step

Model and differentially verify the BIOS/exception transition, then continue the execution spine to
the first frame loop. Camera and graphics-submitter RE begins only after that upstream path is real.
