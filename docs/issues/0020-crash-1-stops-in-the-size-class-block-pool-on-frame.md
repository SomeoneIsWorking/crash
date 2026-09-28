---
id: 20
title: Crash 1's stop was a FAULT in the size-class block pool; the title now runs to its display wait and presents 0 frames
status: investigating
symptom: the product left guest execution at 0x800159A8 on frame 0 after 8,177,050 guest cycles, and every widescreen leg on record had also run with the CD model reporting no media
state_items: S006,S011
tags: crash1,re,block-pool,native-override,disc-media,measurement
created: 2026-09-28
updated: 2026-09-28
---

## The lead was wrong about the class of failure, and that changed the work

The directive this issue answers described the stop at `0x800159A8` as "a Lightrec budget exit - an
executor budget exhaustion, not a fault". **It is a fault, and the product's own log says so in its
own words** (`scratch/wide/leg_4x3.log`, recorded 2026-09-28T00:25:20Z):

    frame 0 left guest execution at 0x800159A8 with fault after 8177050 cycles (Lightrec execution fault)

Two facts in that one line are decisive. The reason is `fault`, not `BudgetExhausted`; and the detail is
the string `runtime/cpu/lightrec_executor.cpp:624` returns for
`LIGHTREC_EXIT_SEGFAULT | LIGHTREC_EXIT_NOMEM | LIGHTREC_EXIT_UNKNOWN_OP`. Immediately above it, the
guest printed its own diagnosis:

    ERROR: Segmentation fault in recompiled code: invalid load/store at address PC 0x00800004

So the three possibilities the directive named are separated. This is **not** budget exhaustion: the
frame driver at `titles/crash1/core/crash1_frame_driver.cpp:215` loops on `BudgetExhausted` and only
returns when the reason is something else, so a budget exit cannot reach that error line at all. It is
**not** a translation refusal either — that reports `PSXPORT_LIGHTREC_FALLBACK_BLOCK_LIMIT` or a
`fallbackThresholdFault`, and the shutdown telemetry in the same log reports `fallback_blocks=0`. It
is a guest **fault**: an invalid load through a pointer the guest had published.

## What the address is, from the bytes

`0x800159A8` is the word `0x8C620004` — `lw $v0,0x4($v1)`. It is not a function entry; it is the
second class-field read inside the engine's size-class block-cell lookup, whose entry is
`0x80015978`. Recovered in full, with every constant decoded out of the instruction that produces it
by `tools/probe_crash1_block_pool.py`:

    0x80015978  srl  v0,a0,13          ; class key = request >> 13
    0x8001597C  lui  v1,0x8006
    0x80015980  lw   v1,-0x3AD0(v1)    ; v1 = *(0x8005C530), a 256-entry bucket table
    0x80015984  andi v0,v0,0x03FC      ; byte offset (class & 0x3FC)
    0x80015988  addu v0,v0,v1
    0x8001598C  lw   v1,0(v0)          ; the bucket's first cell
    0x80015994  lw   v0,4(v1)          ; does the first cell serve this class?
    0x8001599C  beq  v0,a0,0x800159BC   ; yes -> return it
    0x800159A4  addiu v1,v1,8           ; no -> step one 8-byte cell
    0x800159A8  lw   v0,4(v1)          ; <-- THE STOP
    0x800159B0  bne  v0,a0,0x800159A8   ; the back edge targets 0x800159A8, the loop head
    0x800159B4  addiu v1,v1,8           ; delay slot, always
    0x800159B8  addiu v1,v1,-8          ; what the delay slot is undone by on the match path
    0x800159BC  jr   ra
    0x800159C0  addu v0,v1,zero

    key  = request >> 13
    cell = bucket[(key & 0x3FC) >> 2]
    if (cell->class == key) return cell;
    for (cell += 1; cell->class != key; cell += 1) {}
    return cell;

**Which condition is false:** the only thing in this function that can fault is the class read through
`cell`, and `cell` comes from `*(0x8005C530 + (key & 0x3FC))`. The faulting address was `0x00800004`,
so `$v1` was `0x00800000`, which is in Expansion Region 1 and is not mapped. Nothing else in the body
can fault, and there is no bound check anywhere on the path — including at `0x80015994`, where the
*first* cell is read with no check either.

## The engine's own bound is NOT a usable rule, and that was MEASURED, not reasoned

`0x800159C4` is the same computation WITH a bound, in the same image: it reads a pool base from
`0x8005C534`, the live cell count from `*(0x8005C540)+0x404`, rejects when
`((cell - poolBase) >> 3) >= count`, and returns `0xFFFFFFF6`. It has **0 of 72,192 words** as a `jal`
call site, so it does not run. It is the obvious thing to apply, and applying it was tried and
**measured to be wrong**:

| leg | disc | engine's bound | outcome |
|---|---|---|---|
| A | yes | enforced | "no cell serving class 702 within 576 live cell(s) past pool base 0x80061FA0"; walk stopped at 0x800631A0 after 86 cells; then faulted after 320,508 cycles |
| B | yes | not enforced | reached `0x800170FC`, Crash 1's first measured GpuUpdate display wait |

The reason is measurable: on leg A the pool base `0x80061FA0` is **490 cells** from the bucket's first
cell `0x80062EF0` and the pool publishes **576** live cells, so the bound fires at `0x800631A0` — while
the cell that serves the class lies beyond it. The guest's 256 buckets are not one contiguous array,
so a distance from a single global base is not a bound on any one of them.

**Two corrections this recovery forced on itself, both kept here because a comment hides both.** The
first transcription read `0x8005C534` as the pool's HIGH end; the guest SUBTRACTS it from the cell, so
with the base above the array every in-array cell has a large distance and the walk rejects on its
first step. The test caught it (`the walk read 0x80100018, outside the 3-cell fixture`). The second
was mine: the owner applied the engine's bound, the controlled run above showed it breaking a path
retail completes, and it was removed.

## What is native, and the control that proves it is transparent

`titles/crash1/core/crash1_block_pool.{h,cpp}` is the recovered function, registered through
`crash::dynarec::installOverride` at `0x80015978` from `Crash1Runtime::registerOverrides`. The JIT
still runs everything else. The owner walks exactly as retail does and stops only at the **edge of main
RAM** — a host-memory fact, true for every cell retail can read and false for the one cell retail
faults on. The engine's bound is still read, still evaluated (signed, as the guest's `sra`/`slt` pair
is), and still reported per lookup; it just does not decide.

Transparency is measured, not asserted. Four runs of the same binary, same settings, `--frames 400`:

| run | owner | result |
|---|---|---|
| disc, owner OFF | not installed | `frame 0 reached an unexpected boundary at 0x800170FC with ra=0x800170FC` |
| disc, owner ON, engine's bound enforced | installed | fault after 320,508 cycles; pool named class 702 unservable |
| disc, owner ON, faithful walk | installed | `frame 0 reached an unexpected boundary at 0x800170FC with ra=0x800170FC` — the same place as OFF |
| no disc, owner ON | installed | the same boundary; the pool reports `0 live cell(s)`, first cell `0x8005E688` |

The owner's only added rule therefore fires in exactly the case retail cannot execute, and the run's
outcome is otherwise identical to the override-free control. The owner also names the caller:
**every** lookup in every run came from `ra = 0x80015164`, the return address of the `jal 0x80015978`
at `0x8001515C` inside `FUN_80015118` — which the six-site census could not have chosen, because 818
distinct `jal` targets exist in the same 72,192 words and the graph saturates.

## The frame driver's boundary check was wrong, and that was the next stop

With the pool no longer stopping the run, the next stop was the driver's own guard, and the measured
provenance did not match what it demanded:

    frame 0 reached an unexpected boundary at 0x800170FC with ra=0x800170FC

`0x800170FC` is `after_first_vsync` — the instruction after the guest's own `jal` to the libetc VSync
leaf at `0x800170F4`. psxport's `PlatformHle::vsync` asks for the boundary through the WEAK
`requestExecutionExit(reason)`, so the executor fills the standing architectural PC, and `r[31]` holds
the same continuation because the `jal` put it there. The driver accepted only the CoreLoop transition
override's provenance, so the common case — the guest's own display wait, which the whole frame loop
exists to serve — was reported as unexpected and aborted the run.
`Crash1FrameDriver::isMeasuredFrameBoundary` now accepts both provenances and refuses a mixture, the
VSync leaf's own entry, and any other address; all six cells are pinned in
`tests/crash1_frame_turn.cpp`.

**After that correction the run completes: exit 0**, 1,549 translated blocks, 1,689,262 executed
blocks, 26,457,241 executed instructions, **0 fallback blocks**, and the CD read the disc for real
(104 hunk lookups, 85 hits, 19.5 ms in `chd_read`).

## Why there is still no frame, plainly

**Zero frames are presented, and no drawn aspect is claimed.** The end-of-run line says it directly:

    [producers] run-end: OtAttr spans recorded 0 (overflow 0) — the guest leg's feed

The guest reaches its measured display wait and then submits **no primitives at all**, so there is
nothing to draw. Every `[wide]` line reads `render_width == native_width`. The title runs; it does not
show a picture.

## A second instrument defect, found on the way, and fixed rather than published

Every widescreen leg on record also ran with no media. The logs carry the framework's own words:

    [disc:warn] no disc image: tried PSXPORT_CRASH1_DISC, PSXPORT_DISC (env and ./.env), and a
    *.chd drop-in in the working directory. The CD model will run with NO MEDIA.
    [cd:error] CdRead: LBA 16 unreadable at sector 0/1 - 0 sector(s) delivered

The cause was the tool, not the title: `tools/probe_crash1_widescreen_legs.py` set no disc path at all,
so the leg could never have shown a picture. The user's disc is on this machine (its path is the operator's, so it is not recorded here) and
Crash 1's `SYSTEM.CNF` boot target is already provisioned and verified at
`scratch/bin/crash1/SCUS_949.00`. The probe now
takes `--disc`, passes it as `PSXPORT_CRASH1_DISC`, and **refuses** to report a picture verdict for a
leg whose log shows media was absent. Asked about the pre-existing legs it answers
`(False, 'The CD model will run with NO MEDIA')` for both — the other answer, on real logs.

**Falsifier for the media part of this issue:** a media-less run that reaches the display wait with the
pool reporting a non-zero live cell count. That has not happened — the media-less leg reports
`0 live cell(s)` — so the media is still doing real work even though it is no longer the only thing
that was broken.

## Not established, stated so it is not read as a result

- **Any frame count or drawn aspect for Crash 1.** Zero frames are presented and `OtAttr spans recorded
  0` is the reason. No widening claim of any kind is made from these runs.
- **Why the guest submits no primitives.** The run reaches GpuUpdate's display wait and stops
  submitting. The owner reports five unservable classes on the disc-backed leg and two on the
  media-less leg, all from `FUN_80015118`, so the engine is asking for cells the pool does not hold.
  Whether that is the whole cause or a symptom of an earlier uninitialised structure is NOT
  established: nothing here measured the pool's initialiser (`0x80012F10` reads the same `0x8005C530`
  at `0x80012FC4`) or what should have filled it.
- **The unit of the request word.** The lookup compares each cell's second word against
  `request >> 13`, and the callers do not pass a uniform byte count — `0x80015118` passes a tagged
  handle read out of a struct at `lw a0,0(s0)`. The measured requests are `0x0057CCFB`, `0x15814CE7`,
  `0x5452D94D`, `0x4E938CCD`; none is a plausible byte count, which is consistent with a handle and
  does not establish it.
- **That a `jal` census is the whole call graph.** One class per bucket is this implementation's
  reading of a linear 8-byte-stride walk. A switch-table or function-pointer route would not appear in
  a `jal` census, and none was looked for.

## Proper next step

1. **Own the pool's initialiser.** `0x80012F10` reads the same bucket table at `0x80012FC4` and is the
   allocate path. On the disc-backed leg the table IS built (576 live cells, correctly based), so the
   question is why the cells' class fields do not cover the classes asked for. Read the cell write at
   `0x80013020` (`sw a1,0(s3)`) against the class the lookup compares and name the mismatch.
2. **Recover the primitive submission path** and report how many prims reach the ordering table, with
   the denominator the producers line already carries. `0` of `N` is the number to beat, and `N` has
   to be stated.
3. Nothing in the framework needs changing for the pool or the boundary. If a *framework* segment
   boundary turns out to be wrong, name the file and the seam then; do not raise a cycle budget to
   make a run pass.
