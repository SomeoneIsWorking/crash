---
id: 12
title: Crash 1 menu does not consume published BIOS PadRead input
status: investigating
symptom: REPL Start and Cross update the BIOS PadRead word at the 3D title menu, but the retail pad button structs remain zero and the menu does not advance
state_items: S004,S011
tags: crash1,input,bios,pad,gameplay
created: 2026-08-27
updated: 2026-09-12
---

## Root cause

Crash 1 uses the BIOS auto-pad path, not the standard four-byte libpad receive buffers published by
psxport's generic pad service. Retail `PadUpdate` at `0x800167A4` calls the `PadRead` wrapper
`[0x8003E460,0x8003E490)`, whose authenticated LUI/LW pair reads the combined controller word at
`0x80057054` and complements it. The host pad accepted REPL input, but no Crash owner published the
finalized mask to that word, so it remained `0xFFFFFFFF` for both pressed and released frames.

The BIOS word also uses Crash's byte-swapped per-port logical layout: standard active-low Cross
`0xBFFF` must become primary halfword `0xFFBF`, so retail complement yields logical Cross `0x0040`.
The first publisher incorrectly placed that primary halfword in bits 31–16 and disconnected the low
halfword. Retail `PadUpdate` selects `PadRead() & 0xFFFF` for controller 0 and `>> 16` for controller
1, so it always saw zero buttons for the primary controller. The required word is `0xFFFFFFBF` for
Cross and `0xFFFFF7FF` for Start. Writing a generic four-byte pad packet here would corrupt the
second port and is not a valid substitute.

## What was tried / dead ends

An isolated `fb08d30f` real-disc run accepted `tap cross` and `press cross`, but the 3D title menu
never selected Start. With Cross held at frame 1171 and released at frame 1172, `0x80057054` remained
`0xFFFFFFFF` and both retail pad structs at `0x8005E71C` remained zero. This falsifies the idea that
menu timing or a short tap was the cause. The same run visibly reached the main menu and reconciled
1,172/1,172 frame fences, so boot/render progression was not the input failure.

## Current discriminator

`crash1_bios_pad_input.*` publishes the finalized host mask once per host-owned frame. A 2026-09-12
authenticated Lightrec run before the halfword correction reached the 3D menu at frame 1,172. With
Start held, `0x80057054` became `0xF7FFFFFF`; with Cross held through frame 1,185, it became
`0xFFBFFFFF`. Both words at `0x8005E71C` remained zero and the menu did not advance. Those values
place input in the retail second-port halfword, exactly matching the defect above.

The corrected production publisher and asset-free test put controller 0 in the low halfword and
assert controller 1 remains disconnected. The next retail menu probe must verify the corrected word,
primary current buttons at `0x8005E720`, and actual menu navigation. If the corrected word reaches
the game but the primary struct stays zero, inspect `0x8005E718` (pad count) and `0x80061A30`
(replay suppression) before changing any other owner. The executable's Init `0x8001652C` calls
`0x80016718(2)` to set that count; PadUpdate's direct call sites are scene initialization
`0x80011DD8` and primary-object runtime update `0x8001DA40`.
