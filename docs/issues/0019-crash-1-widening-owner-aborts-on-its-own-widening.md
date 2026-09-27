---
id: 19
title: Crash 1's widening owner aborts on its own correct widening, and the product refuses frame 0 at BOTH aspects
status: open
symptom: No 4:3-versus-16:9 picture pair. The 16:9 leg dies in the widening owner's guard with `the guest published OFX 86 but $a0 was 5636096`; the 4:3 leg dies in the frame-loop contract with `frame 0 reached an unexpected boundary at 0x800170FC`. Neither leg draws a frame, so there is nothing to compare.
state_items: S006,S010,S011
tags: widescreen,render,evidence,projection,frame-loop,contract,aborted-leg
created: 2026-09-28
updated: 2026-09-28
---

## Answer

**Two independent refusals, and they are not the same defect.** The 4:3 leg's refusal is PRE-EXISTING
and is what actually blocks the pair: the product aborts frame 0 before any scene renders, at either
aspect. The 16:9 leg's refusal is a SECOND, NEWER defect that sits on top of the first, so fixing it
alone would not produce a picture either.

The widening mechanism itself is measurably working from the guest's own registers. That is the
important part of this issue: **the owner is not what is stopping the proof.**

## What the guest published, from its own coprocessor registers

This title's horizontal projection is the GTE screen offset OFX (CR[24]), not a viewport rectangle,
so the guest-side witness is the published register, not `render_width`. Both legs, real
`Crash Bandicoot (USA).chd`, `build/ci/crash1_port`:

| leg | tracked settings | guest's own publication | host canvas | announced `[wide]` line |
|---|---|---|---|---|
| 4:3 | `config/aspect_4x3.ini` (`aspect=0`) | `H 1000, OFX 0, OFY 0` | `320 (native 320)` | `native_width=320 render_width=320` |
| 16:9 | `config/aspect_16x9.ini` (`aspect=1`) | `H 1000, OFX 0, OFY 0` (init) | `428 (native 320)` | `native_width=320 render_width=320` |

`H` is retail's 1000 in both legs. `OFY` is 0 in both. The 16:9 leg latched a 428-wide host canvas
against a 320 native, which is the widening the owner exists to produce.

**The `[wide] native picture:` line reads `render_width == native_width` in BOTH legs and must not be
read as "the widening did not happen."** `picture_announce` prints before the guest's own display
publication, so it reports the 4:3 canvas in a run that is about to widen; the project state already
records this (S006, second framework-side fact). The tool therefore keeps the announce verdict
separate from the guest-side witness and reports both, and its selftest (13/13) asserts they are
separate answers.

## Defect 1 (NEW, `game/core/guest_projection_publication.cpp`): the guard reads `$a0` after the leaf shifted it

The real log line, 16:9 leg, `build/ci/crash1_port` and `build/agent-clang/crash1_port` alike:

    [crash1-wide:error] the guest published OFX 86 but $a0 was 5636096; the plan and the guest's
    own leaf disagree, so the frame would not be the widening it claims

`5636096 == 86 << 16`. The retail leaf, read out of the RAM dump with the framework's own
`tools/disasm.py` over `scratch/raw/crash1.ram` at the manifest's `set_geom_offset` entry:

    80042F8C  00240400  sll      $a0, $a0, 0x10     <- MUTATES $a0 IN PLACE
    80042F90  002c0500  sll      $a1, $a1, 0x10
    80042F94  00c0c448  (COP2 move, Capstone cannot decode this complete word)
    80042F98  00c8c548  (COP2 move, Capstone cannot decode this complete word)
    80042F9C  0800e003  jr       $ra
    80042FA0  00000000  nop
    scanned 6/6 words; decoded 4/6 words; unknown 2; REFUSED incomplete decode

The leaf shifts its argument left by 16 in the argument register itself — whole pixels in, 16.16 out.
`kCentreXArgument` is register 4 (`game/core/guest_projection_publication.cpp:15`), so the sequence in
`publishCentre` is:

    206   core.r[kCentreXArgument] = widenedCentreX(retailX, latched.projectionHorizontalMargin);  // = 86
    ...
    217   retail(core);                                                                           // $a0 -> 86 << 16
    ...
    232   const std::int32_t expectedX = static_cast<std::int32_t>(core.r[kCentreXArgument]);    // = 5636096
    241   if (publishedX != expectedX) { ... std::abort(); }                                      // 86 != 5636096

**The guest published exactly the value the plan asked for.** `publishedX` is 86, read out of
CR[24] as the code intends, and the comparison is against a register the guest's own leaf has already
transformed. `expectedX` is read AFTER the call that destroys it; it has to be captured before line
217, the way `retail_` (line 202) already is for the vertical check — which is why only the
horizontal arm fires and `publishedY != retail_.y` does not.

**Why nothing caught it.** It is invisible at 4:3 by construction: retail's `OFX` is 0, so `86` is
`0`, `0 << 16` is `0`, and `0 == 0` passes. Any non-zero centre trips it. The guard was introduced by
commit `cbf6352` (`git log -S "const std::int32_t expectedX" -- game/core/guest_projection_publication.cpp`),
whose message also claims to fix a sign bug in this same file.

**The fix is one line's worth of ordering** — capture the widened argument into a local before
`retail(core)` and compare against that. It is NOT made here: `game/core/guest_projection_publication.cpp`
is a semantic change to the widening owner and is outside this measurement arm's ownership, and fixing
it alone would still leave Defect 2 blocking the picture.

## Defect 2 (PRE-EXISTING, `titles/crash1/core/crash1_frame_driver.cpp`): frame 0 refuses a boundary the contract does not recognise

    [crash1-frame:error] frame 0 reached an unexpected boundary at 0x800170FC with ra=0x800170FC

The contract at line 259 admits a display-field boundary only when `result.guestPc` is the one
measured VSync site `kContract.guestVSync.begin` (`0x8003E4F0`, the manifest's `vsync.entry`) and
`$ra` is `afterFirstVSync` or `afterVSync`. Here `$ra = 0x800170FC` **is** `after_first_vsync`, so the
continuation is a legal one, but `result.guestPc` is `0x800170FC` rather than `0x8003E4F0`.

Where the provenance came from: the title's own VSync owner states it correctly
(`crash1_frame_driver.cpp:200-202` passes `kContract.guestVSync.begin` explicitly), so the boundary did
not come from there. `psx::cpu::requestExecutionExit(core, reason)` stamps no PC at all
(`runtime/cpu/execution_control.cpp:28-33`, "guestPc 0 means unstated"), and the executor fills an
unstamped request with the standing architectural PC (`runtime/cpu/lightrec_executor.cpp:617-619`).
So the boundary came from `PlatformHle::vsync` — the framework's generic BIOS `A(44h)` WaitVBlank arm
(`runtime/psx/platform_hle.cpp:70`) — through a call site this manifest does not record, and it came
back stamped with the guest's own PC.

**This one predates the latest commit.** Reproduced with `build/verify/crash1_port`, built 11:51 on
2026-09-27, twelve hours before `cbf6352` (23:47) touched either widescreen file:

    [crash1-wide] guest projection init published H 1000, OFX 0, OFY 0 ... host canvas 320 (native 320)
    [crash1-frame:error] frame 0 reached an unexpected boundary at 0x800170FC with ra=0x800170FC

So this is the binding constraint on the pair. It is the same boundary issue 0009 already recorded in
its migration note: *"Crash 1 consumes that result, performs host field work, and resumes at the
measured guest return address"* — the continuation is handled; the **provenance check is not**.

## What the margin census would have said, and why it was not run

**Not run, and not replaced with a substitute.** Both legs abort before a frame is composed, so there
is no presented picture to census. A margin census over an absent frame would be a zero from a scan
that never ran, which is the specific failure mode this repository's diagnostics rules name. The
shared reporter would refuse the pair by name on a missing capture, and `tools/probe_crash1_widescreen_pair.py`
returns 2 with `NOTHING WAS COMPARED, and this is not a pass` rather than substituting a value.

The near-plane hazard in issue 0016 could therefore not be checked for near-geometry loss either.
That is worth stating plainly: `0x800578D0` **is** the GTE near plane (`H < Z < 12000`), and no picture
exists to look for the symptom it would cause.

## What is NOT the cause

- **Not missing media.** The earlier legs failed with `unimplemented BIOS A0:0x27` because no disc was
  provisioned. The CHD is present on this machine
  (`/mnt/Boy/ROM/PSX CHD/Crash Bandicoot (USA).chd`, 33,593 hunks) and both legs now open it. That
  earlier error is fixed; these two are not.
- **Not `aspect=3` (AUTO).** Both legs are pinned by name through `PSXPORT_SETTINGS` by tracked files
  that differ in exactly one key. `PSXPORT_DISC`/`PSXPORT_PRESENT_SINK`/`PSXPORT_PRESENT_SHOT_AT` all
  took effect — the disc opened, and the sink took 320x240 and 428x240 respectively.
- **Not the widening geometry.** The guest published retail's `H 1000` and `OFY 0` in both legs and
  the plan latched a 428-wide canvas. Nothing here suggests the widening drops near geometry; nothing
  here can confirm it either.

## Next step

Resolve Defect 2 first, because it blocks the 4:3 leg and therefore the pair regardless of Defect 1.
The question is specific and answerable from the binary: which call site reaches BIOS `A(44h)`
WaitVBlank at `0x800170FC` in a frame the manifest's `_call_sites` does not list, and whether the
contract's provenance check should admit a boundary the framework's own generic VSync arm raised.
Then Defect 1, which is the ordering fix above. Re-run `tools/probe_crash1_widescreen_pair.py`
unchanged afterwards; it already reports both refusals by name with their owners.
