---
id: 12
title: Crash 1 host input never reaches BIOS PadRead word
status: investigating
symptom: REPL Cross is accepted at the 3D title menu, but PadUpdate stays zero and Start cannot enter gameplay
state_items: S004,S011
tags: crash1,input,bios,pad,gameplay
created: 2026-08-27
updated: 2026-08-27
---

## Root cause

Crash 1 uses the BIOS auto-pad path, not the standard four-byte libpad receive buffers published by
psxport's generic pad service. Retail `PadUpdate` at `0x800167A4` calls the `PadRead` wrapper
`[0x8003E460,0x8003E490)`, whose authenticated LUI/LW pair reads the combined controller word at
`0x80057054` and complements it. The host pad accepted REPL input, but no Crash owner published the
finalized mask to that word, so it remained `0xFFFFFFFF` for both pressed and released frames.

The BIOS word also uses Crash's byte-swapped per-port logical layout: standard active-low Cross
`0xBFFF` must become primary halfword `0xFFBF`, so retail complement/shift yields logical Cross
`0x0040`. Writing a generic four-byte pad packet at this address would corrupt the second port and is
not a valid substitute.

## What was tried / dead ends

An isolated `fb08d30f` real-disc run accepted `tap cross` and `press cross`, but the 3D title menu
never selected Start. With Cross held at frame 1171 and released at frame 1172, `0x80057054` remained
`0xFFFFFFFF` and both retail pad structs at `0x8005E71C` remained zero. This falsifies the idea that
menu timing or a short tap was the cause. The same run visibly reached the main menu and reconciled
1,172/1,172 frame fences, so boot/render progression was not the input failure.

## Work in progress

`crash1_bios_pad_input.*` now publishes the finalized host mask to the manifest-authenticated BIOS
word once per host-owned frame, before retail `PadUpdate`. Its focused production-seam test covers
released, Cross, and Start values plus Crash's post-complement logical interpretation. The changed
Clang product built successfully against the exact `fb08d30f` framework source, but the focused test
was not run after that build and its scratch framework checkout still reports a dirty build identity
because three dependency gitlinks were provisioned as local symlinks. No post-change game run was
performed. Preserve this implementation and its unrun focused proof during the execution migration;
do not rebuild or rerun the static product to finish the issue. The acceptance run is now
CRASH1-JIT-01/02: the native/Lightrec product must publish the same authenticated word, enter
gameplay, and contain neither an interpreter nor generated guest code.
