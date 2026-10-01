# 0023 — Crash 1's widened leaf is never called, and the reason is not the projection

## The situation

The 400-frame disc-backed leg exits 0, executes 3,461,249 blocks and 46,438,388 instructions from
3,011 translated blocks with **0 fallback**, presents 239,549 GP0 primitives, and every captured fence
carries a picture. So neither the run boundary nor the owner explains the missing widened centre:
`crash1_widescreen` installs on the leaf `0x80042F8C`, and the framework's own announcement in the
same run reads `native_width=512 render_width=684` against `512/512` in the 4:3 leg.

**The leaf is never entered.** With input driving the title into a live unpaused level in a 16:9 leg:

    present frame 3903, paused 0, N. SANITY BEACH, 1,384 polygons,
    the widened leaf published a centre 0 time(s)

## What the two callers are

    0x80017824  lui  $s0,0x8005
    0x80017828  addiu $s0,$s0,0x78D0        ; $s0 = 0x800578D0
    0x8001782C  lw   $a0,0($s0)             ; the screen-distance global
    0x80017830  jal  0x8001A7E0
    0x8001783C  jal  0x80042F8C             ; caller A: the camera-DISTANCE publication

    0x80017EE8  lw   $a0,0x193C($a0)        ; the camera-shake word DAT_8006193C
    0x80017EEC  lui  $a1,0x8006
    0x80017EF0  lw   $a1,0x1894($a1)
    0x80017EF4  lui  $v0,0x8006
    0x80017EF8  lw   $v0,0x1940($v0)
    0x80017EFC  sra  $a1,$a1,8
    0x80017F00  jal  0x80042F8C             ; caller B: the per-frame CENTRE publication
    0x80017F04  addu $a1,$a1,$v0            ; delay slot: the centre is completed here

Both are the **camera** publishers, and neither is entered in a live level. The presented content is
the UIS copyright screen and a lit 3D scene — drawn without the per-frame camera publication.

## The owner is not the cause, and this is proved positively

Calling the leaf with a non-zero centre in a wide leg prints:

    call 80042f8c(a0=00000005, a1=0, a2=0, a3=0)
      guest centre 5 -> 91 (retail 5 + margin 86, OFY 0, H 288), host canvas 684 (native 512)

The key intercepts, the owner runs, and retail 5 plus the measured margin 86 is 91. A plain 400-frame
4:3 leg prints exactly one `projection init published` line, so the second one was the injected call:
the control channel really does consult title overrides.

**One trap to keep in mind when reading that owner.** `publishCentre` prints its `guest centre` line
only when the plan is wide AND the retail centre is non-zero, so a 4:3 leg — and any call passing
`$a0 = 0` — is silent whether or not the owner ran. The owner's own invocation count is the reading
to trust, not the log line's absence.

## Who else writes `CR[24]`? Nobody

Over the whole image the guest has exactly two control-register writers per projection register, and
the owner already overrides the entry of both: `0x80042B88` (inside `gte_init`) and `0x80042F94`
(inside `SetGeomOffset`). There is no third writer to move to, and no later write that could revert a
widening. The control **readers** are zero for `CR[24]`, `CR[25]` and `CR[26]`, so nothing branches on
the published offset and a changed `CR[24]` cannot change a gameplay decision.

## What is left open

1. **Why this camera publishes no centre at all.** The owner is alive and correct, the guest writes
   the offset in only the two places the owner owns, and a live unpaused level still publishes none.
   That is a question about the camera's control flow, not about the projection.
2. **The indirect set is not closed.** A direct-call census does not reach `jalr` or a function table,
   and this image has **122 `jalr` sites** and a GPU driver pointer table at `0x80054A24` with **18
   guest-code entries**. "Exactly two callers" was a statement about a scan, not a closed argument.
   Closing it means enumerating both sets and asking which of them can land on `0x80042F8C`.

**Falsifier for the first:** if a caller outside those two exists, the "never entered" reading falls.
**Falsifier for the second:** if the leaf IS entered and only its log line is missing, the premise is
a dead tap again — read the owner's own invocation count.
