---
id: 22
title: "Zero frames are presented" was read off a dead counter; the guest submits 239,549 primitives and 400 presented frames carry a picture
status: resolved
symptom: docs recorded the Crash 1 native/Lightrec frontier as "the guest reaches its display wait and submits no primitives" and "Zero frames are presented", both sourced from one line — [producers] run-end: OtAttr spans recorded 0 (overflow 0)
state_items: S005,S006,S011
tags: crash1,measurement,dead-tap,otattr,presentation,instrument
created: 2026-09-28
updated: 2026-09-28
---

## The claim, and the single line it came from

`docs/issues/0020` and `docs/project-state.md` S011 both carried this as the bottom line of the
Crash 1 frontier:

    [producers] run-end: OtAttr spans recorded 0 (overflow 0) — the guest leg's feed

    Zero frames are presented, and no drawn aspect is claimed.

Both sentences were read off that one number. **The number is a dead tap for this title**, and
`tools/probe_crash1_primitives.py` now measures the two things the sentence was standing in for, in
one process, so the two numbers are directly comparable rather than argued about.

## What one disc-backed run measures

    tools/probe_crash1_primitives.py --disc "$DISC" --frames 400

| quantity | value | denominator |
|---|---|---|
| requested frames | 400 | — |
| translated blocks | 3,011 | — |
| executed blocks | 3,461,249 | — |
| executed instructions | 46,438,388 | — |
| fallback blocks | **0** | of 3,461,249 executed blocks |
| **`OtAttr` spans recorded** | **0** | of 239,549 submitted primitives, same process |
| **guest GP0 primitives** | **239,549** | over 400 frames (frames 1..400) |
| per frame | 1 .. 925, mean 598.9 | over the 400 frames present |
| GP0 `0x7C` (textured rect) | 210,672 | of 239,549 |
| GP0 `0x30` (Gouraud poly) | 28,833 | of 239,549 |
| GP0 `0x2A` (textured Gouraud poly) | 44 | of 239,549 |
| presented frames captured | 6 | of 400 requested |
| non-black pixels in the last captured frame | **35.7 %** (512x240) | of 122,880 pixels |
| captured frames carrying a picture | **6 of 6** | of 6 |

So the guest is not silent and the product is not frameless. `scratch/framedump/f000205_0005_real.png`
is the Universal Interactive Studios copyright screen and `f000400_0005_real.png` is a lit 3D scene
(sky, cloud band, grass, a hut), both captured by the product's own presenter at fence 205 and 400.

## WHY the counter reads zero, from the code path rather than from a guess

`OtAttr` counts a guest store only when it lands inside a configured packet-pool window, and that
window comes from the LEGACY `GameConfig::packetPoolBase/Stride` or the live
`packetPoolBasePtrs/EndPtrs` (`psxport/runtime/psx/ot_attr.cpp`, `pool_range_uncached`, which builds
`RenderNoiseMask::from(c->cfg, "otattr")`). **Crash 1 is a typed `GameRuntime` and declares neither**,
so `c->cfg` is null, the mask is empty, `mPoolCount` stays 0, and `trackStoreSlow` returns before the
span table for every store — whatever the guest drew.

The framework has a warning for exactly this case and it says the right thing:

> packet-pool attribution is STRUCTURALLY BLIND here, so an empty span table means 'not measured',
> NOT 'the guest submitted nothing'

**That warning is unreachable from this path, and that is the defect worth naming.** `poolRangeMiss`
— the only caller of `pool_range_uncached` — runs under `if (c->cfg != mPoolCfg)` in `trackStoreSlow`,
and `OtAttr::mPoolCfg` is initialised to `nullptr` (`ot_attr.h:372`). For a null-config title that
test is `nullptr != nullptr`, i.e. **false**, so the resolution never happens: no window, no warning,
and `native_boot.cpp:286` prints a bare `0` with no caveat. The one place the framework would have
told a reader that the zero is meaningless is the one place a direct runtime cannot reach.

This is the workspace's recurring instrument failure, in its purest form: a metric that reads a tap
nothing writes returns a confident answer about the wrong subject, and the zero it returns is the
most believable possible output. A `GameRuntime`-based port cannot distinguish "the guest drew
nothing" from "this counter has no window" by reading it.

## The discriminator, which is why this is a measurement and not a reading

The falsifier is inside the same run, not beside it. A span count of 0 next to a prim count of 0
would be consistent with a game that draws nothing. A span count of 0 next to **239,549** primitives
in the same process, from the same product, on the same disc, is a measurement of the counter. The
tool prints the span line verbatim and labels it NOT USED, so the number stays visible and its
meaning stays bounded.

The instrument has also been shown the other answer on the shipping product, which is the only way to
trust it:

| invocation | result |
|---|---|
| `--disc <the CHD> --frames 400` | exit 0 — 239,549 prims, 6 of 6 presented frames painted |
| `--disc /nonexistent/... --frames 4` | **exit 1** — `FAIL: the leg ran with NO MEDIA (the log says 'CdRead: LBA 16 unreadable')` |
| no `--disc` | **exit 2** — `REFUSED: pass --disc <the user's CHD>` |
| `--selftest-only` | 12 of 12 cases fired |

The selftest's cases are the decoder's, and they are all refusals or discriminating verdicts: a black
4x2 PNG decodes to 0.0 non-black, a two-pixel-painted 4x2 PNG decodes to 0.25, and a non-PNG, a
truncated PNG, an interlaced PNG, a headerless CSV, a short CSV row, an empty CSV, an absent CSV and a
media-less log are each **refused** rather than answered with a zero. A decoder that returned 0.0 for a
file it could not read would reproduce the very defect this issue is about, one level down.

## What is now claimed, and what is still not

**Claimed, measured above:** the Crash 1 native/Lightrec product boots, reads its disc, executes
26M–46M guest instructions with zero dynarec fallback, submits hundreds of GP0 primitives per frame,
and presents frames whose pixels are 35.7 % non-black at fence 400.

**NOT claimed, and stated here so a reader does not read the table above as more than it is:**

- **No drawn ASPECT is claimed.** Every `[wide]` line still reads `render_width == native_width`, and
  S006's per-frame widening leg is still missing. A picture at 4:3 is not a widescreen leg.
- **This is not gameplay evidence.** The captures are the publisher splash (fence 205) and an
  attract/world scene (fence 400). Nothing here shows input reaching the guest, a level being played,
  or the BIOS pad word being consumed — issue 0012's downstream pad consumption is untouched by this.
- **The `0x30`/`0x2A` primitives are Gouraud polygons submitted by the guest**, and the `is3d` column
  in that CSV is meaningless for this title: the workspace has already measured that `is3d` reads
  psxport's own `ProjPrim` cache, whose three writers have no callers on Lightrec, so it is 0 by
  construction for every Lightrec title. Nothing in this issue is derived from that column.
- **The frontier is not "done".** What moved is the *measurement*. The previous bottom line was a
  counter with no window; the next one has to be a capability — the pad consumption in issue 0012, or
  the widescreen leg in S006.

## Not established, stated so it is not read as a result

- **That the picture is correct.** 35.7 % non-black is a picture, not a comparison. Nothing here was
  diffed against an oracle or against the reference emulator, so nothing here is a parity claim.
- **That 400 frames is enough.** The per-frame prim count is still rising at fence 400 (528 at frame
  200, 925 at frame 399), so the run was cut by the frame cap, not by the game reaching a state. A
  longer run is a separate measurement and was not taken.
- **Anything about the pool from this issue.** The pool owner's unservable-class count is a separate
  instrument (`tools/probe_crash1_block_pool.py`) and a separate claim (issue 0021).

## Proper next step

1. **Give `OtAttr` a window, or make it say it has none.** This is a psxport change, not a crash one:
   `poolRangeMiss` should run when the mask is unresolved, not only when `c->cfg` changes value, so a
   typed `GameRuntime` gets the framework's own "not measured" warning instead of a bare zero. Until
   then, no `GameRuntime`-based port in this workspace may quote that line.
2. **Close issue 0012 against this run.** The product is now visibly presenting, so the pad path has a
   live consumer to be measured against: hold Start and Cross and report whether the BIOS word at
   `0x80057054` moves AND whether the presented picture responds.
3. **Take the S006 leg from this run's shape.** A picture exists, so the widening question is no
   longer blocked on "is there a frame at all" — only on `render_width == native_width`.
