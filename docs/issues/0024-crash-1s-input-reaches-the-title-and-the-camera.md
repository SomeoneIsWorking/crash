# Crash 1 — input reaches the title; the camera path still does not run

Measured 2026-09-29 on this machine with the disc, all of it by `tools/probe_crash1_input.py`, which
drives the product through the framework's own loopback control channel (`PSXPORT_DEBUG_SERVER=1` plus
`external/psxport/tools/dbgclient.py`). It is a product surface, not a test backdoor: no product
argument changes it.

## 1. The input publisher WORKS, and issue 0012's claim was wrong

    before: pad word 0x80057054 = 0xFFFFFFFF, present frame 78,  frame non-black 0.033
    during: 0xFFFFF7FF  x12 samples                     (bit 11 cleared: Start, active-low)
    after:  pad word 0x80057054 = 0xFFFFFFFF, present frame 1304, frame non-black 0.867
    PASS: the pad word moved to 0xFFFFF7FF, the captured frame content changed, and the
          present counter advanced 78 -> 1304 over the same interval.

`docs/issues/0012` recorded "host input did not reach Crash's BIOS auto-pad word". It did. That
measurement was taken on a product that presented nothing (issue 0022's picture did not exist yet), so
it was a fact about the run rather than about the title.

## 2. The title is reachable and answers input

Holding Start and then tapping walks: the UIS copyright screen, the menu
(`START / LOAD GAME / PASSWORD / OPTIONS`), and then a real level — **"N. SANITY BEACH"**, a full 3D
scene at 512x240 with 1,384 polygons in the final frame, over roughly 4,000 presented frames.

## 3. The camera path still does not run — and the PAUSE was NOT the reason

The first run that reached the level ended showing the game PAUSED, so the obvious suspect was that a
paused title does not update its camera. **It is not that.** The control channel's own `frame` reply
carries `paused=`, and with the taps walked through it reads `paused 0` while the presented frame is
a live, unpaused level:

    N. SANITY BEACH, 512x240, Crash on a path with the HUD, 1,403 polygons in the final frame,
    present frame 3960, paused 0, ~4,000 frames after the hold

and **the owner still publishes a centre 0 times.** So the census holds and its conclusion is wrong
in a way that matters: a level reached by input, running, with a camera on screen, does not call the
widened leaf.

What that leaves, and it is a short list:

* **The census missed an indirect path.** It covered direct `jal`/`j` and `lui`+`addiu`
  materialisations. It did NOT close `jr $ra` / `jalr` through a computed register, and this workspace
  has measured that shape before in this very image: the Spider-Man census reported **0** for every
  library routine because that image reaches its routines through `jalr $ra,$vN` pointers, so a
  `jal`-only view is a property of the scan and not of the title. The same scan run here, restricted
  to direct calls, would report 2 and be true — and would still miss a third route. **This is the
  first thing to check**, because the census's own falsifier section named it and it was never run.
* **The level's camera publishes through a sibling path.** The two callers found are the camera
  distance and centre publishers. The question that was NOT asked is "who writes `CR[24]`", which is
  a different and larger census, and a camera mode can publish its centre somewhere the widened owner
  does not sit.
* **The owner is installed but the key does not fire.** `installCrash1Widescreen` binds a `NativeKey`
  on the leaf's address, and a key built against the wrong image silently never fires — the same
  failure the Crash 1 resume test had to defend against. The install line IS in the log, which proves
  `install()` returned true and proves nothing about whether the key matches the executing image.

## 4. The control/treatment pair, and what the positive control did to it

The candidate experiment is recorded here because the first reading of it was WRONG, and the way it
was wrong is the most reusable thing in this file.

**The experiment.** The control channel's `call A [a0..a3]` enters a guest function from outside.
Whether that path consults a title's native overrides is not documented, so a negative result through
it would have said nothing. The same command was therefore aimed at a site whose override is known to
fire from the guest — the projection init at `0x80042B1C` — and at the widened leaf:

    call 80042B1C(a0=1000,...)  -> a SECOND `guest projection init published` line appeared
    call 80042F8C(a0=0,...)     -> v0=00223FFC, and no `guest centre` line

The first reading was "the mechanism is consulted and the leaf's key is dead, so the cause is either a
moved image identity or a suppression that was never released". **Both the control and the treatment
were in 4:3 legs or carried `$a0 = 0`, and `publishCentre` prints its line only under
`latched.widescreen() && retailX != 0`.** So the treatment's silence proved nothing, and the
"second init line" needed its own control: a plain 400-frame leg with no injected calls prints
exactly ONE init line, which is what makes the injected one real — but that only establishes the
mechanism, not the key.

**The positive control, which is the whole point.** A 16:9 leg, `$a0 = 5`:

    call 80042f8c(a0=00000005, a1=0, a2=0, a3=0)
      -> guest centre 5 -> 91 (retail 5 + margin 86, OFY 0, H 288), host canvas 684 (native 512)

The key intercepts, the owner runs, the widening is correct, and both of the "key dead" candidates
are refuted. **So the guest does not call `0x80042F8C` in a live unpaused level** — see
`docs/issues/0023` for what that leaves open.

**The generalisable half.** A negative result read off a log line is only a measurement if the line's
PRINT CONDITION has been satisfied, and the condition here is two-fold and one of the folds is the
aspect the run is in. This is the fifth dead tap in this workspace (`is3d`, the `VSync(0)` census,
`OtAttr`, the Spider-Man gate word, and now the owner's own centre line), and it is the first one that
lives in the owner rather than in a counter. The habit that catches all five: before quoting an
absence, name the feeder AND the condition, and then make the absence fire positively at least once.

## 5. Three defects the tool had, all found by making its own verdict disagree with its own flags

Recorded because a tool that prints a verdict next to a contradicting flag is not a tool, and all
three are the same shape of mistake this workspace keeps making.

* It read the pad word with `w32`, which is a **WRITE**. The reply echoed the address, so the tool
  read `0xFFFFFFFF` back every sample and would have reported "the host's button never reached the
  word" for a run in which it moved on all twelve. The read verb is `rw <addr> [n]`.
* It compared captured frames by **file size**. A fixed-size PPM has a constant length, so a frame
  whose every pixel changed compared equal, and the tool reported "the frame did not change" while
  the non-black fraction moved by 0.83. It compares a content digest now.
* Its `frame` parse looked for a field the reply does not contain, so `advanced` was always False
  while the PASS branch printed "with the frame counter advancing". The verdict now requires every
  flag it names, and a run that cannot establish one exits 2 as UNUSABLE rather than passing.

## 6. The instrument this needs, and does not have
