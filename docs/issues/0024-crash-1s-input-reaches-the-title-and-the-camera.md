# Crash 1 — input reaches the title; the camera path still does not run

## 1. The input publisher works

A held Start button, read through the framework's own loopback control channel:

    before: pad word 0x80057054 = 0xFFFFFFFF, present frame 78,  frame non-black 0.033
    during: 0xFFFFF7FF  x12 samples                     (bit 11 cleared: Start, active-low)
    after:  pad word 0x80057054 = 0xFFFFFFFF, present frame 1304, frame non-black 0.867

The BIOS auto-pad word at `0x80057054` moves, the captured frame content changes, and the present
counter advances 78 → 1304 over the same interval. The earlier claim that host input never reached
that word was measured on a product that presented nothing, so it was a fact about the run.

## 2. The title is reachable and answers input

Holding Start and then tapping walks the UIS copyright screen, the menu
(`START / LOAD GAME / PASSWORD / OPTIONS`), and then a real level — **"N. SANITY BEACH"**, a full 3D
scene at 512x240 with 1,384 polygons in the final frame, over roughly 4,000 presented frames.

## 3. What is still missing: the camera path

The widened leaf `0x80042F8C` publishes a centre **0 times** in that run, at `paused 0` — the pause
is not the reason. So the level is reached by input, running, with a camera on screen, and the camera
publishes no centre. Issue 0023 owns that question: its two callers are the camera distance and centre
publishers, neither runs, the guest has no other writer of `CR[24]`, and the indirect (`jalr` /
function-table) set was never enumerated.

The owner's own positive control proves the widening itself is correct in a wide leg:

    call 80042f8c(a0=00000005, a1=0, a2=0, a3=0)
      guest centre 5 -> 91 (retail 5 + margin 86, OFY 0, H 288), host canvas 684 (native 512)

## What this needs next

Representative gameplay is still the gate: the run reaches a level's rendered frame, not a state the
title simulates under the player's control. Resolve the camera publication (0023), then interactive
gameplay, then the oracle/device comparison and host qualification of issue 0013.

## Reading the input tool's own output

Two of its readings are easy to misread and both are now guarded in the tool itself: the pad word must
be read with `rw`, never `w32` (which is a write, and would echo `0xFFFFFFFF` back), and captured
frames are compared by content digest, never by file size (a fixed-size PPM has a constant length). A
run that cannot establish every flag its verdict names exits 2 as UNUSABLE rather than passing.
