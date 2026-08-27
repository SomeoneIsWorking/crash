---
id: 11
title: Crash 1 frame driver reads a transposed CoreLoop done address
status: resolved
symptom: The real-disc driver aborts after frame 0 because it reads 0x80036428 instead of the authenticated transition's 0x80056428 done flag
state_items: S004
tags: crash1,frame-loop,transition,boot
created: 2026-08-27
updated: 2026-08-27
---

## Evidence

An isolated real-disc run with the corrected CdControlF binding crossed the authenticated page stream,
decompressed subsequent pages, and entered `Crash1FrameDriver::stepFrame`. The generated CoreLoop
returned during host frame 0, after which the driver's explicit done-path refusal fired before
presentation. No guest VSync was dispatched.

## Root cause

The driver read the wrong address. `runtime.native_frame.done_address` recorded `0x80036428`, while
the authenticated transition body begins `lui v0,0x8005; lw v0,0x6428(v0)` and therefore reads
`0x80056428`. Unrelated nonzero state at the transposed address triggered the done-path refusal even
though the generated transition had returned through its ordinary host seam.

## Resolution

The manifest now records `0x80056428` together with load site `0x80012510`. The executable verifier
decodes that LUI/LW pair and refuses a mismatched address independently of the transition body hash.
Against pinned psxport `3c342ec3`, a fully isolated real-disc PSX-render run completed the 120-frame
cap, reconciled 120/120 frame fences through `beginLogicFrame` 119, returned normally, and emitted no
guest-VSync, fatal, error, or watchdog-timeout line. Presented frames 30, 60, and 119 were captured and
visually inspected as the centered SCEA Presents splash. This proves the transposed done address caused
the earlier frame-0 refusal; it does not prove gameplay or native rendering.
