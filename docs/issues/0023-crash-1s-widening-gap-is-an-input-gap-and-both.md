# 0023 — Crash 1's widening gap is an INPUT gap, and both callers of the widened leaf are named

## What was believed

`docs/issues/0015` and S006 recorded the 16:9 leg as "the guest published the retail centre but never
the widened one, so the product stopped before its per-frame `SetGeomOffset`". The reason recorded was
a *run* boundary: the product left guest execution on frame 0, so the per-frame publication was never
reached.

## What is measured now

The run boundary is gone. The 400-frame disc-backed leg exits 0, executes 3,461,249 blocks and
46,438,388 instructions from 3,011 translated blocks with **0 fallback**, presents 239,549 GP0
primitives, and every captured fence carries a picture (`docs/issues/0022`). So the run is not the
reason, and the owner is not the reason either: `crash1_widescreen.cpp` installs on the leaf
`kSetGeomOffset = 0x80042F8C`, and the framework's own announcement in the same run reads
`native_width=512 render_width=684` against `512/512` in the 4:3 leg.

**The leaf is simply never entered.** `tools/probe_crash1_widescreen_legs.py` parses the owner's own
`[crash1-wide] guest centre` line, and the 16:9 leg log contains **zero** of them.

## The census that closes it

A whole-image scan of all 72,192 instruction words of `SCUS_949.00`, for every way a `jal`/`j` can
reach the leaf and for every `lui`+`addiu` materialisation of its address:

| reach | count | sites |
|---|---|---|
| direct `jal`/`j` | **2** | `0x8001783C`, `0x80017F00` (both `jal 0x80042F8C`, word `0x0C010BE3`) |
| `lui`+`addiu` materialisation | **0** | — |
| stored pointer / indirect dispatch | none reachable | the leaf is not in a table; `installCrash1Widescreen` binds a `NativeKey` on the address |

So there are exactly two callers, and **neither runs in 400 frames**. That is a closed argument, not
a scan that failed to find a third reader — a leaf with two direct callers and no indirect path is
either called by one of those two or not called.

## What the two callers are, from the decoded words

    0x80017824  lui  $s0,0x8005
    0x80017828  addiu $s0,$s0,0x78D0        ; $s0 = 0x800578D0
    0x8001782C  lw   $a0,0($s0)             ; $a0 = the global the workspace map calls the NEAR PLANE
    0x80017830  jal  0x8001A7E0
    0x8001783C  jal  0x80042F8C             ; caller A: the camera-DISTANCE publication

    0x80017EE8  lw   $a0,0x193C($a0)        ; the camera-shake word DAT_8006193C
    0x80017EEC  lui  $a1,0x8006
    0x80017EF0  lw   $a1,0x1894($a1)
    0x80017EF4  lui  $v0,0x8006
    0x80017EF8  lw   $v0,0x1940($v0)
    0x80017EFC  sra  $a1,$a1,8
    0x80017F00  jal  0x80042F8C             ; caller B: the per-frame CENTRE publication
    0x80017F04  addu $a1,$a1,$v0            ; delay slot: the horizontal centre is completed here

Both are the **camera** publishers: caller A feeds the distance/clip and caller B re-authors the
horizontal centre every frame from the camera-shake word. The owner's comment already said the leaf's
argument "is a live global (0x80017F00 passes DAT_8006193C, the camera-shake word)"; the census says
the whole enclosing path is camera code, and it is not entered.

## Why the run is not in a camera at frame 400

The presented frames are the UIS copyright screen and one lit 3D scene (hut, sky, cloud band, grass)
— content that is drawn without the per-frame camera publication. So the run reaches a rendered
frame and still never enters the code that publishes a camera. The remaining step between those two
facts is **player input**: the attract/UI content advances only on input, and the menu does not move.

## The next step, named

Issue 0012's pad consumption at `0x80057054` is no longer a correctness question on a dead run — the
product now presents a picture, so a held Start/Cross has something to respond to. Measure, in one
run and together:

1. whether the BIOS pad word at `0x80057054` moves while a button is held, and
2. whether the presented frame changes as a result, and
3. whether `0x8001783C` or `0x80017F00` is entered afterwards.

All three in one run, because (1) without (2) is a title that reads input and ignores it, and (2)
without (3) is a title that animates without ever reaching the camera. Any of the three alone is a
fact about the tool.

## Falsifiers

* If a caller of `0x80042F8C` exists outside the two found (an indirect jump through a table, a
  `jr` to a computed address), the census above is wrong and the "never entered" reading falls.
  `tools/probe_crash1_primitives.py` does not currently check that, and it should — the closed
  argument only closes while the indirect set is empty.
* If the leaf IS entered and the owner's line is simply not printed, this issue's premise is a third
  dead tap. The discriminator is cheap: the owner counts its own invocations, and that count is the
  thing to read before trusting the absence of a log line.
